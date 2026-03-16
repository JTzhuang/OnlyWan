# Phase 6: Fix Duplicate Symbols - Research

**Researched:** 2026-03-16
**Domain:** C/C++ linker duplicate symbol resolution, CMake build system
**Confidence:** HIGH

## Summary

Phase 6 is a surgical cleanup phase. The codebase has accumulated three distinct categories of duplicate symbol definitions that will cause linker failure when the library is linked: (1) `wan_params_*` functions defined in both `wan-api.cpp` and `wan_config.cpp`, (2) `wan_generate_video_i2v` and `wan_generate_video_i2v_ex` defined in both `src/wan_i2v.cpp` and `src/api/wan_i2v.cpp`, and (3) `wan_generate_video_t2v`, `wan_generate_video_t2v_ex`, `wan_generate_video_i2v`, `wan_generate_video_i2v_ex` stubbed again in `wan-api.cpp` alongside the real implementations in `wan_t2v.cpp`/`wan_i2v.cpp`. The root cause of the file-level duplicates is the CMake `GLOB_RECURSE` pattern `src/*.cpp` + `src/*/*.cpp` which picks up both `src/wan_i2v.cpp` and `src/api/wan_i2v.cpp`.

The fix strategy is clear: `wan_config.cpp` is the authoritative home for `wan_params_*` (it has validation logic), so those definitions must be removed from `wan-api.cpp`. `src/api/wan_i2v.cpp` is the authoritative I2V file (it lives with the other API files), so `src/wan_i2v.cpp` must be deleted. All generation stubs in `wan-api.cpp` must be removed since `wan_t2v.cpp` and `src/api/wan_i2v.cpp` already define them. The CMake glob must be narrowed to `src/api/*.cpp` plus explicit top-level files. Finally, `avi_writer.h` has a one-character typo in its `#define` guard that must be fixed.

**Primary recommendation:** Remove duplicate definitions from `wan-api.cpp`, delete `src/wan_i2v.cpp`, fix the CMake glob, fix the include guard. No new code needed — this is pure deletion and correction.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| BUILD-01 | CMakeLists.txt supports multi-platform compilation without linker errors | Fix GLOB_RECURSE pattern; explicit source list eliminates duplicate TU pickup |
| API-05 | Config params interface (`wan_params_*` functions) defined and linkable | Remove duplicate definitions from `wan-api.cpp`; keep authoritative copy in `wan_config.cpp` |
</phase_requirements>

## Standard Stack

### Core
| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| CMake `file(GLOB ...)` vs explicit sources | 3.20+ | Source file enumeration | GLOB_RECURSE is the root cause; switching to explicit list or scoped GLOB eliminates the problem |
| C++ One Definition Rule (ODR) | C++17 | Governs symbol uniqueness across TUs | Violated by current setup; fix restores compliance |

### Supporting
| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| `nm` / `objdump` | system | Verify symbols after fix | Run on `.o` files to confirm no duplicate exports |
| `cmake --build` with `VERBOSE=1` | 3.20+ | See exact compiler/linker invocations | Useful to confirm which files are compiled |

## Architecture Patterns

### Recommended Project Structure (after fix)
```
src/
├── util.cpp              # compiled directly
├── model.cpp             # compiled directly
├── name_conversion.cpp   # compiled directly
├── tokenize_util.cpp     # compiled directly
├── vocab/vocab.cpp       # compiled directly
└── api/
    ├── wan-api.cpp       # context, model load, image load, free — NO param or generation defs
    ├── wan_config.cpp    # ALL wan_params_* definitions (authoritative)
    ├── wan_t2v.cpp       # ALL wan_generate_video_t2v* definitions (authoritative)
    ├── wan_loader.cpp    # model loading internals
    └── wan_i2v.cpp       # ALL wan_generate_video_i2v* definitions (authoritative)

# DELETED:
# src/wan_i2v.cpp         <- remove entirely
```

### Pattern 1: Explicit CMake Source List (preferred over GLOB_RECURSE)
**What:** List each source file explicitly instead of using recursive glob
**When to use:** When directory structure has files at multiple depths that must not all be included

