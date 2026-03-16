# Phase 9: API Fixes + Vocab mmap - Research

**Researched:** 2026-03-16
**Domain:** C++ API wiring, progress callbacks, mmap vocabulary loading
**Confidence:** HIGH

## Summary

Phase 9 addresses three v1.0 legacy issues. First, `wan_generate_video_t2v` and `wan_generate_video_i2v` in `wan_t2v.cpp` and `wan_i2v.cpp` are pure stubs returning `WAN_ERROR_UNSUPPORTED_OPERATION` — they need to delegate to the `_ex` implementations already working in `wan-api.cpp`. Second, the Euler denoising loop in `wan-api.cpp` never calls `params->progress_cb`, so progress callbacks are silently ignored. Third, the `src/vocab/` directory contains four header files totalling 127MB of hex-encoded byte arrays (`umt5.hpp`=54MB, `clip_t5.hpp`=29MB, `mistral.hpp`=35MB, `qwen.hpp`=10MB) compiled into the library at build time — these must be replaced with runtime mmap loading from external files.

The three fixes are independent and low-risk. FIX-01 and FIX-02 are pure changes to existing `.cpp` files with no header or ABI changes. PERF-01 requires replacing the embedded arrays with mmap and updating `CMakeLists.txt` to stop compiling the large headers.

**Primary recommendation:** Fix stubs by forwarding to `_ex` (FIX-01), add `progress_cb` invocation inside the Euler loop (FIX-02), replace embedded vocab arrays with mmap-loaded files using a sidecar path convention (PERF-01).

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| FIX-01 | Remove `wan_generate_video_t2v` / `wan_generate_video_i2v` legacy stubs; call actual `_ex` implementation | `wan_t2v.cpp` and `wan_i2v.cpp` confirmed as pure stubs returning `WAN_ERROR_UNSUPPORTED_OPERATION`; `_ex` functions fully implemented in `wan-api.cpp` |
| FIX-02 | `progress_cb` fires on every Euler denoising step with correct step/total values | Euler loop in `wan-api.cpp` confirmed: no `progress_cb` call present; `params->progress_cb` and `params->user_data` fields exist in `wan_params_t` |
| PERF-01 | Vocab files (~85MB) loaded via mmap at runtime, not embedded in compiled headers | `src/vocab/` confirmed: 127MB total across 4 `.hpp` files; `vocab.cpp` wraps them as `std::string`; `t5.hpp` and `clip.hpp` call `load_umt5_tokenizer_json()` / `load_clip_merges()` from `vocab.h` |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| POSIX mmap | OS-provided | Map vocab files into address space without copying | Zero-copy, lazy page faulting, OS manages eviction |
| sys/mman.h + fcntl.h | POSIX | mmap/munmap/open/close/fstat | Standard POSIX; already on Linux/macOS |
| std::string | C++17 stdlib | Copy mmap region into owned string for tokenizer | Safe lifetime; tokenizer outlives mmap region |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Win32 CreateFileMapping / MapViewOfFile | Win32 | mmap equivalent on Windows | Only when building for Windows |
| std::ifstream fallback | C++17 stdlib | Read vocab file into string if mmap unavailable | Fallback for platforms without mmap |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| mmap + copy to string | read() into std::string | Both copy data; mmap avoids double-buffering at OS level |
| sidecar file path | compile-time path macro | Macro is fragile; sidecar derived from model path is portable |
| sidecar path | GGUF-embedded vocab | GGUF embedding adds format complexity; vocab is plain JSON/text |

**Installation:** No new dependencies. POSIX mmap is available on Linux/macOS. Windows path uses Win32 API.

## Architecture Patterns

### Recommended Project Structure

```
src/vocab/
  vocab.h          # unchanged public interface: load_* declarations
  vocab.cpp        # changed: mmap implementations replacing embedded includes
  umt5.hpp         # kept but no longer included at compile time
  clip_t5.hpp      # kept but no longer included at compile time
  mistral.hpp      # kept but no longer included at compile time
  qwen.hpp         # kept but no longer included at compile time
src/api/
  wan-api.cpp      # FIX-02: add progress_cb call in Euler loop (both T2V and I2V)
  wan_t2v.cpp      # FIX-01: replace stub body with _ex delegation
  wan_i2v.cpp      # FIX-01: replace stub body with _ex delegation
```

### Pattern 1: Stub Delegation (FIX-01)

**What:** `wan_generate_video_t2v` and `wan_generate_video_i2v` build a `wan_params_t` from their flat arguments and call the corresponding `_ex` function.
**When to use:** Legacy flat-arg API must remain ABI-compatible while delegating to the structured `_ex` API.

