# Phase 9: API Fixes + Vocab mmap - Research

**Researched:** 2026-03-16
**Domain:** C++ API wiring, progress callbacks, mmap vocabulary loading
**Confidence:** HIGH

## Summary

Phase 9 addresses three v1.0 legacy issues. First, wan_generate_video_t2v and wan_generate_video_i2v in wan_t2v.cpp and wan_i2v.cpp are pure stubs that return WAN_ERROR_UNSUPPORTED_OPERATION — they need to delegate to the _ex implementations already working in wan-api.cpp. Second, the Euler denoising loop in wan-api.cpp never calls params->progress_cb, so progress callbacks are silently ignored. Third, the vocab directory contains four header files totalling 127MB of hex-encoded byte arrays (umt5.hpp=54MB, clip_t5.hpp=29MB, mistral.hpp=35MB, qwen.hpp=10MB) that are compiled into the library at build time — these must be replaced with runtime mmap loading from external files.

The three fixes are independent and low-risk. FIX-01 and FIX-02 are pure changes to existing .cpp files with no header or ABI changes. PERF-01 requires adding a vocab file path to the load API or a discovery convention, replacing the embedded arrays with mmap, and updating CMakeLists.txt to stop compiling the large headers.

**Primary recommendation:** Fix stubs by forwarding to _ex (FIX-01), add progress_cb invocation inside the Euler loop (FIX-02), replace embedded vocab arrays with mmap-loaded files using a sidecar path convention (PERF-01).

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| FIX-01 | Remove wan_generate_video_t2v / wan_generate_video_i2v legacy stubs; call actual _ex implementation | wan_t2v.cpp and wan_i2v.cpp confirmed as pure stubs returning WAN_ERROR_UNSUPPORTED_OPERATION; _ex functions fully implemented in wan-api.cpp |
| FIX-02 | progress_cb fires on every Euler denoising step with correct step/total values | Euler loop in wan-api.cpp confirmed: no progress_cb call present; params->progress_cb and params->user_data fields exist in wan_params_t |
| PERF-01 | Vocab files (~85MB) loaded via mmap at runtime, not embedded in compiled headers | vocab/ directory confirmed: 127MB total across 4 .hpp files; vocab.cpp wraps them as std::string; t5.hpp and clip.hpp call load_umt5_tokenizer_json() / load_clip_merges() from vocab.h |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| POSIX mmap | OS-provided | Map vocab files into address space without copying | Zero-copy, lazy page faulting, OS manages eviction |
| sys/mman.h + fcntl.h | POSIX | mmap/munmap/open/close/fstat | Standard POSIX; already on Linux/macOS; Windows needs CreateFileMapping |
| std::string_view | C++17 | Non-owning view over mmap region | Zero-copy pass to JSON parser; C++17 already required by project |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Windows CreateFileMapping / MapViewOfFile | Win32 | mmap equivalent on Windows | Only when building for Windows |
| std::ifstream fallback | C++17 stdlib | Read vocab file into string if mmap unavailable | Fallback for platforms without mmap |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| mmap | read() into std::string | mmap avoids copy; read() simpler but wastes RAM for 85MB |
| mmap | memory-mapped gguf embedding | GGUF embedding adds format complexity; vocab is plain JSON/text |
| sidecar file path | compile-time path macro | Macro is fragile; sidecar path passed at load time is flexible |

**Installation:** No new dependencies. POSIX mmap is available on Linux/macOS. Windows path uses Win32 API already available.

## Architecture Patterns

### Recommended Project Structure


### Pattern 1: Stub Delegation (FIX-01)
**What:** wan_generate_video_t2v and wan_generate_video_i2v build a wan_params_t from their flat arguments and call the corresponding _ex function.
**When to use:** Legacy flat-arg API must remain ABI-compatible while delegating to the structured _ex API.
**Example:**
# 0 "<stdin>"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "<stdin>"

### Pattern 2: Progress Callback in Euler Loop (FIX-02)
**What:** After each Euler step, invoke params->progress_cb if non-null, passing step index (0-based), total steps, and fractional progress.
**When to use:** Inside the denoising loop in both wan_generate_video_t2v_ex and wan_generate_video_i2v_ex.
**Example:**
# 0 "<stdin>"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "<stdin>"
Note: wan.h documents the callback return value: non-zero = abort. The loop must respect this.

### Pattern 3: Vocab mmap Loading (PERF-01)
**What:** Replace the embedded hex-array .hpp files with runtime mmap of external vocab files. vocab.cpp loads from file path instead of compiled-in arrays.
**When to use:** At T5Embedder / CLIPTokenizer construction time, when the tokenizer JSON/merges data is needed.

**Approach A — vocab path passed via wan_load_model:**
Add  parameter to  (or a new ). Pass down to T5Embedder and CLIPTokenizer constructors.

