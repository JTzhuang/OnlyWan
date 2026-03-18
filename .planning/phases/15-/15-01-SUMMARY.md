---
phase: 15-multi-gpu-inference
plan: 01
subsystem: multi-gpu-api
tags: [api, types, cmake, nccl, multi-gpu]
completed: 2026-03-18T13:07:15Z
duration_seconds: 892
tasks_completed: 3
files_modified: 3

dependency_graph:
  requires: [15-00]
  provides: [multi-gpu-types, nccl-integration]
  affects: [wan.h, wan-internal.hpp, CMakeLists.txt]

tech_stack:
  added: [NCCL, ggml-backend-sched]
  patterns: [conditional-compilation, multi-backend-management]

key_files:
  created: []
  modified:
    - path: include/wan-cpp/wan.h
      role: Multi-GPU public API types
      lines_added: 20
    - path: include/wan-cpp/wan-internal.hpp
      role: Multi-GPU internal context
      lines_added: 42
    - path: CMakeLists.txt
      role: NCCL CMake integration
      lines_added: 41

decisions:
  - choice: Use ggml_backend_sched_t for multi-GPU scheduling
    rationale: GGML provides built-in backend scheduler for multi-device coordination
  - choice: Guard multi-GPU code with WAN_USE_MULTI_GPU
    rationale: Ensures single-GPU builds remain unaffected and compile without CUDA
  - choice: Make NCCL optional with fallback warning
    rationale: Allows multi-GPU builds without NCCL, library will use fallback communication

metrics:
  commits: 3
  files_changed: 3
  lines_added: 103
  build_verified: true
---

# Phase 15 Plan 01: Multi-GPU API Foundation Summary

**One-liner:** Established multi-GPU type contracts with distribution strategy enum, extended internal context with backend scheduler, and integrated NCCL build system support.

## Overview

This plan established the foundational types and build system infrastructure for multi-GPU inference support. Added public API types for multi-GPU configuration, extended internal context to manage multiple backends, and integrated NCCL library detection into the CMake build system.

## Tasks Completed

### Task 1: Add Multi-GPU Types to Public API
**Status:** ✅ Complete
**Commit:** e3cd2dd

Added multi-GPU types to `include/wan-cpp/wan.h`:
- Added `WAN_ERROR_GPU_FAILURE` error code for GPU-specific failures
- Added `wan_distribution_strategy_t` enum with 5 strategies:
  - `WAN_DISTRIBUTION_DATA_PARALLEL` - replicate model, split batch
  - `WAN_DISTRIBUTION_TENSOR_PARALLEL` - split model layers across GPUs
  - `WAN_DISTRIBUTION_PIPELINE_PARALLEL` - assign layers to different GPUs
  - `WAN_DISTRIBUTION_HYBRID` - combine multiple strategies
  - `WAN_DISTRIBUTION_AUTO` - let library choose optimal strategy
- Extended `wan_params_t` with three new fields:
  - `const int* gpu_ids` - array of GPU device IDs
  - `int num_gpus` - number of GPUs to use
  - `wan_distribution_strategy_t distribution_strategy` - distribution strategy

### Task 2: Extend Internal Context with MultiGPUState
**Status:** ✅ Complete
**Commit:** 9bb6494

Extended `include/wan-cpp/wan-internal.hpp` with multi-GPU internal structures:
- Added conditional include for `ggml-backend-sched.h` under `WAN_USE_MULTI_GPU` guard
- Extended `WanParams` struct with multi-GPU fields matching public API
- Added `MultiGPUState` struct containing:
  - `std::vector<ggml_backend_t> backends` - vector of GPU backends
  - `ggml_backend_sched_t scheduler` - GGML backend scheduler for coordination
  - `std::vector<int> gpu_ids` - GPU device IDs
  - `wan_distribution_strategy_t strategy` - selected distribution strategy
  - `bool initialized` - initialization state flag
- Added `std::unique_ptr<MultiGPUState> multi_gpu_state` to `wan_context` under guard
- Implemented proper RAII cleanup in `MultiGPUState` destructor

### Task 3: Add CMake NCCL Integration
**Status:** ✅ Complete
**Commit:** f396f71

