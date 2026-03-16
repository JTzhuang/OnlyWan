---
phase: 09-api-fixes-vocab-mmap
plan: 01
subsystem: api
tags: [api, progress-callback, stub-delegation, t2v, i2v]
dependency_graph:
  requires: [08-02]
  provides: [FIX-01, FIX-02]
  affects: [src/api/wan_t2v.cpp, src/api/wan_i2v.cpp, src/api/wan-api.cpp]
tech_stack:
  added: []
  patterns: [flat-arg-to-struct delegation, progress callback with abort]
key_files:
  modified:
    - src/api/wan_t2v.cpp
    - src/api/wan_i2v.cpp
    - src/api/wan-api.cpp
decisions:
  - "I2V flat API has no width/height params — use ctx->params or defaults (832x480)"
  - "Abort path in each loop frees all ggml contexts before returning WAN_ERROR_GENERATION_FAILED"
metrics:
  duration: 3 min
  completed: "2026-03-16T16:09:32Z"
  tasks_completed: 3
  files_modified: 3
---

# Phase 9 Plan 01: API Fixes and Progress Callback Summary

Legacy T2V/I2V stubs now delegate to their `_ex` counterparts via `wan_params_t`, and both Euler loops fire `progress_cb` after every step with correct abort handling.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Replace T2V stub with _ex delegation | fb5078d | src/api/wan_t2v.cpp |
| 2 | Replace I2V stub with _ex delegation | 8a76e8a | src/api/wan_i2v.cpp |
| 3 | Wire progress_cb into both Euler loops | c69a255 | src/api/wan-api.cpp |

## Decisions Made

1. I2V flat API has no width/height params — width/height sourced from `ctx->params` if non-zero, else WAN2.2 I2V standard defaults (832x480).
2. Abort path in each Euler loop frees all in-scope ggml contexts before returning `WAN_ERROR_GENERATION_FAILED` — T2V frees `denoise_ctx` + `output_ctx`; I2V frees `denoise_ctx` + `img_enc_ctx` + `output_ctx`.

## Deviations from Plan

None — plan executed exactly as written.

## Verification Results

- `WAN_ERROR_UNSUPPORTED_OPERATION` absent from both stub files
- `grep -c "progress_cb" wan-api.cpp` = 4 (two null-checks + two invocations)
- `WAN_ERROR_GENERATION_FAILED` appears at lines 525 and 803 (one abort path per loop)
- `cmake --build build --target wan-cpp` exits 0, zero errors, zero warnings

## Self-Check: PASSED

- FOUND: src/api/wan_t2v.cpp
- FOUND: src/api/wan_i2v.cpp
- FOUND: src/api/wan-api.cpp
- FOUND: .planning/phases/09-api-fixes-vocab-mmap/09-01-SUMMARY.md
- FOUND: fb5078d feat(09-01): replace T2V stub with _ex delegation
- FOUND: 8a76e8a feat(09-01): replace I2V stub with _ex delegation
- FOUND: c69a255 feat(09-01): wire progress_cb into both Euler loops in wan-api.cpp