**Approach B — sidecar convention (recommended):**
Derive vocab file paths from the model file path: look for  and . Falls back to embedded arrays if files not found. This requires no API change.

**Approach C — environment variable / compile-time default:**
 env var or CMake option. Less portable.

Recommendation: Approach B (sidecar) for zero API breakage, with Approach A as opt-in override.

**mmap implementation:**
# 0 "<stdin>"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "<stdin>"

### Anti-Patterns to Avoid
- **Calling _ex from stub without building params:** The flat-arg stubs must populate ALL relevant wan_params_t fields (steps, cfg, seed, width, height, num_frames, fps, progress_cb, user_data) before delegating. Missing fields will use zero-values which may cause incorrect behavior.
- **Calling progress_cb before ggml_free(step_ctx):** step_ctx must be freed first to avoid memory accumulation across steps.
- **Hard-coding vocab file paths:** Paths must be derived from model path or configurable; hard-coded paths break portability.
- **Removing embedded vocab arrays immediately:** Keep the .hpp files and fall back to them if external files are not found. This preserves backward compatibility.
- **Including large vocab .hpp files in multiple TUs:** Already avoided by vocab.cpp being the sole includer. Do not change this.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Cross-platform mmap | Custom file mapping | POSIX mmap + Win32 MapViewOfFile | OS handles page faulting, eviction, sharing |
| Vocab file discovery | Complex search logic | Simple sidecar path: dirname(model_path) + "/vocab/" | Predictable, no search needed |
| Progress fraction | Custom formula | (float)(i+1)/(float)steps | Matches wan.h callback contract |

**Key insight:** The stub delegation and progress callback are trivial wiring changes. The mmap work is the only non-trivial piece, and even that is ~50 lines of POSIX code.

## Common Pitfalls

### Pitfall 1: wan_params_t zero-initialization in stub
**What goes wrong:** Stub builds a local wan_params_t with  but forgets to set width/height/num_frames, which default to 0. The _ex function then computes lW=0, lH=0, lT=0 and produces empty output or crashes.
**Why it happens:** wan_params_t has many fields; easy to miss some when constructing from flat args.
**How to avoid:** Map every flat argument explicitly. Add an assertion or early-return in _ex for zero dimensions.
**Warning signs:** WAN_SUCCESS returned but output AVI is 0 bytes or 0 frames.

### Pitfall 2: progress_cb abort not handled
**What goes wrong:** Callback returns non-zero to request abort, but the loop continues running all steps anyway.
**Why it happens:** The abort return value is documented in wan.h but the loop has no check.
**How to avoid:** Check return value after every callback invocation; break and return WAN_ERROR_GENERATION_FAILED.
**Warning signs:** UI cancel button has no effect.

### Pitfall 3: mmap lifetime shorter than tokenizer
**What goes wrong:** MmapFile goes out of scope and unmaps the memory while T5UniGramTokenizer still holds a pointer/string_view into it.
**Why it happens:** mmap region freed before tokenizer is destroyed.
**How to avoid:** Either copy the mmap data into a std::string (safe, small overhead), or keep MmapFile alive as long as the tokenizer. Since vocab.cpp returns std::string already, copying is the correct approach — mmap just avoids the compile-time embedding, not the runtime copy.
**Warning signs:** Segfault or garbage tokens during tokenization.

### Pitfall 4: vocab .hpp files still included via vocab.cpp
**What goes wrong:** After switching to mmap, vocab.cpp still #includes the large .hpp files, so compile time and binary size are unchanged.
**Why it happens:** Forgetting to remove the #include lines from vocab.cpp.
**How to avoid:** Remove all four #include lines from vocab.cpp when mmap path is active. Guard with a CMake option if fallback is needed.
**Warning signs:** Build still takes minutes; libwan-cpp.a is still ~85MB larger than expected.

### Pitfall 5: Windows build breaks
**What goes wrong:** POSIX mmap (sys/mman.h) is not available on Windows; build fails.
**Why it happens:** mmap is POSIX-only.
**How to avoid:** Wrap with  /  using CreateFileMapping + MapViewOfFile on Windows, or use std::ifstream fallback on Windows.
**Warning signs:** MSVC build errors on sys/mman.h not found.

## Code Examples

### FIX-01: Complete stub replacement for wan_t2v.cpp
# 0 "<stdin>"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "<stdin>"

### FIX-01: Complete stub replacement for wan_i2v.cpp
# 0 "<stdin>"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "<stdin>"

Note: wan_generate_video_i2v does not have width/height parameters in its signature (confirmed from wan.h). The _ex function requires them via params. Use sensible defaults or read from ctx.

### FIX-02: Progress callback insertion point in Euler loop
# 0 "<stdin>"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "<stdin>"

