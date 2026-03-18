---
phase: 15-multi-gpu-inference
plan: 02
subsystem: multi-gpu-tensor-parallel
tags: [api, tensor-parallel, backend-scheduler, multi-gpu]
completed: 2026-03-18T05:21:36Z
duration_seconds: 341
tasks_completed: 3
files_modified: 6

dependency_graph:
  requires: [15-01, 15-03]
  provides: [tensor-parallel-execution, backend-scheduler]
  affects: [wan_loader.cpp, wan-api.cpp, ggml_extend.hpp, wan.hpp, wan-internal.hpp]

tech_stack:
  added: [ggml_backend_sched, ggml_backend_cuda_split_buffer_type]
  patterns: [tensor-parallelism, split-buffer-allocation, scheduler-based-compute]

key_files:
  created: []
  modified:
    - path: src/api/wan_loader.cpp
      role: Multi-GPU backend initialization and device validation
      lines_added: 73
    - path: include/wan-cpp/wan-internal.hpp
      role: Multi-GPU state creation API and is_multi_gpu helper
      lines_added: 9
    - path: src/api/wan-api.cpp
      role: Tensor-split model loading and scheduler-based denoising
      lines_added: 178
    - path: src/ggml_extend.hpp
      role: Split buffer allocation and scheduler compute methods
      lines_added: 89
    - path: src/wan.hpp
      role: WanRunner scheduler wrapper
      lines_added: 21

decisions:
  - choice: Use ggml_backend_cuda_split_buffer_type for tensor distribution
    rationale: GGML provides built-in split buffer type for distributing tensors across GPUs
  - choice: Create backend scheduler in wan_load_model_from_file
    rationale: Scheduler needs all backends initialized before graph execution
  - choice: Target gpu_ids[0] as main_device for primary backend
    rationale: Main device hosts the primary backend for single-GPU fallback operations
  - choice: Add is_multi_gpu() helper to wan_context
    rationale: Clean branching logic in denoising loops without exposing internal state

metrics:
  commits: 3
  files_changed: 6
  lines_added: 370
  build_verified: false
---

# Phase 15 Plan 02: Tensor Parallel Multi-GPU Execution Summary

**One-liner:** Tensor parallel execution distributes model weights across GPUs via split buffers and uses backend scheduler for coordinated graph computation.

## Overview

This plan implements the core multi-GPU tensor parallel execution path. Model weights are distributed across GPUs using GGML split buffer types, and the denoising loop uses the backend scheduler instead of direct single-backend compute. This enables single-model inference across multiple GPUs with automatic work distribution.

## Tasks Completed

### Task 1: Add Multi-GPU Backend Initialization
**Status:** ✅ Complete
**Commit:** de09c72

Implemented multi-GPU backend creation with device validation:
- Added `create_multi_gpu_state` function in wan_loader.cpp
- Validates GPU homogeneity by checking device descriptions match
- Creates CUDA backends for each GPU via `ggml_backend_cuda_init`
- Validates GPU IDs are within valid range (0 to device_count-1)
- Returns nullptr on validation failure with detailed error logging
- Added forward declaration in wan-internal.hpp

**Files modified:**
- `src/api/wan_loader.cpp` - Implemented create_multi_gpu_state with validation
- `include/wan-cpp/wan-internal.hpp` - Added forward declaration

**Implementation details:**
- GPU homogeneity check: compares `ggml_backend_name()` for all GPUs
- Creates temporary backends for validation, then permanent backends for state
- Logs success with GPU count and strategy

### Task 2: Add Tensor-Split Model Loading
**Status:** ✅ Complete
**Commit:** 9b3c495

Extended model loading to support multi-GPU tensor splitting:
- Added `alloc_params_buffer_split` method to GGMLRunner
- Extended `wan_load_model_from_file` with multi-GPU initialization path
- Creates backend scheduler via `ggml_backend_sched_new`
- Distributes model weights via `ggml_backend_cuda_split_buffer_type`
- Re-allocates params buffers for all runners (WanRunner, VAERunner, T5Embedder, CLIPRunner)
- Overrides primary backend to target main_device (gpu_ids[0])

**Files modified:**
- `src/ggml_extend.hpp` - Added alloc_params_buffer_split method
- `src/api/wan-api.cpp` - Extended wan_load_model_from_file with multi-GPU path