Added NCCL detection and linking to `CMakeLists.txt`:
- Added `WAN_NCCL` CMake option for enabling NCCL support
- Added `WAN_USE_MULTI_GPU` definition when `WAN_CUDA` is enabled
- Implemented NCCL library detection with standard search paths:
  - `/usr/lib`, `/usr/local/lib`, `/usr/lib/x86_64-linux-gnu`
  - `$ENV{NCCL_ROOT_DIR}/lib`, `$ENV{CUDA_PATH}/lib64`
- Added NCCL include directory detection
- Added conditional `WAN_USE_NCCL` definition when NCCL is found
- Added conditional NCCL linking to wan-cpp target
- Added warning when NCCL requested but not found (fallback mode)
- Added warning when WAN_NCCL specified without WAN_CUDA
- Verified single-GPU build (WAN_CUDA=OFF) still compiles successfully
- Verified multi-GPU build (WAN_CUDA=ON) configures and compiles with new types

## Verification Results

### Build Verification
- ✅ Single-GPU build (WAN_CUDA=OFF) compiles successfully
- ✅ Multi-GPU build (WAN_CUDA=ON) configures successfully
- ✅ WAN_USE_MULTI_GPU definition propagates to compilation
- ✅ NCCL detection logic works correctly
- ✅ No compilation errors in modified headers

### Type Contract Verification
- ✅ `wan_distribution_strategy_t` enum defined with 5 values
- ✅ `wan_params_t` contains `gpu_ids`, `num_gpus`, `distribution_strategy` fields
- ✅ `WAN_ERROR_GPU_FAILURE` error code exists
- ✅ `MultiGPUState` contains `backends` vector and `scheduler`
- ✅ `wan_context` contains `multi_gpu_state` under guard
- ✅ CMake `WAN_NCCL` option exists and conditionally links NCCL

## Deviations from Plan

None - plan executed exactly as written.

## Technical Notes

### Design Decisions

1. **Conditional Compilation Guards**: All multi-GPU code is guarded by `WAN_USE_MULTI_GPU`, which is automatically defined when `WAN_CUDA` is enabled. This ensures:
   - Single-GPU builds remain unaffected
   - No multi-GPU overhead for CPU/single-GPU users
   - Clean separation of concerns

2. **GGML Backend Scheduler**: Using `ggml_backend_sched_t` from GGML's backend scheduler provides:
   - Built-in multi-device coordination
   - Automatic memory management across devices
   - Tensor placement optimization
   - Reduced implementation complexity

3. **Optional NCCL**: NCCL is optional with fallback warning because:
   - Not all systems have NCCL installed
   - Basic multi-GPU can work without NCCL (using CUDA peer-to-peer)
   - NCCL provides optimized collective operations but isn't strictly required

### Implementation Details

- `MultiGPUState` uses RAII pattern for automatic cleanup
- Backend vector stores raw `ggml_backend_t` pointers managed by GGML
- Scheduler is freed before backends to maintain proper cleanup order
- GPU IDs are stored in both `MultiGPUState` and `WanParams` for different access patterns

### Build System Integration

- NCCL detection searches standard system paths and environment variables
- `WAN_USE_MULTI_GPU` is automatically defined with CUDA (no manual flag needed)
- `WAN_USE_NCCL` is only defined when NCCL library is actually found
- Build system provides clear warnings for misconfiguration

## Next Steps

This plan establishes the type contracts that subsequent plans will implement:
- **Plan 15-02**: Implement multi-GPU context initialization
- **Plan 15-03**: Implement backend scheduler integration
- **Plan 15-04**: Implement distribution strategies
- **Plan 15-05**: Add multi-GPU testing and validation

## Commits

| Task | Commit | Message |
|------|--------|---------|
| 1 | e3cd2dd | feat(15-01): add multi-GPU types to public API |
| 2 | 9bb6494 | feat(15-01): extend internal context with MultiGPUState |
| 3 | f396f71 | feat(15-01): add CMake NCCL integration |

## Self-Check: PASSED

### Files Created
All files exist as expected (no new files created, only modifications).

### Files Modified
- ✅ FOUND: include/wan-cpp/wan.h
- ✅ FOUND: include/wan-cpp/wan-internal.hpp
- ✅ FOUND: CMakeLists.txt

### Commits Verified
- ✅ FOUND: e3cd2dd (Task 1)
- ✅ FOUND: 9bb6494 (Task 2)
- ✅ FOUND: f396f71 (Task 3)

All artifacts verified successfully.
