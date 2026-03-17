# Phase 12: Wire Vocab Dir to Public API - Research

**Researched:** 2026-03-17
**Domain:** C public API extension, vocab mmap, CLI argument parsing
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Function signature: `wan_error_t wan_set_vocab_dir(const char* dir)`
- Global function, not bound to context (symmetric with internal `wan_vocab_set_dir(const std::string&)`)
- Must be called before `wan_load_model` / `wan_load_model_from_file`
- Declared in `wan.h` "Parameter Configuration" block
- `wan_load_model` validates vocab dir exists at load time; missing dir returns `WAN_ERROR_INVALID_ARGUMENT`
- `wan_set_vocab_dir` itself does NOT validate (store only); validation deferred to load_model
- `WAN_EMBED_VOCAB=ON`: calling `wan_set_vocab_dir` returns a warning-level error code (`WAN_ERROR_INVALID_ARGUMENT` or new `WAN_WARN_IGNORED`; planner decides)
- CLI arg: `--vocab-dir <path>`, optional, consistent with `--model`/`--output` style
- `WAN_EMBED_VOCAB=OFF` build without `--vocab-dir`: print startup warning
- Call order: parse args -> `wan_set_vocab_dir` -> `wan_load_model`

### Claude's Discretion
- Exact comment wording for `wan_set_vocab_dir` in `wan.h`
- Warning message text
- Specific warning code value for `WAN_EMBED_VOCAB=ON` case (reuse existing or add new)

### Deferred Ideas (OUT OF SCOPE)
- Per-context independent vocab directories (requires context-bound API)
- Auto-discovery of vocab directory (future convenience improvement)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| PERF-01 | Vocab files (~85MB) loaded via runtime mmap, not compiled-in arrays; public API exposes the dir path | `wan_vocab_set_dir` already implemented in vocab.cpp:29; Phase 12 bridges it to public API via `wan_set_vocab_dir` in wan-api.cpp |
| ENCODER-01 | T5 text encoder integrated | T5 tokenizer uses `load_umt5_tokenizer_json()` / `load_t5_tokenizer_json()` which read from `g_vocab_dir`; setting dir correctly enables T5 tokenization |
| ENCODER-02 | CLIP image encoder integrated | CLIP tokenizer uses `load_clip_merges()` which reads from `g_vocab_dir`; same fix unblocks CLIP |
| API-03 | T2V generation interface | `wan_generate_video_t2v_ex` calls `t5_embedder->tokenize()` which calls `load_umt5_tokenizer_json()`; vocab dir must be set before `wan_load_model` for tokenizer to work |
| API-04 | I2V generation interface | `wan_generate_video_i2v_ex` calls both T5 and CLIP tokenizers; same dependency on vocab dir |
</phase_requirements>

## Summary

Phase 12 is a thin bridging layer. The internal implementation (`wan_vocab_set_dir` in `src/vocab/vocab.cpp`) is already complete and functional — it sets the global `g_vocab_dir` string that all six `load_*` functions consult before falling back to embedded arrays. The only missing pieces are: (1) a public C API declaration and implementation that calls through to it, (2) a directory-existence check inside `wan_load_model` when `WAN_EMBED_VOCAB` is not defined, and (3) a `--vocab-dir` argument in the CLI.

The codebase follows a strict single-TU rule: `wan-api.cpp` is the only translation unit that includes internal headers. All new public API functions must be implemented there (or in a new `src/api/wan_vocab.cpp` that only includes `wan.h` and `vocab.h`). Since `wan_set_vocab_dir` only needs `vocab.h` (no runner headers), it can safely live in either `wan-api.cpp` or a dedicated thin file. The simplest approach matching existing patterns is to add it directly to `wan-api.cpp`.

The CLI (`examples/cli/main.cpp`) uses a hand-rolled `strcmp`-based argument parser — no getopt, no third-party library. Adding `--vocab-dir` follows the exact same pattern as every other string argument already present. The `WAN_EMBED_VOCAB` compile-time flag is available as a preprocessor macro in the CLI because `wan-cpp` propagates it via `target_compile_definitions` (PRIVATE on the library, but the CLI can check it via its own compile definitions or a header guard).

