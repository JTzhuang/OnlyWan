---
phase: 06-fix-duplicate-symbols
plan: 01
subsystem: build/api
tags: [duplicate-symbols, cmake, linker, abi]
dependency_graph:
  requires: []
  provides: [clean-link]
  affects: [src/api/wan-api.cpp, src/api/wan_config.cpp, CMakeLists.txt, examples/cli/avi_writer.h]
tech_stack:
  added: []
  patterns: [explicit-cmake-source-list, wan-api-visibility]
key_files:
  modified:
    - src/api/wan_config.cpp
    - CMakeLists.txt
    - examples/cli/avi_writer.h
  deleted:
    - src/wan_i2v.cpp (untracked duplicate)
decisions:
  - Explicit CMake source list over GLOB_RECURSE to prevent accidental duplicate pickup
  - WAN_API on all 12 wan_params_* functions for correct ABI visibility on Windows shared builds
metrics:
  duration: ~15 min
  completed: 2026-03-16T03:21:18Z
  tasks: 3
  files: 4
---

# Phase 06 Plan 01: Fix Duplicate Symbols Summary

**One-liner:** Eliminated ODR violations by removing param/generation stubs from wan-api.cpp, adding WAN_API to all wan_config.cpp exports, replacing GLOB_RECURSE with an explicit source list, and fixing a broken avi_writer.h include guard.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Remove duplicate wan_params_* and generation stubs from wan-api.cpp | 79996b2 | src/api/wan-api.cpp |
| 2 | Add WAN_API visibility to wan_config.cpp, remove dead code | c521909 | src/api/wan_config.cpp |
| 3 | Fix CMakeLists.txt glob, delete src/wan_i2v.cpp, fix avi_writer.h guard | 44f8471 | CMakeLists.txt, examples/cli/avi_writer.h |

## Deviations from Plan

None - plan executed exactly as written.

## Decisions Made

- Explicit CMake source list: `set(WAN_LIB_SOURCES ...)` replaces `file(GLOB_RECURSE ...)` to prevent `src/wan_i2v.cpp` (untracked duplicate) from being compiled alongside `src/api/wan_i2v.cpp`.
- `WAN_API` macro block was already present in `wan_config.cpp` from a prior fix; only the three missing function annotations needed adding.

## Verification Results

- `nm libwan-cpp.a | grep " T wan_params_create" | wc -l` → 1
- Build completed with 0 "multiple definition" errors
- `src/wan_i2v.cpp` confirmed absent
- `avi_writer.h` guard is `AVI_WRITER_H` (no leading `__`, no dot)