```cpp
// src/api/wan_t2v.cpp
#include "wan-internal.hpp"
#include "wan.h"

extern "C" {

wan_error_t wan_generate_video_t2v(wan_context_t* ctx,
                                   const char* prompt,
                                   const char* output_path,
                                   int steps, float cfg, int seed,
                                   int width, int height,
                                   int num_frames, int fps,
                                   wan_progress_cb_t progress_cb,
                                   void* user_data) {
    wan_params_t p = {};
    p.steps       = steps;
    p.cfg         = cfg;
    p.seed        = seed;
    p.width       = width;
    p.height      = height;
    p.num_frames  = num_frames;
    p.fps         = fps;
    p.progress_cb = progress_cb;
    p.user_data   = user_data;
    return wan_generate_video_t2v_ex(ctx, prompt, &p, output_path);
}

} // extern "C"
```

### Pattern 2: Progress Callback in Euler Loop (FIX-02)

**What:** After each Euler step, invoke `params->progress_cb` if non-null, passing step index (0-based), total steps, and fractional progress. Respect abort return value.
**When to use:** Inside the denoising loop in both `wan_generate_video_t2v_ex` and `wan_generate_video_i2v_ex` in `wan-api.cpp`.

```cpp
// Inside Euler loop in wan-api.cpp, after ggml_free(step_ctx):
ggml_free(step_ctx);

if (params->progress_cb) {
    int abort = params->progress_cb(i, steps,
                                    (float)(i + 1) / (float)steps,
                                    params->user_data);
    if (abort) {
        ggml_free(denoise_ctx);
        ggml_free(output_ctx);
        return WAN_ERROR_GENERATION_FAILED;
    }
}
```

Note: `wan.h` documents the callback return value: non-zero = abort generation.

### Pattern 3: Vocab mmap Loading (PERF-01)

**What:** Replace embedded hex-array `.hpp` includes in `vocab.cpp` with runtime mmap of external vocab files. The `vocab.h` interface (`load_*` functions) is unchanged — callers see no difference.

**Sidecar path convention (recommended):** Derive vocab file paths from a global vocab directory set at model load time. Falls back to embedded arrays if external files not found, preserving backward compatibility.

```cpp
// src/vocab/vocab.cpp — new implementation
#include "vocab.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>

static std::string g_vocab_dir;

void wan_vocab_set_dir(const std::string& dir) {
    g_vocab_dir = dir;
}

static std::string mmap_read(const std::string& path) {
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) return {};
    struct stat st;
    if (::fstat(fd, &st) < 0) { ::close(fd); return {}; }
    size_t sz = (size_t)st.st_size;
    if (sz == 0) { ::close(fd); return {}; }
    void* addr = ::mmap(nullptr, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (addr == MAP_FAILED) return {};
    std::string result(static_cast<const char*>(addr), sz);
    ::munmap(addr, sz);
    return result;
}

std::string load_umt5_tokenizer_json() {
    if (!g_vocab_dir.empty()) {
        std::string s = mmap_read(g_vocab_dir + "/umt5_tokenizer.json");
        if (!s.empty()) return s;
    }
    // Fallback to embedded array
    #include "umt5.hpp"
    return std::string(reinterpret_cast<const char*>(umt5_tokenizer_json_str),
                       sizeof(umt5_tokenizer_json_str));
}
// ... same pattern for load_clip_merges, load_t5_tokenizer_json, etc.
```

Note: The fallback `#include` inside the function body is unusual but valid C++ and keeps the embedded data available without polluting the TU's global namespace. An alternative is a separate `vocab_embedded.cpp` that only compiles when `WAN_EMBED_VOCAB=ON`.

**CMakeLists.txt change:** Add a `WAN_EMBED_VOCAB` option (default `OFF`). When `OFF`, `vocab.cpp` uses mmap only and does not include the large `.hpp` files, dramatically reducing compile time and binary size.

### Anti-Patterns to Avoid

- **Stub with missing params fields:** The flat-arg stubs must populate ALL relevant `wan_params_t` fields before delegating. Zero-valued width/height/num_frames will produce empty or crashed output.
- **Calling progress_cb before ggml_free(step_ctx):** Free step context first to avoid memory accumulation.
- **Hard-coding vocab file paths:** Paths must be derived from model path or configurable.
- **Removing embedded vocab arrays entirely:** Keep as fallback; removing breaks builds where external vocab files are not present.
- **Including large vocab .hpp files in multiple TUs:** Already avoided; do not change this pattern.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Cross-platform file mapping | Custom file mapping | POSIX mmap + Win32 MapViewOfFile | OS handles page faulting, eviction, sharing |
| Vocab file discovery | Complex search logic | Sidecar: dirname(model_path) + "/vocab/" | Predictable, no search needed |
| Progress fraction | Custom formula | `(float)(i+1)/(float)steps` | Matches `wan.h` callback contract exactly |

