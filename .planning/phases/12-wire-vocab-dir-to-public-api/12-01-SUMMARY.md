---
phase: 12-wire-vocab-dir-to-public-api
plan: 01
subsystem: public-api, vocab, cli
tags: [vocab, public-api, cli, wan_set_vocab_dir, WAN_EMBED_VOCAB]
dependency_graph:
  requires: []
  provides: [wan_set_vocab_dir public API, --vocab-dir CLI flag, load-time vocab dir validation]
  affects: [src/vocab/vocab.h, src/vocab/vocab.cpp, include/wan-cpp/wan.h, src/api/wan-api.cpp, examples/cli/main.cpp, examples/cli/CMakeLists.txt]
tech_stack:
  added: []
  patterns: [WAN_EMBED_VOCAB compile-time guard, stat()/S_ISDIR() directory validation, C public API extension]
key_files:
  created: []
  modified:
    - src/vocab/vocab.h
    - src/vocab/vocab.cpp
    - include/wan-cpp/wan.h
    - src/api/wan-api.cpp
    - examples/cli/main.cpp
    - examples/cli/CMakeLists.txt
decisions:
  - "wan_set_vocab_dir returns WAN_ERROR_INVALID_ARGUMENT for WAN_EMBED_VOCAB=ON builds (no-op with clear contract)"
  - "Load-time guard uses stat()/S_ISDIR() to validate directory existence before model load"
  - "WAN_EMBED_VOCAB propagated explicitly to wan-cli via target_compile_definitions (PRIVATE on wan-cpp, does not auto-propagate)"
metrics:
  duration: 3 min
  completed: 2026-03-17
---

# Phase 12 Plan 01: Wire Vocab Dir to Public API Summary

**One-liner:** `wan_set_vocab_dir` public C API with load-time directory validation and `--vocab-dir` CLI flag wired through WAN_EMBED_VOCAB guards.

## Tasks Completed

| Task | Description | Commit | Files |
|------|-------------|--------|-------|
| 1 | Add vocab accessors, wan_set_vocab_dir, load-time guard | 7241be9 | vocab.h, vocab.cpp, wan.h, wan-api.cpp |
| 2 | Add --vocab-dir to CLI and propagate WAN_EMBED_VOCAB | a396703 | main.cpp, CMakeLists.txt |

## Decisions Made

1. `wan_set_vocab_dir` returns `WAN_ERROR_INVALID_ARGUMENT` for `WAN_EMBED_VOCAB=ON` builds — clear contract that the function is a no-op when vocab is compiled in.
2. Load-time guard uses `stat()`/`S_ISDIR()` (POSIX) and `GetFileAttributesA()` (Windows) to validate the directory exists before proceeding with model load.
3. `WAN_EMBED_VOCAB` must be explicitly propagated to `wan-cli` via `target_compile_definitions` — it is `PRIVATE` on `wan-cpp` and does not propagate to dependents automatically.

## Deviations from Plan

None — plan executed exactly as written.

## Verification Results

- `grep "wan_set_vocab_dir" include/wan-cpp/wan.h` — returns `wan_error_t wan_set_vocab_dir(const char* dir);`
- `grep "wan_vocab_dir_is_set" src/vocab/vocab.h` — returns accessor declaration
- `grep "wan_set_vocab_dir" src/api/wan-api.cpp` — returns WAN_API implementation and wan_vocab_set_dir call
- `grep "vocab_dir" examples/cli/main.cpp` — returns 7 lines (struct, init, parse, usage x2, main x2)
- `grep "WAN_EMBED_VOCAB" examples/cli/CMakeLists.txt` — returns propagation block
- `cmake --build build` — exits 0, all targets built
- `./build/bin/wan-cli --help | grep vocab-dir` — prints `--vocab-dir <path>` line

## Self-Check: PASSED

All 6 modified files exist on disk. Both task commits (7241be9, a396703) confirmed in git log.
