---
phase: 07-wire-core-model
plan: 01
subsystem: internal-api
tags: [wan-context, runners, clip, t5, wan-internal]
dependency_graph:
  requires: [06-fix-duplicate-symbols]
  provides: [wan_context-with-runners, CLIPVisionModelProjectionRunner]
  affects: [wan-api.cpp, wan_loader.cpp, wan_t2v.cpp, wan_i2v.cpp]
tech_stack:
  added: []
  patterns: [forward-declarations-for-ODR-safety, shared_ptr-incomplete-types]
key_files:
  created: []
  modified:
    - include/wan-cpp/wan-internal.hpp
    - src/api/wan-api.cpp
    - src/api/wan_loader.cpp
decisions:
  - Forward-declare runner types in wan-internal.hpp to avoid ODR violations; include full headers only in wan-api.cpp
  - CLIPVisionModelProjectionRunner was already present in src/clip.hpp — Task 0 skipped
metrics:
  duration: "~20 min"
  completed: "2026-03-16T05:51:10Z"
  tasks_completed: 2
  files_modified: 3
---

# Phase 7 Plan 1: Wire Core Model — wan_context Restructure Summary

**One-liner:** Replaced stub WanModel/WanVAE wrappers in wan_context with real shared_ptr members for WAN::WanRunner, WAN::WanVAERunner, T5Embedder, and CLIPVisionModelProjectionRunner using forward declarations to prevent ODR violations.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 0 | Add CLIPVisionModelProjectionRunner to src/clip.hpp | (skipped — already present) | src/clip.hpp |
| 1 | Rewrite wan-internal.hpp with real runner members | 9c5c78e | include/wan-cpp/wan-internal.hpp, src/api/wan-api.cpp, src/api/wan_loader.cpp |

## Decisions Made

1. **Forward declarations in wan-internal.hpp** — The heavy headers (wan.hpp, t5.hpp, clip.hpp) contain non-inline function definitions that cause ODR violations when included in multiple TUs. Using forward declarations for runner types in wan-internal.hpp and including full headers only in wan-api.cpp (which owns the shared_ptr lifetimes) eliminates the duplicate symbol errors.

2. **Task 0 skipped** — CLIPVisionModelProjectionRunner was already present in src/clip.hpp (lines 1024-1078) with the exact interface specified in the plan. No changes needed.

3. **WanModel::load removed from wan_loader.cpp** — The old stub implementation referenced the now-deleted WanModelPtr/WanModel types. Removed it; wan_loader.cpp now only implements WanBackend::create.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] ODR violation: DiT:: functions multiply defined**
- **Found during:** Task 1 build verification
- **Issue:** Including wan.hpp (and clip.hpp) in wan-internal.hpp caused DiT::patchify and related functions to be defined in every TU that included wan-internal.hpp, producing linker errors.
- **Fix:** Replaced `#include "wan.hpp"`, `#include "t5.hpp"`, `#include "clip.hpp"` in wan-internal.hpp with forward declarations. Added full includes to wan-api.cpp only.
- **Files modified:** include/wan-cpp/wan-internal.hpp, src/api/wan-api.cpp
- **Commit:** 9c5c78e

**2. [Rule 1 - Bug] wan_loader.cpp referenced deleted WanModel::load and WanModelPtr**
- **Found during:** Task 1
- **Issue:** wan_loader.cpp implemented WanModel::load which used WanModelPtr — both removed from wan-internal.hpp.
- **Fix:** Rewrote wan_loader.cpp to remove WanModel::load, keeping only WanBackend::create and the GGUF utility.
- **Files modified:** src/api/wan_loader.cpp
- **Commit:** 9c5c78e

## Build Verification

```
cmake --build .../wan/build 2>&1 | grep -E "error:|undefined reference"
# (no output — zero errors)
```

## Self-Check: PASSED

| Item | Status |
|------|--------|
| include/wan-cpp/wan-internal.hpp | FOUND |
| src/api/wan-api.cpp | FOUND |
| src/api/wan_loader.cpp | FOUND |
| commit 9c5c78e | FOUND |