### PERF-01: vocab.cpp with mmap + fallback
# 0 "<stdin>"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "<stdin>"

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Embedded hex arrays in .hpp | mmap from external file | Phase 9 | ~85MB removed from binary; compile time drops significantly |
| Stub returning WAN_ERROR_UNSUPPORTED_OPERATION | Delegate to _ex | Phase 9 | Legacy API becomes functional |
| Silent progress (no callback) | progress_cb fires per step | Phase 9 | UI integration becomes possible |

**Deprecated/outdated:**
- Embedded vocab .hpp compilation: causes ~85MB binary bloat and multi-minute compile times; replaced by runtime mmap.

## Open Questions

1. **I2V stub width/height defaults**
   - What we know: wan_generate_video_i2v has no width/height params; wan_generate_video_i2v_ex requires them via wan_params_t
   - What's unclear: Should defaults come from ctx->params, from a hardcoded WAN2.2 default (832x480), or should the stub read image dimensions?
   - Recommendation: Use ctx->params.width/height if set (non-zero), else fall back to 832x480 for I2V

2. **Vocab file naming convention**
   - What we know: Current embedded arrays are named umt5_tokenizer_json_str, clip_merges_utf8_c_str, etc.
   - What's unclear: What filenames should the external vocab files use?
   - Recommendation: umt5_tokenizer.json, clip_merges.txt, mistral_merges.txt, mistral_vocab.json, qwen2_merges.txt — matching the logical names in vocab.h

3. **Fallback behavior when vocab files missing**
   - What we know: Embedded arrays are always available as fallback
   - What's unclear: Should missing external vocab files be a hard error or silent fallback?
   - Recommendation: Silent fallback to embedded arrays; log a warning if log callback is set

4. **CMakeLists.txt: remove vocab .hpp from compile**
   - What we know: vocab.cpp currently #includes all four .hpp files; they are compiled into vocab.cpp.o
   - What's unclear: Whether to keep the .hpp files as optional fallback or remove them entirely
   - Recommendation: Keep .hpp files, guard their inclusion with a CMake option WAN_EMBED_VOCAB (default OFF for new builds)

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None detected — no test directory, no pytest.ini, no jest.config |
| Config file | none |
| Quick run command | Build:  |
| Full suite command | Manual: run CLI example with known model and check output |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| FIX-01 | wan_generate_video_t2v returns WAN_SUCCESS (not WAN_ERROR_UNSUPPORTED_OPERATION) | smoke | Build + run CLI with t2v model | No test file — manual only |
| FIX-01 | wan_generate_video_i2v returns WAN_SUCCESS | smoke | Build + run CLI with i2v model | No test file — manual only |
| FIX-02 | progress_cb called N times for N steps with correct step/total | unit | No automated test — manual callback inspection | No test file |
| PERF-01 | libwan-cpp.a size reduced by ~85MB | build metric |  after build | N/A |
| PERF-01 | Tokenization produces same tokens as embedded path | regression | No automated test — manual comparison | No test file |

### Sampling Rate
- Per task commit: [  7%] Built target ggml-base
[ 83%] Built target ggml-cuda
[ 91%] Built target ggml-cpu
[ 92%] Built target ggml
[100%] Built target wan-cpp
- Per wave merge: Full build + manual CLI smoke test
- Phase gate: Build zero errors + manual T2V and I2V generation succeeds before verify

### Wave 0 Gaps
- No test infrastructure exists in this project. All validation is manual build + CLI execution.
- None — existing pattern is build verification only; no test files to create.

## Sources

### Primary (HIGH confidence)
- Direct source code analysis:  — confirmed stub returning WAN_ERROR_UNSUPPORTED_OPERATION
- Direct source code analysis:  — confirmed stub returning WAN_ERROR_UNSUPPORTED_OPERATION
- Direct source code analysis:  lines 478-518 — confirmed Euler loop has no progress_cb call
- Direct source code analysis:  lines 75, 89-93 — confirmed progress_cb signature and params fields
- Direct source code analysis:  — confirmed 127MB total embedded hex arrays
- Direct source code analysis:  lines 343-348 — confirmed T5UniGramTokenizer calls load_umt5_tokenizer_json() / load_t5_tokenizer_json()
- Direct source code analysis:  line 114 — confirmed CLIPTokenizer calls load_clip_merges()
- Direct source code analysis:  — confirmed all load_* functions return std::string from embedded arrays

### Secondary (MEDIUM confidence)
- POSIX mmap(2) man page pattern — standard POSIX API, well-established

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- FIX-01 (stub delegation): HIGH — stubs confirmed by direct code reading; _ex signatures confirmed
- FIX-02 (progress callback): HIGH — Euler loop confirmed by direct code reading; callback fields confirmed in wan_params_t
- PERF-01 (vocab mmap): HIGH — file sizes confirmed (127MB total); call sites confirmed in t5.hpp and clip.hpp; mmap pattern is standard POSIX
- I2V width/height default: MEDIUM — requires planner decision on default values

**Research date:** 2026-03-16
**Valid until:** 2026-04-16 (stable codebase, no external dependencies changing)
