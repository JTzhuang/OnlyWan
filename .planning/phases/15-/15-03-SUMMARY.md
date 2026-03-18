---
phase: 15-multi-gpu-inference
plan: 03
subsystem: multi-gpu-data-parallel
tags: [api, data-parallel, threading, multi-gpu]
completed: 2026-03-18T05:13:30Z
duration_seconds: 167
tasks_completed: 3
files_modified: 4

dependency_graph:
  requires: [15-01]
  provides: [data-parallel-api, batch-generation]
  affects: [wan.h, wan-api.cpp, wan_loader.cpp, wan-internal.hpp]

tech_stack:
  added: [std::thread, round-robin-scheduling]
  patterns: [data-parallelism, per-device-model-loading, concurrent-generation]

key_files:
  created: []
  modified:
    - path: src/api/wan_loader.cpp
      role: Device-targeting backend creation
      lines_added: 14
    - path: include/wan-cpp/wan-internal.hpp
      role: Multi-GPU backend API extension
      lines_added: 3
    - path: include/wan-cpp/wan.h
      role: Batch generation public API
      lines_added: 34
    - path: src/api/wan-api.cpp
      role: Data parallel batch generation implementation
      lines_added: 143

decisions:
  - choice: Use std::thread for concurrent execution
    rationale: Simple, portable threading without external dependencies
  - choice: Round-robin GPU assignment
    rationale: Balanced load distribution across GPUs for batch requests
  - choice: Per-request error reporting via wan_batch_result_t array
    rationale: Allows partial success - some requests can succeed while others fail

metrics:
  commits: 3
  files_changed: 4
  lines_added: 194
  build_verified: false
---

# Phase 15 Plan 03: Data Parallel Batch Generation Summary

**One-liner:** Data parallel batch generation distributes independent requests across GPUs with device-targeted model loading and concurrent threading.

## Overview

This plan implements data parallelism for batch video generation. Each GPU loads a complete model copy and processes independent generation requests concurrently. Requests are distributed across GPUs using round-robin assignment, with per-request error reporting for partial success handling.

## Tasks Completed

### Task 1: Add Device-Targeting Model Loader Helper
**Status:** ✅ Complete
**Commit:** 30c385c

Extended `WanBackend::create` to accept device_id parameter:
- Added `device_id` parameter (default 0) to `WanBackend::create`
- Added `WanBackend::create_on_device` helper for multi-GPU mode
- Updated `ggml_backend_cuda_init(device_id)` to target specific GPU
- Updated wan-api.cpp to pass device_id=0 for backward compatibility
- Guarded multi-GPU helper with `WAN_USE_MULTI_GPU`

**Files modified:**
- `src/api/wan_loader.cpp` - Extended create method, added create_on_device
- `include/wan-cpp/wan-internal.hpp` - Updated WanBackend API declaration
- `src/api/wan-api.cpp` - Updated existing call to pass device_id=0

### Task 2: Declare Batch Generation API
**Status:** ✅ Complete
**Commit:** 1a7309f

Added batch generation API to public header:
- Added `wan_batch_result_t` struct for per-request error reporting
- Added `wan_generate_batch_t2v` function declaration
- API accepts arrays of prompts and output paths
- Requires `gpu_ids` and `num_gpus` in params
- Guarded with `WAN_USE_MULTI_GPU` for backward compatibility

**Files modified:**
- `include/wan-cpp/wan.h` - Added batch generation API section

### Task 3: Implement Data Parallel Batch Generation
**Status:** ✅ Complete
**Commit:** ad1d444

Implemented concurrent batch generation with threading:
- Created `BatchWorkerContext` struct to pass data to worker threads
- Implemented `batch_worker_thread` function:
  - Loads model via `wan_load_model`
  - Overrides backend with `create_on_device` targeting specific GPU
  - Calls `wan_generate_video_t2v_ex` for generation
  - Reports errors in `wan_batch_result_t`
- Implemented `wan_generate_batch_t2v`:
  - Validates input parameters
  - Creates worker contexts with round-robin GPU assignment
  - Launches std::thread per request
  - Joins all threads and aggregates results
  - Returns `WAN_ERROR_GENERATION_FAILED` if any request failed
- Added fallback implementation returning `WAN_ERROR_UNSUPPORTED_OPERATION` for non-multi-GPU builds

**Files modified:**
- `src/api/wan-api.cpp` - Added batch generation implementation

**Implementation details:**
- Round-robin assignment: `gpu_ids[i % num_gpus]`
- Each thread gets independent wan_context_t
- Per-request error messages stored in results array
- Thread-safe: no shared state between workers

## Deviations from Plan

None - plan executed exactly as written.

## Verification

Manual verification performed:
- ✅ `grep "wan_generate_batch_t2v" include/wan-cpp/wan.h` returns declaration
- ✅ `grep "create_on_device" src/api/wan_loader.cpp` returns implementation
- ✅ `grep "std::thread" src/api/wan-api.cpp` confirms threading

Automated tests not run (requires multi-GPU hardware and test infrastructure from Plan 15-04).

## Next Steps

This plan provides the foundation for data parallel execution:
- **Plan 15-02**: Implement multi-GPU context initialization (tensor/pipeline parallel)
- **Plan 15-04**: Add multi-GPU testing and validation
- **Future**: Performance optimization (thread pool, async execution)

## Commits

| Task | Commit | Message |
|------|--------|---------|
| 1 | 30c385c | feat(15-03): add device-targeting model loader helper |
| 2 | 1a7309f | feat(15-03): declare batch generation API |
| 3 | ad1d444 | feat(15-03): implement data parallel batch generation |

## Self-Check: PASSED

### Files Modified
- ✅ FOUND: src/api/wan_loader.cpp
- ✅ FOUND: include/wan-cpp/wan-internal.hpp
- ✅ FOUND: include/wan-cpp/wan.h
- ✅ FOUND: src/api/wan-api.cpp

### Commits Verified
- ✅ FOUND: 30c385c (Task 1)
- ✅ FOUND: 1a7309f (Task 2)
- ✅ FOUND: ad1d444 (Task 3)

All artifacts verified successfully.