**Primary recommendation:** Add `wan_set_vocab_dir` to `wan-api.cpp`, add directory-existence check in `wan_load_model`, add `--vocab-dir` to CLI `main.cpp` with a `#ifndef WAN_EMBED_VOCAB` startup warning.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C standard library (`sys/stat.h`, `sys/types.h`) | POSIX | Directory existence check (`stat()`) | Already used in `vocab.cpp` for mmap; same headers available |
| C++ `<string>` | C++17 | Internal string storage for `g_vocab_dir` | Already the type used in `vocab.cpp:27` |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `<filesystem>` (C++17) | C++17 | `std::filesystem::is_directory()` | Alternative to `stat()` for dir check — cleaner but adds a dependency; `stat()` preferred for consistency with existing code |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `stat()` for dir check | `std::filesystem::is_directory()` | filesystem is cleaner but overkill; stat() is already in scope via vocab.cpp includes |
| Adding to `wan-api.cpp` | New `src/api/wan_vocab.cpp` | New file adds CMake churn; wan-api.cpp already has all global-state API functions |

**Installation:** No new dependencies required.

## Architecture Patterns

### Recommended Project Structure
No new files needed. Changes touch:
```
include/wan-cpp/wan.h          # +wan_set_vocab_dir declaration
src/api/wan-api.cpp            # +wan_set_vocab_dir impl, +dir check in wan_load_model
examples/cli/main.cpp          # +--vocab-dir arg, +WAN_EMBED_VOCAB warning
```

### Pattern 1: Global-State API Function (matches existing wan_set_log_callback)
**What:** A `WAN_API`-exported `extern "C"` function that sets a global variable, implemented in `wan-api.cpp`.
**When to use:** Any API function that configures global (not context-bound) state before model load.
**Example:**
```cpp
// Source: src/api/wan-api.cpp (existing pattern — wan_set_log_callback)
WAN_API void wan_set_log_callback(wan_log_cb_t callback, void* user_data) {
    g_log_callback = callback;
    g_log_user_data = user_data;
}

// New function follows identical pattern:
WAN_API wan_error_t wan_set_vocab_dir(const char* dir) {
    // WAN_EMBED_VOCAB=ON: vocab is embedded, external dir is ignored
#ifdef WAN_EMBED_VOCAB
    return WAN_ERROR_INVALID_ARGUMENT;  // or WAN_WARN_IGNORED if planner adds it
#endif
    if (!dir) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }
    wan_vocab_set_dir(std::string(dir));
    return WAN_SUCCESS;
}
```

### Pattern 2: Load-Time Directory Validation (matches existing file-existence check)
**What:** In `wan_load_model`, after argument validation, check that `g_vocab_dir` is a valid directory when `WAN_EMBED_VOCAB` is not defined.
**When to use:** Deferred validation — store path cheaply, validate expensively only when needed.
**Example:**
```cpp
// Source: src/api/wan-api.cpp — wan_load_model, after null checks
#ifndef WAN_EMBED_VOCAB
    {
        // Validate vocab dir exists (g_vocab_dir is accessible via vocab.h)
        struct stat st;
        if (g_vocab_dir_is_empty() || stat(g_vocab_dir_path(), &st) != 0 || !S_ISDIR(st.st_mode)) {
            set_last_error(ctx.get(), "Vocab dir not set or not found; call wan_set_vocab_dir() before wan_load_model()");
            return WAN_ERROR_INVALID_ARGUMENT;
        }
    }
#endif
```
Note: `g_vocab_dir` is a `static std::string` in `vocab.cpp`. To check it from `wan-api.cpp`, the cleanest approach is to add a `wan_vocab_get_dir()` accessor to `vocab.h`/`vocab.cpp`, or expose the check as `wan_vocab_dir_is_set()`. This avoids reaching into the static variable across TU boundaries.

### Pattern 3: CLI String Argument (matches --model, --output, --backend)
**What:** `strcmp`-based argument parsing in `main.cpp`, storing into `cli_options_t`.
**When to use:** Every CLI string argument follows this pattern.
**Example:**
```c
// Source: examples/cli/main.cpp (existing pattern — --backend)
if ((strcmp(arg, "-b") == 0 || strcmp(arg, "--backend") == 0) && i + 1 < argc) {
    opts->backend = argv[++i];
    continue;
}

// New --vocab-dir follows identical pattern:
if (strcmp(arg, "--vocab-dir") == 0 && i + 1 < argc) {
    opts->vocab_dir = argv[++i];
    continue;
}
```