```cmake
# Source: CMake official docs — explicit is always safer than GLOB_RECURSE
set(WAN_LIB_SOURCES
    src/util.cpp
    src/model.cpp
    src/name_conversion.cpp
    src/tokenize_util.cpp
    src/vocab/vocab.cpp
    src/api/wan-api.cpp
    src/api/wan_config.cpp
    src/api/wan_t2v.cpp
    src/api/wan_i2v.cpp
    src/api/wan_loader.cpp
)
```

### Pattern 2: Scoped GLOB (acceptable alternative)
**What:** Use separate non-recursive GLOBs for each directory level
**When to use:** When explicit listing is too verbose or files are added frequently

```cmake
# Source: CMake docs — GLOB (not GLOB_RECURSE) is scoped to one directory
file(GLOB WAN_ROOT_SOURCES "src/*.cpp")
file(GLOB WAN_API_SOURCES  "src/api/*.cpp")
file(GLOB WAN_VOCAB_SOURCES "src/vocab/*.cpp")
set(WAN_LIB_SOURCES ${WAN_ROOT_SOURCES} ${WAN_API_SOURCES} ${WAN_VOCAB_SOURCES})
```

### Pattern 3: Include Guard Fix
**What:** `#ifndef` guard macro and `#define` macro must match exactly
**When to use:** Every header file

```c
// BROKEN (current state in avi_writer.h line 8-9):
#ifndef __AVI_WRITER_H__
#define __.AVI_WRITER_H__   // <-- dot instead of underscore — guard never activates

// CORRECT:
#ifndef AVI_WRITER_H
#define AVI_WRITER_H        // also drop leading __ (reserved by C++ standard)
```

Note: Identifiers beginning with `__` (double underscore) are reserved in C++. The corrected guard should use `AVI_WRITER_H` or `WAN_AVI_WRITER_H`.

### Anti-Patterns to Avoid
- **GLOB_RECURSE across multi-depth src/:** Picks up files at both `src/` and `src/api/` — the root cause here. Use scoped GLOBs or explicit lists.
- **Defining public API functions in multiple TUs:** Even if one is "the real one" and others are stubs, the linker sees all of them. Stubs must be removed, not just commented out.
- **Mismatched include guard macros:** The `#ifndef` check passes (macro not defined), the `#define` defines a *different* macro, so the guard never fires on re-inclusion.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Detecting duplicate symbols | Custom script | `nm -C libwan-cpp.a \| sort \| uniq -d` | Standard tooling, zero effort |
| Verifying include guard correctness | Manual review | Compiler warning `-Wguard-alignment` or grep | Compiler catches mismatches |
| Tracking which files CMake compiled | Custom logging | `cmake --build . -- VERBOSE=1` | Built into CMake |

## Common Pitfalls

### Pitfall 1: Removing the wrong copy
**What goes wrong:** Deleting `wan_config.cpp`'s `wan_params_*` instead of `wan-api.cpp`'s — loses validation logic
**Why it happens:** Both files have complete implementations; easy to pick the wrong one
**How to avoid:** `wan_config.cpp` has the `WanConfig` namespace with validation constants and uses them in setters — it is clearly the authoritative copy. `wan-api.cpp`'s copy has no validation (bare `if (params) params->seed = seed`).
**Warning signs:** After deletion, `wan_params_set_steps` accepts `steps=0` without error

### Pitfall 2: Forgetting wan-api.cpp generation stubs
**What goes wrong:** Deleting `src/wan_i2v.cpp` fixes one duplicate, but `wan-api.cpp` still defines `wan_generate_video_i2v` and `wan_generate_video_i2v_ex` as stubs — still a duplicate with `src/api/wan_i2v.cpp`
**Why it happens:** Three-way duplication is easy to miss; audit only caught two files
**How to avoid:** Search `wan-api.cpp` for all `extern "C"` function definitions and cross-check against `wan_t2v.cpp` and `src/api/wan_i2v.cpp`. Lines 296-370 of `wan-api.cpp` contain T2V and I2V stubs that duplicate `wan_t2v.cpp` and `src/api/wan_i2v.cpp`.
**Warning signs:** Linker still fails after deleting `src/wan_i2v.cpp`

### Pitfall 3: CMake glob cache not invalidated
**What goes wrong:** After changing GLOB pattern or deleting a file, CMake uses cached file list and still compiles the deleted file
**Why it happens:** CMake caches GLOB results; re-running cmake (not just make) is required
**How to avoid:** After any CMakeLists.txt change, run `cmake ..` (reconfigure) before `cmake --build .`
**Warning signs:** Build still references deleted file path in error output