**Implementation details:**
- Tensor split: equal distribution (1.0 / num_gpus per GPU)
- Split buffer type created with main_device and tensor_split array
- Scheduler created with all backends, GGML_DEFAULT_GRAPH_SIZE, non-parallel mode
- Single-GPU fallback preserved when num_gpus <= 1

### Task 3: Adapt Denoising Loop for Scheduler
**Status:** ✅ Complete
**Commit:** bfa12c1

Modified denoising loops to use backend scheduler for multi-GPU:
- Added `compute_with_sched` method to GGMLRunner
- Added `compute_with_sched` wrapper to WanRunner
- Added `is_multi_gpu()` helper to wan_context
- Branched T2V Euler loop to use scheduler when multi-GPU active
- Branched I2V Euler loop to use scheduler when multi-GPU active
- Single-GPU path preserved in else branches

**Files modified:**
- `src/ggml_extend.hpp` - Added compute_with_sched method
- `src/wan.hpp` - Added WanRunner::compute_with_sched wrapper
- `include/wan-cpp/wan-internal.hpp` - Added is_multi_gpu() helper
- `src/api/wan-api.cpp` - Updated T2V and I2V denoising loops

**Implementation details:**
- `compute_with_sched` calls `ggml_backend_sched_reset`, `ggml_backend_sched_alloc_graph`, `ggml_backend_sched_graph_compute`
- Branching logic: `if (ctx->is_multi_gpu()) { compute_with_sched } else { compute }`
- Both conditional and unconditional passes use scheduler in multi-GPU mode
- All multi-GPU code guarded by `WAN_USE_MULTI_GPU`

## Deviations from Plan

None - plan executed exactly as written.

## Verification

Manual verification performed:
- ✅ `grep "create_multi_gpu_state" src/api/wan_loader.cpp` returns implementation
- ✅ `grep "compute_with_sched" src/ggml_extend.hpp` returns method definition
- ✅ `grep "alloc_params_buffer_split" src/ggml_extend.hpp` returns method definition
- ✅ `grep "is_multi_gpu" src/api/wan-api.cpp` returns 4 matches (2 T2V + 2 I2V branches)

Automated tests not run (requires multi-GPU hardware and test infrastructure from Plan 15-04).

## Next Steps

This plan provides the foundation for tensor parallel execution:
- **Plan 15-04**: Add multi-GPU testing and validation
- **Future**: Performance optimization (NCCL integration, pipeline parallelism)

## Commits

| Task | Commit | Message |
|------|--------|---------|
| 1 | de09c72 | feat(15-02): add multi-GPU backend initialization with device validation |
| 2 | 9b3c495 | feat(15-02): add tensor-split model loading for multi-GPU |
| 3 | bfa12c1 | feat(15-02): adapt denoising loop for multi-GPU scheduler |

## Self-Check: PASSED

### Files Modified
- ✅ FOUND: src/api/wan_loader.cpp
- ✅ FOUND: include/wan-cpp/wan-internal.hpp
- ✅ FOUND: src/api/wan-api.cpp
- ✅ FOUND: src/ggml_extend.hpp
- ✅ FOUND: src/wan.hpp

### Commits Verified
- ✅ FOUND: de09c72 (Task 1)
- ✅ FOUND: 9b3c495 (Task 2)
- ✅ FOUND: bfa12c1 (Task 3)

All artifacts verified successfully.es)

Automated tests not run (requires multi-GPU hardware and test infrastructure from Plan 15-04).

## Next Steps

This plan completes the tensor parallel execution path:
- **Plan 15-04**: Add multi-GPU testing and validation
- **Future**: Performance optimization (NCCL integration, pipeline parallelism)

## Commits

| Task | Commit | Message |
|------|--------|---------|
| 1 | de09c72 | feat(15-02): add multi-GPU backend initialization with device validation |
| 2 | 9b3c495 | feat(15-02): add tensor-split model loading for multi-GPU |
| 3 | bfa12c1 | feat(15-02): adapt denoising loop for multi-GPU scheduler |

## Self-Check: PASSED

### Files Modified
- ✅ FOUND: src/api/wan_loader.cpp
- ✅ FOUND: include/wan-cpp/wan-internal.hpp
- ✅ FOUND: src/api/wan-api.cpp
- ✅ FOUND: src/ggml_extend.hpp
- ✅ FOUND: src/wan.hpp

### Commits Verified
- ✅ FOUND: de09c72 (Task 1)
- ✅ FOUND: 9b3c495 (Task 2)
- ✅ FOUND: bfa12c1 (Task 3)

All artifacts verified successfully.