### Pattern 4: WAN_EMBED_VOCAB Compile-Time Guard in CLI
**What:** `#ifndef WAN_EMBED_VOCAB` guard to emit a startup warning when vocab dir is absent.
**When to use:** CLI needs to detect the build configuration at compile time.
**Example:**
```c
// In main(), after parse_args, before wan_load_model:
#ifndef WAN_EMBED_VOCAB
    if (!opts.vocab_dir) {
        fprintf(stderr, "Warning: WAN_EMBED_VOCAB=OFF build but --vocab-dir not provided; vocab loading will fail\n");
    } else {
        wan_set_vocab_dir(opts.vocab_dir);
    }
#else
    if (opts.vocab_dir) {
        wan_set_vocab_dir(opts.vocab_dir);  // returns warning code; ignore or log
    }
#endif
```
For the CLI to see `WAN_EMBED_VOCAB`, it must be added to the CLI's compile definitions in `examples/cli/CMakeLists.txt` (or propagated from the library target). Currently `WAN_EMBED_VOCAB` is defined as `PRIVATE` on `wan-cpp` target, so the CLI does NOT inherit it. The CMakeLists.txt must be updated to pass it through.

### Anti-Patterns to Avoid
- **Accessing `g_vocab_dir` directly from `wan-api.cpp`:** It is `static` in `vocab.cpp` — not accessible across TUs. Use an accessor function declared in `vocab.h`.
- **Validating dir in `wan_set_vocab_dir`:** Locked decision says store-only; validation is deferred to load time.
- **Making `--vocab-dir` required:** Locked decision says optional; absence only triggers a warning.
- **Defining `WAN_EMBED_VOCAB` as PUBLIC on the library target:** It controls internal compilation of 85MB arrays; it must stay PRIVATE. Propagate to CLI via a separate `target_compile_definitions` call.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Directory existence check | Custom file-walking logic | `stat()` + `S_ISDIR` (POSIX) or `GetFileAttributesA` (Win32) | One-liner; already used in same codebase |
| Compile-time vocab flag detection | Runtime config file | `#ifdef WAN_EMBED_VOCAB` preprocessor guard | CMake already defines the macro; no runtime overhead |

**Key insight:** The entire internal plumbing (`g_vocab_dir`, `wan_vocab_set_dir`, `load_vocab_file`) is already implemented and tested. Phase 12 is purely a surface-area exposure task.

## Common Pitfalls

### Pitfall 1: WAN_EMBED_VOCAB not visible to CLI
**What goes wrong:** CLI `main.cpp` uses `#ifndef WAN_EMBED_VOCAB` but the macro is never defined for that TU, so the warning branch never fires even in embedded builds.
**Why it happens:** `target_compile_definitions(${WAN_LIB} PRIVATE WAN_EMBED_VOCAB)` — PRIVATE means it does not propagate to consumers.
**How to avoid:** In `examples/cli/CMakeLists.txt`, add:
```cmake
if(WAN_EMBED_VOCAB)
    target_compile_definitions(wan-cli PRIVATE WAN_EMBED_VOCAB)
endif()
```
**Warning signs:** CLI always prints the "vocab dir not provided" warning even in embedded builds.

### Pitfall 2: Accessing static g_vocab_dir across TU boundary
**What goes wrong:** `wan-api.cpp` tries to read `g_vocab_dir` (defined `static` in `vocab.cpp`) to perform the load-time check — linker error or silent read of wrong variable.
**Why it happens:** `static` at file scope means internal linkage; the symbol is invisible outside `vocab.cpp`.
**How to avoid:** Add a small accessor to `vocab.h` / `vocab.cpp`:
```cpp
// vocab.h
bool wan_vocab_dir_is_set();   // returns !g_vocab_dir.empty()

// vocab.cpp
bool wan_vocab_dir_is_set() { return !g_vocab_dir.empty(); }
```
Then `wan-api.cpp` calls `wan_vocab_dir_is_set()` for the load-time guard.

### Pitfall 3: wan_set_vocab_dir declared in wrong section of wan.h
**What goes wrong:** Function placed outside `extern "C"` block or in wrong section, breaking C linkage or ABI.
**Why it happens:** `wan.h` has multiple sections; the `extern "C"` wraps the entire file between `#ifdef __cplusplus` guards.
**How to avoid:** Insert declaration in the "Parameter Configuration" section (line ~242 in current wan.h), after `wan_params_set_progress_callback`. The `extern "C"` wrapper already covers the whole file.

