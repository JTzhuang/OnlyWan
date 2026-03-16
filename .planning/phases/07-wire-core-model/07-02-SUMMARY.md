---
phase: 07-wire-core-model
plan: 02
subsystem: model-loading
tags: [model-loader, wan-runner, vae-runner, t5-embedder, clip-runner, gguf]
dependency_graph:
  requires: [07-01]
  provides: [WanModel-load, full-weight-loading]
  affects: [wan-api.cpp, wan_loader.cpp, wan-internal.hpp]
tech_stack:
  added: []
  patterns: [ModelLoader-weight-loading, single-TU-runner-construction, dynamic-prefix-detection]
key_files:
  created: []
  modified:
    - include/wan-cpp/wan-internal.hpp
    - src/api/wan-api.cpp
    - src/api/wan_loader.cpp
decisions:
  - WanModel::load implemented in wan-api.cpp (not wan_loader.cpp) to avoid ODR violations from non-inline runner definitions
  - is_wan_gguf kept in wan_loader.cpp as a non-static function for reuse; declared in wan-internal.hpp
  - T5 and CLIP prefixes detected dynamically from tensor_storage_map at load time
metrics:
  duration: "5 min"
  completed: "2026-03-16T05:59:53Z"
  tasks_completed: 1
  files_modified: 3
---

# Phase 7 Plan 2: Wire Core Model — Weight Loading Summary

**One-liner:** ModelLoader-based GGUF weight loading for WanRunner, WanVAERunner, T5Embedder, and CLIPVisionModelProjectionRunner with dynamic prefix detection, implemented in wan-api.cpp to avoid ODR violations.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Rewrite WanModel::load() to use ModelLoader and populate all runners including CLIP | e1a045e | include/wan-cpp/wan-internal.hpp, src/api/wan-api.cpp, src/api/wan_loader.cpp |

## Decisions Made

1. **WanModel::load in wan-api.cpp** — Moving the implementation to wan-api.cpp (which already includes wan.hpp, t5.hpp, clip.hpp) avoids the ODR violations that caused duplicate symbol linker errors when wan_loader.cpp included those same headers. This is the same pattern established in Plan 01.

2. **is_wan_gguf promoted to non-static** — Made non-static and declared in wan-internal.hpp so wan-api.cpp can call it for metadata validation without duplicating the GGUF parsing logic.

3. **Dynamic prefix detection** — T5 prefix (`cond_stage_model.` or `text_encoders.t5xxl.`) and CLIP prefix (`cond_stage_model.visual.` or `clip_vision_model.`) are detected by scanning tensor_storage_map at load time, matching the research pitfall documented in the plan.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] ODR violation: DiT:: functions multiply defined when wan_loader.cpp included runner headers**
- **Found during:** Task 1 build verification
- **Issue:** Initial implementation placed WanModel::load in wan_loader.cpp with full runner header includes, causing the same ODR duplicate symbol errors as Plan 01.
- **Fix:** Moved WanModel::load to wan-api.cpp (single TU owning all runner headers). wan_loader.cpp retains only WanBackend::create and is_wan_gguf with no runner includes.
- **Files modified:** src/api/wan-api.cpp, src/api/wan_loader.cpp, include/wan-cpp/wan-internal.hpp
- **Commit:** e1a045e

**2. [Rule 1 - Bug] WanModel struct not declared — wan-api.cpp could not call Wan::WanModel::load**
- **Found during:** Task 1 (first build attempt)
- **Issue:** WanModel struct was added to wan-internal.hpp after WanModelLoadResult was defined, but the initial ordering placed WanModel before WanModelLoadResult causing a forward reference error.
- **Fix:** Reordered declarations: WanModelLoadResult first, then WanModel struct, then is_wan_gguf declaration.
- **Files modified:** include/wan-cpp/wan-internal.hpp
- **Commit:** e1a045e

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
| commit e1a045e | FOUND |
| init_from_file_and_convert_name in wan-api.cpp | FOUND |
| WAN::WanRunner in wan-api.cpp | FOUND |
| WAN::WanVAERunner in wan-api.cpp | FOUND |
| T5Embedder in wan-api.cpp | FOUND |
| CLIPVisionModelProjectionRunner in wan-api.cpp | FOUND |
| load_tensors (4 calls) in wan-api.cpp | FOUND |
| gguf_init_from_file NOT in weight-loading path | CONFIRMED |