**Key insight:** FIX-01 and FIX-02 are trivial wiring changes (~10 lines each). PERF-01 is ~50 lines of POSIX code plus a CMake option.

## Common Pitfalls

### Pitfall 1: wan_params_t zero-initialization in I2V stub
**What goes wrong:** `wan_generate_video_i2v` has no `width`/`height` parameters in its signature (confirmed from `wan.h`). The stub must supply defaults for `_ex`.
**Why it happens:** The flat I2V API predates the structured params API and omitted dimensions.
**How to avoid:** Use `ctx->params.width`/`height` if non-zero, else fall back to 832x480 (WAN2.2 I2V default).
**Warning signs:** `WAN_SUCCESS` returned but output AVI is 0 bytes or has wrong dimensions.

### Pitfall 2: progress_cb abort not handled
**What goes wrong:** Callback returns non-zero to request abort, but the loop continues running all steps.
**Why it happens:** The abort return value is documented in `wan.h` but the loop has no check.
**How to avoid:** Check return value after every callback invocation; break and return `WAN_ERROR_GENERATION_FAILED`.
**Warning signs:** UI cancel button has no effect.

### Pitfall 3: mmap lifetime shorter than tokenizer
**What goes wrong:** mmap region freed while tokenizer still holds a pointer into it.
**Why it happens:** mmap region freed before tokenizer is destroyed.
**How to avoid:** Copy mmap data into `std::string` before returning from `load_*` functions. The `vocab.h` interface already returns `std::string` by value, so this is the natural approach.
**Warning signs:** Segfault or garbage tokens during tokenization.

### Pitfall 4: vocab .hpp files still included after mmap switch
**What goes wrong:** `vocab.cpp` still `#include`s the large `.hpp` files, so compile time and binary size are unchanged.
**Why it happens:** Forgetting to guard the includes with the `WAN_EMBED_VOCAB` CMake option.
**How to avoid:** Gate all four `#include` lines behind `#ifdef WAN_EMBED_VOCAB`.
**Warning signs:** Build still takes minutes; `libwan-cpp.a` is still ~85MB larger than expected.

### Pitfall 5: Windows build breaks on sys/mman.h
**What goes wrong:** POSIX mmap is not available on Windows; build fails with missing header.
**Why it happens:** `sys/mman.h` is POSIX-only.
**How to avoid:** Wrap with `#ifdef _WIN32` using `CreateFileMapping` + `MapViewOfFile`, or use `std::ifstream` fallback on Windows.
**Warning signs:** MSVC build errors on `sys/mman.h` not found.

## Code Examples

### FIX-01: wan_i2v.cpp — I2V stub with width/height defaults

```cpp
// Source: wan.h wan_generate_video_i2v signature (no width/height params)
#include "wan-internal.hpp"
#include "wan.h"

extern "C" {

wan_error_t wan_generate_video_i2v(wan_context_t* ctx,
                                   const wan_image_t* image,
                                   const char* prompt,
                                   const char* output_path,
                                   int steps, float cfg, int seed,
                                   int num_frames, int fps,
                                   wan_progress_cb_t progress_cb,
                                   void* user_data) {
    wan_params_t p = {};
    p.steps       = steps;
    p.cfg         = cfg;
    p.seed        = seed;
    // I2V flat API has no width/height — use context params or I2V defaults
    p.width       = (ctx && ctx->params.width  > 0) ? ctx->params.width  : 832;
    p.height      = (ctx && ctx->params.height > 0) ? ctx->params.height : 480;
    p.num_frames  = num_frames;
    p.fps         = fps;
    p.progress_cb = progress_cb;
    p.user_data   = user_data;
    return wan_generate_video_i2v_ex(ctx, image, prompt, &p, output_path);
}

} // extern "C"
```

### PERF-01: CMakeLists.txt addition