### Pitfall 4: Missing WAN_API macro on wan_set_vocab_dir
**What goes wrong:** Function not exported from shared library build; linker error for consumers.
**Why it happens:** Forgetting `WAN_API` prefix — all public functions in `wan-api.cpp` require it.
**How to avoid:** Follow the exact pattern of every other function in `wan-api.cpp`: `WAN_API wan_error_t wan_set_vocab_dir(const char* dir)`.

### Pitfall 5: stat() not available on Windows
**What goes wrong:** `stat()` / `S_ISDIR` compile error on MSVC.
**Why it happens:** POSIX `stat` is `_stat` on Windows; `S_ISDIR` is not defined.
**How to avoid:** Use the same `#ifndef _WIN32` / `#else` guard pattern already present in `vocab.cpp`:
```cpp
#ifndef _WIN32
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) { ... }
#else
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) { ... }
#endif
```

## Code Examples

Verified patterns from existing source:

### wan.h declaration style (Parameter Configuration section)
```c
// Source: include/wan-cpp/wan.h lines 260-327 (existing wan_params_set_* pattern)
/** Set vocabulary directory for runtime vocab file loading.
 *
 * Call before wan_load_model() / wan_load_model_from_file().
 * Has no effect (returns WAN_ERROR_INVALID_ARGUMENT) when built with WAN_EMBED_VOCAB=ON.
 *
 * @param dir Path to directory containing vocab files
 *            (umt5_tokenizer.json, t5_tokenizer.json, clip_merges.txt, etc.)
 * @return WAN_SUCCESS on success, WAN_ERROR_INVALID_ARGUMENT if dir is NULL
 *         or if built with WAN_EMBED_VOCAB=ON
 */
wan_error_t wan_set_vocab_dir(const char* dir);
```

### wan-api.cpp implementation
```cpp
// Source: src/api/wan-api.cpp — follows wan_set_log_callback pattern (line 99)
#include "../../src/vocab/vocab.h"  // already included transitively; verify include path

WAN_API wan_error_t wan_set_vocab_dir(const char* dir) {
#ifdef WAN_EMBED_VOCAB
    return WAN_ERROR_INVALID_ARGUMENT;
#endif
    if (!dir) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }
    wan_vocab_set_dir(std::string(dir));
    return WAN_SUCCESS;
}
```

### vocab.h accessor addition
```cpp
// Source: src/vocab/vocab.h — new line after wan_vocab_set_dir declaration
bool wan_vocab_dir_is_set();
```

### wan_load_model guard
```cpp
// Source: src/api/wan-api.cpp — inside wan_load_model, after null-check block
#ifndef WAN_EMBED_VOCAB
    if (!wan_vocab_dir_is_set()) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }
    {
        const std::string& vdir = /* obtained via accessor */ wan_vocab_get_dir();
        struct stat st;
        if (stat(vdir.c_str(), &st) != 0
#ifndef _WIN32
            || !S_ISDIR(st.st_mode)
#else
            // Windows: use GetFileAttributesA
#endif
        ) {
            return WAN_ERROR_INVALID_ARGUMENT;
        }
    }
#endif
```
Simplest approach: add `wan_vocab_get_dir()` returning `const std::string&` to `vocab.h`/`vocab.cpp` alongside `wan_vocab_dir_is_set()`.