### Pitfall 4: wan_set_log_callback_internal orphaned
**What goes wrong:** `wan_config.cpp` defines `wan_set_log_callback_internal` (line 213) which is not declared in any header — becomes an unreferenced symbol after cleanup
**Why it happens:** Was likely a helper added during Phase 3 without a declaration
**How to avoid:** Either add a declaration to `wan-internal.hpp` or remove the function if unused
**Warning signs:** Linker warning about unreferenced symbol (not an error, but noise)

### Pitfall 5: avi_writer.h included multiple times
**What goes wrong:** With the broken guard (`#define __.AVI_WRITER_H__`), every `#include "avi_writer.h"` re-declares `write_u32_le`, `write_u16_le`, `avi_index_entry` — causes redefinition errors in C++ TUs that include it more than once
**Why it happens:** The `#define` uses `.` (period) instead of `_` (underscore), so the guard macro is never set
**How to avoid:** Fix `#define __.AVI_WRITER_H__` to `#define AVI_WRITER_H` (matching the `#ifndef`)

## Code Examples

### Exact symbols duplicated in wan-api.cpp (lines to remove)

```cpp
// Source: direct inspection of src/api/wan-api.cpp lines 113-187 and 296-370

// REMOVE from wan-api.cpp (lines ~113-187) — kept in wan_config.cpp:
WAN_API wan_params_t* wan_params_create(void) { ... }
WAN_API void wan_params_free(wan_params_t* params) { ... }
WAN_API void wan_params_set_seed(...) { ... }
WAN_API void wan_params_set_steps(...) { ... }
WAN_API void wan_params_set_cfg(...) { ... }
WAN_API void wan_params_set_size(...) { ... }
WAN_API void wan_params_set_num_frames(...) { ... }
WAN_API void wan_params_set_fps(...) { ... }
WAN_API void wan_params_set_negative_prompt(...) { ... }
WAN_API void wan_params_set_n_threads(...) { ... }
WAN_API void wan_params_set_backend(...) { ... }
WAN_API void wan_params_set_progress_callback(...) { ... }

// REMOVE from wan-api.cpp (lines ~296-370) — kept in wan_t2v.cpp / src/api/wan_i2v.cpp:
WAN_API wan_error_t wan_generate_video_t2v(...) { ... }
WAN_API wan_error_t wan_generate_video_t2v_ex(...) { ... }
WAN_API wan_error_t wan_generate_video_i2v(...) { ... }
WAN_API wan_error_t wan_generate_video_i2v_ex(...) { ... }
```

### Symbols duplicated across src/wan_i2v.cpp and src/api/wan_i2v.cpp

```cpp
// Source: direct inspection of both files

// Both files define (extern "C"):
wan_error_t wan_generate_video_i2v(...)      // identical stub
wan_error_t wan_generate_video_i2v_ex(...)   // near-identical implementation

// Resolution: DELETE src/wan_i2v.cpp entirely.
// src/api/wan_i2v.cpp is authoritative (lives with other API files).
```

### Fixed CMake source enumeration

```cmake
# Source: CMake 3.20 docs — explicit list preferred for libraries with known files
# Replace the current GLOB_RECURSE block (lines 101-104 of CMakeLists.txt):

# BEFORE (broken):
file(GLOB_RECURSE WAN_LIB_SOURCES
    "src/*.cpp"
    "src/*/*.cpp"
)

# AFTER (fixed — explicit list):
set(WAN_LIB_SOURCES
    src/util.cpp
    src/model.cpp
    src/name_conversion.cpp
    src/tokenize_util.cpp
    src/vocab/vocab.cpp
    src/api/wan-api.cpp
    src/api/wan_config.cpp
    src/api/wan_t2v.cpp
    src/api/wan_i2v.cpp
    src/api/wan_loader.cpp
)
```

### Fixed avi_writer.h include guard