```cmake
# Vocab embedding option — OFF by default to avoid 127MB compile overhead
option(WAN_EMBED_VOCAB "wan-cpp: embed vocabulary in binary (slow build)" OFF)
if(WAN_EMBED_VOCAB)
    target_compile_definitions(${WAN_LIB} PRIVATE WAN_EMBED_VOCAB)
endif()
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Embedded hex arrays in .hpp (127MB) | mmap from external file | Phase 9 | ~85MB removed from binary; compile time drops significantly |
| Stub returning WAN_ERROR_UNSUPPORTED_OPERATION | Delegate to _ex | Phase 9 | Legacy API becomes functional |
| Silent progress (no callback) | progress_cb fires per Euler step | Phase 9 | UI integration becomes possible |

**Deprecated/outdated:**
- Embedded vocab `.hpp` compilation: causes ~127MB of source and significant binary bloat; replaced by runtime mmap with embedded fallback.

## Open Questions

1. **I2V stub width/height defaults**
   - What we know: `wan_generate_video_i2v` has no width/height params; `_ex` requires them via `wan_params_t`
   - What's unclear: Should defaults come from `ctx->params`, from a hardcoded WAN2.2 default (832x480), or from image dimensions?
   - Recommendation: Use `ctx->params.width/height` if non-zero, else 832x480

2. **Vocab file naming convention**
   - What we know: Embedded arrays are named `umt5_tokenizer_json_str`, `clip_merges_utf8_c_str`, etc.
   - What's unclear: What filenames should external vocab files use?
   - Recommendation: `umt5_tokenizer.json`, `clip_merges.txt`, `t5_tokenizer.json`, `mistral_merges.txt`, `mistral_vocab.json`, `qwen2_merges.txt`

3. **Vocab dir discovery**
   - What we know: Model path is available in `ctx->model_path`
   - What's unclear: Should vocab dir default to `dirname(model_path)/vocab/` automatically, or require explicit `wan_set_vocab_dir()` call?
   - Recommendation: Auto-derive from model path as default; allow override via a new `wan_set_vocab_dir()` function added to `vocab.h`/`vocab.cpp` (internal, not public API)

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None detected — no test directory, no test config files |
| Config file | none |
| Quick run command | `cmake --build <build_dir> --target wan-cpp` |
| Full suite command | Manual: run CLI example with known model, verify output AVI |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| FIX-01 | `wan_generate_video_t2v` returns WAN_SUCCESS | smoke | Build + manual CLI run | No — manual only |
| FIX-01 | `wan_generate_video_i2v` returns WAN_SUCCESS | smoke | Build + manual CLI run | No — manual only |
| FIX-02 | `progress_cb` called N times for N steps | unit | No automated test | No — manual only |
| PERF-01 | `libwan-cpp.a` size reduced ~85MB | build metric | `ls -lh bin/libwan-cpp.a` | N/A |
| PERF-01 | Tokenization output identical to embedded path | regression | No automated test | No — manual only |

### Sampling Rate
- Per task commit: `cmake --build /home/jtzhuang/projects/stable-diffusion.cpp/wan/build --target wan-cpp 2>&1 | tail -3`
- Per wave merge: Full build + manual CLI smoke test (T2V + I2V)
- Phase gate: Zero build errors + manual generation succeeds before `/gsd:verify-work`

### Wave 0 Gaps
None — no test infrastructure exists in this project; all validation is build + manual CLI execution.

## Sources

### Primary (HIGH confidence)
- Direct source: `src/api/wan_t2v.cpp` — confirmed stub returning `WAN_ERROR_UNSUPPORTED_OPERATION`
- Direct source: `src/api/wan_i2v.cpp` — confirmed stub returning `WAN_ERROR_UNSUPPORTED_OPERATION`
- Direct source: `src/api/wan-api.cpp` lines 478-518 — confirmed Euler loop has no `progress_cb` call
- Direct source: `include/wan-cpp/wan.h` lines 75, 78-93 — confirmed `progress_cb` signature and `wan_params_t` fields
- Direct source: `src/vocab/` file sizes — confirmed 127MB total embedded hex arrays
- Direct source: `src/t5.hpp` lines 343-348 — confirmed `T5UniGramTokenizer` calls `load_umt5_tokenizer_json()` / `load_t5_tokenizer_json()`
- Direct source: `src/clip.hpp` line 114 — confirmed `CLIPTokenizer` calls `load_clip_merges()`
- Direct source: `src/vocab/vocab.cpp` — confirmed all `load_*` functions return `std::string` from embedded arrays

### Secondary (MEDIUM confidence)
- POSIX mmap(2) — standard POSIX API, well-established on Linux/macOS

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- FIX-01 (stub delegation): HIGH — stubs confirmed by direct code reading; `_ex` signatures confirmed
- FIX-02 (progress callback): HIGH — Euler loop confirmed by direct code reading; callback fields confirmed in `wan_params_t`
- PERF-01 (vocab mmap): HIGH — file sizes confirmed (127MB total); call sites confirmed in `t5.hpp` and `clip.hpp`; mmap pattern is standard POSIX
- I2V width/height default: MEDIUM — requires planner decision on default values

**Research date:** 2026-03-16
**Valid until:** 2026-04-16 (stable codebase, no external dependencies changing)