### CLI main.cpp additions
```c
// 1. Add field to cli_options_t struct
char* vocab_dir;               // Vocab directory for WAN_EMBED_VOCAB=OFF builds

// 2. init_options: initialize to NULL
opts->vocab_dir = NULL;

// 3. parse_args: add argument handler
if (strcmp(arg, "--vocab-dir") == 0 && i + 1 < argc) {
    opts->vocab_dir = argv[++i];
    continue;
}

// 4. print_usage: document the flag
printf("  --vocab-dir <path>         Vocab files directory (required for WAN_EMBED_VOCAB=OFF builds)\n");

// 5. main(): call wan_set_vocab_dir before wan_load_model
#ifndef WAN_EMBED_VOCAB
    if (!opts.vocab_dir) {
        fprintf(stderr, "Warning: WAN_EMBED_VOCAB=OFF build but --vocab-dir not provided; vocab loading will fail\n");
    } else {
        wan_set_vocab_dir(opts.vocab_dir);
    }
#else
    if (opts.vocab_dir) {
        wan_set_vocab_dir(opts.vocab_dir);
    }
#endif
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Embedded vocab arrays in .hpp (127MB binary, ~2min build) | Runtime mmap from external files via `g_vocab_dir` | Phase 9 plan 02 | Default build is fast; vocab files shipped separately |
| No public API for vocab dir | `wan_set_vocab_dir` (Phase 12) | This phase | Callers can set vocab path without recompiling |

**Deprecated/outdated:**
- `WAN_EMBED_VOCAB=ON` as default: was the original approach; now OFF by default since Phase 9.

## Open Questions

1. **`wan_vocab_get_dir()` vs `wan_vocab_dir_is_set()` accessor design**
   - What we know: `g_vocab_dir` is static in `vocab.cpp`; `wan-api.cpp` needs to read it for load-time validation
   - What's unclear: Whether to expose just a boolean `is_set()` or the full string `get_dir()` for the stat call
   - Recommendation: Expose both `wan_vocab_dir_is_set()` (bool) and `wan_vocab_get_dir()` (const std::string&) — minimal surface, covers both the guard and the stat path

2. **Warning code for WAN_EMBED_VOCAB=ON case**
   - What we know: CONTEXT.md says planner decides between reusing `WAN_ERROR_INVALID_ARGUMENT` or adding `WAN_WARN_IGNORED`
   - What's unclear: Whether downstream callers need to distinguish "bad arg" from "ignored because embedded"
   - Recommendation: Reuse `WAN_ERROR_INVALID_ARGUMENT` — adding a new enum value requires updating all switch statements in consumer code; the doc comment makes the semantics clear

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None detected — no test config files or test directories found |
| Config file | none — Wave 0 gap |
| Quick run command | manual build + CLI invocation |
| Full suite command | manual build + CLI invocation |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PERF-01 | `wan_set_vocab_dir` sets dir; `wan_load_model` uses it for mmap | smoke | `./build/bin/wan-cli --help` (compile check); manual run with vocab dir | ❌ Wave 0 |
| ENCODER-01 | T5 tokenizer loads from vocab dir | smoke | manual: `wan-cli -m model.gguf -p "test" --vocab-dir /path/to/vocab` | ❌ Wave 0 |
| ENCODER-02 | CLIP tokenizer loads from vocab dir | smoke | manual: `wan-cli -m model.gguf -i img.jpg --vocab-dir /path/to/vocab` | ❌ Wave 0 |
| API-03 | T2V generation succeeds with vocab dir set | smoke | manual: T2V invocation with `--vocab-dir` | ❌ Wave 0 |
| API-04 | I2V generation succeeds with vocab dir set | smoke | manual: I2V invocation with `--vocab-dir` | ❌ Wave 0 |

### Sampling Rate
- Per task commit: `cmake --build build --target wan-cli 2>&1 | tail -5` (compile check)
- Per wave merge: full manual smoke test with `--vocab-dir`
- Phase gate: all success criteria from CONTEXT.md verified before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] No automated test framework — all validation is manual CLI invocation
- [ ] `cmake --build build --target wan-cli` must succeed after each change as the compile-time gate

## Sources

### Primary (HIGH confidence)
- Direct source read: `src/vocab/vocab.cpp` — `g_vocab_dir`, `wan_vocab_set_dir`, `load_vocab_file` implementation
- Direct source read: `src/vocab/vocab.h` — existing declarations
- Direct source read: `include/wan-cpp/wan.h` — full public API, section structure, comment style
- Direct source read: `src/api/wan-api.cpp` — `WAN_API` macro pattern, `wan_set_log_callback` global-state pattern, `wan_load_model` structure
- Direct source read: `src/api/wan_config.cpp` — `wan_params_set_*` implementation pattern
- Direct source read: `examples/cli/main.cpp` — argument parsing pattern, `cli_options_t` structure
- Direct source read: `CMakeLists.txt` — `WAN_EMBED_VOCAB` option definition, PRIVATE compile definition
- Direct source read: `examples/cli/CMakeLists.txt` — CLI target configuration

### Secondary (MEDIUM confidence)
- POSIX `stat()`/`S_ISDIR` — standard POSIX API, well-established; Windows `GetFileAttributesA` — Win32 API

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all libraries already in use in the codebase
- Architecture: HIGH — all patterns directly observed in existing source files
- Pitfalls: HIGH — derived from direct code inspection (static linkage, PRIVATE cmake defs, WAN_API macro)

**Research date:** 2026-03-17
**Valid until:** 2026-04-17 (stable codebase, no fast-moving dependencies)