```c
// Source: direct inspection of examples/cli/avi_writer.h lines 8-9

// BEFORE (broken):
#ifndef __AVI_WRITER_H__
#define __.AVI_WRITER_H__

// AFTER (fixed):
#ifndef AVI_WRITER_H
#define AVI_WRITER_H
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| GLOB_RECURSE for all sources | Explicit source list or scoped GLOB per directory | CMake best practice since ~3.x | Prevents accidental inclusion of duplicate/test files |
| Duplicate stubs alongside real impls | Single authoritative TU per function | C ODR — always | Linker can link the library |

**Deprecated/outdated:**
- `GLOB_RECURSE` spanning multiple depth levels in a library: fragile, picks up unintended files. Replaced by explicit lists or per-directory GLOBs.
- Leading `__` in include guard macros: reserved by C++ standard (ISO C++ 17.6.4.3). Use `PROJECTNAME_FILENAME_H` pattern instead.

## Open Questions

1. **wan_set_log_callback_internal in wan_config.cpp**
   - What we know: Defined at line 213 of `wan_config.cpp`, not declared in any header, not called anywhere visible
   - What's unclear: Whether it's intentionally kept for future use or is dead code from Phase 3
   - Recommendation: Remove it during this phase cleanup; if needed later it can be re-added with a proper declaration

2. **WAN_API macro on wan_params_* in wan_config.cpp**
   - What we know: `wan-api.cpp` uses `WAN_API` on all param functions; `wan_config.cpp` does not use `WAN_API`
   - What's unclear: Whether the missing `WAN_API` on `wan_config.cpp` definitions will affect symbol visibility on Windows shared builds
   - Recommendation: Add `WAN_API` to all `extern "C"` function definitions in `wan_config.cpp` to match the header declarations and ensure correct DLL export on Windows

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None detected — no test infrastructure exists |
| Config file | none |
| Quick run command | `cmake --build /home/jtzhuang/projects/stable-diffusion.cpp/wan/build 2>&1 \| grep -E "error\|warning"` |
| Full suite command | `cmake -B /home/jtzhuang/projects/stable-diffusion.cpp/wan/build /home/jtzhuang/projects/stable-diffusion.cpp/wan && cmake --build /home/jtzhuang/projects/stable-diffusion.cpp/wan/build` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| BUILD-01 | Library links without duplicate symbol errors | build smoke | `cmake --build build 2>&1 \| grep -c "multiple definition"` (expect 0) | Wave 0 |
| API-05 | `wan_params_*` defined exactly once | symbol check | `nm build/libwan-cpp.a 2>/dev/null \| grep "wan_params_create" \| grep -c " T "` (expect 1) | Wave 0 |

### Sampling Rate
- **Per task commit:** `cmake --build build 2>&1 | grep -E "error:|multiple definition"`
- **Per wave merge:** Full cmake reconfigure + build
- **Phase gate:** Zero linker errors before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] Build directory must exist: `cmake -B build .` — needed before any build smoke test
- [ ] No unit test framework — validation is build-time only for this phase; linker success is the acceptance criterion

## Sources

### Primary (HIGH confidence)
- Direct source inspection: `src/api/wan-api.cpp` — lines 113-187 (param defs), 296-370 (generation stubs)
- Direct source inspection: `src/api/wan_config.cpp` — lines 117-218 (authoritative param defs)
- Direct source inspection: `src/api/wan_i2v.cpp` vs `src/wan_i2v.cpp` — identical extern "C" symbols
- Direct source inspection: `CMakeLists.txt` lines 101-104 — GLOB_RECURSE root cause
- Direct source inspection: `examples/cli/avi_writer.h` lines 8-9 — broken include guard
- `.planning/v1.0-MILESTONE-AUDIT.md` — audit findings confirming all duplicate locations

### Secondary (MEDIUM confidence)
- CMake documentation pattern: explicit source lists preferred over GLOB_RECURSE for libraries
- C++ standard: identifiers with leading `__` are reserved (ISO C++ 17.6.4.3)

## Metadata

**Confidence breakdown:**
- Duplicate symbol locations: HIGH — confirmed by direct file inspection, line numbers documented
- Fix strategy: HIGH — standard C++ ODR resolution, no ambiguity
- CMake fix: HIGH — GLOB_RECURSE behavior is well-documented and the file list is fully known
- Include guard fix: HIGH — trivial one-character typo with clear correct form

**Research date:** 2026-03-16
**Valid until:** Stable — this is a static codebase fix, not a moving-target library
