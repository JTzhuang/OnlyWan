---
phase: 14-cuda-graph
plan: 01
subsystem: performance-optimization
tags: [cuda-graph, flash-attention, buffer-persistence, optimization]
dependency_graph:
  requires: []
  provides: [CG-01, OP-01, CG-02]
  affects: [ggml_extend, wan-api, cmake-config]
tech_stack:
  added: []
  patterns: [graph-caching, auto-feature-detection]
key_files:
  created: []
  modified:
    - src/ggml_extend.hpp
    - src/api/wan-api.cpp
    - CMakeLists.txt
decisions:
  - "Graph structure stability optimization via cached_graph pointer"
  - "Flash attention auto-enabled for non-CPU backends via ggml_backend_is_cpu check"
  - "GGML_CUDA_USE_GRAPHS controlled by WAN_CUDA_GRAPHS CMake option (default ON)"
metrics:
  duration_seconds: 681
  duration_minutes: 11
  completed_date: "2026-03-17T09:45:45Z"
  tasks_completed: 2
  files_modified: 3
  commits: 1
---

# Phase 14 Plan 01: Buffer Persistence + Flash Attention + CUDA Graph Summary

**One-liner:** Implemented graph structure caching to skip redundant rebuilds, auto-enabled Flash Attention for GPU backends, and added GGML_CUDA_USE_GRAPHS compile flag

## Overview

Successfully implemented three independent optimizations that form the foundation for CUDA graph acceleration:

1. **CG-01**: Graph structure stability optimization — skip redundant graph rebuilds when buffer is persistent
2. **OP-01**: Flash Attention auto-enable — automatically enable for non-CPU backends
3. **CG-02**: GGML_CUDA_USE_GRAPHS compile flag — enable CUDA graph capture/replay in GGML

## Tasks Completed

### Task 1: CG-01 + OP-01 — Buffer Persistence + Flash Attention Auto-Enable

**Commit:** 73b6355

**Changes:**

1. **src/ggml_extend.hpp** — Added graph structure stability optimization:
   - Added member variables: `bool graph_structure_stable` and `struct ggml_cgraph* cached_graph`
   - Added method: `void set_graph_structure_stable(bool stable)`
   - Modified `compute()`: Skip `alloc_compute_buffer`, `reset_compute_ctx`, `get_compute_graph`, and `ggml_gallocr_alloc_graph` when `graph_structure_stable && compute_allocr != nullptr && cached_graph != nullptr`
   - Cache graph pointer after first successful compute when buffer is persistent
   - Clear `cached_graph` in `free_compute_buffer()`

2. **src/api/wan-api.cpp** — Auto-enabled Flash Attention for non-CPU backends:
   - After backend creation in `wan_load_model()`, check `!ggml_backend_is_cpu(backend)`
   - Call `set_flash_attention_enabled(true)` on `wan_runner`, `vae_runner`, and `clip_runner`
   - Added log message: "Auto-enabled flash attention for non-CPU backend"
   - T5Embedder skipped (doesn't inherit GGMLRunner flash_attn interface)

3. **CMakeLists.txt** — Added GGML_CUDA_USE_GRAPHS compile flag:
   - Added CMake option: `WAN_CUDA_GRAPHS` (default ON)
   - Added `add_definitions(-DGGML_CUDA_USE_GRAPHS)` in WAN_CUDA, WAN_HIPBLAS, and WAN_MUSA blocks
   - Added log messages for each backend when CUDA graphs enabled

**Verification:**
- ✅ Compilation successful with CUDA backend
- ✅ CMake output shows "CUDA graph capture/replay enabled"
- ✅ Code inspection confirms all three optimizations implemented
- ✅ Binary built successfully (wan-cli: 36MB, wan-convert: 2.2MB)

## Deviations from Plan

None — plan executed exactly as written.

## Technical Details

### CG-01: Graph Structure Stability

The optimization works by caching the `ggml_cgraph*` pointer after the first successful compute call. On subsequent calls, if:
- `graph_structure_stable == true`
- `compute_allocr != nullptr` (buffer exists)
- `cached_graph != nullptr` (graph cached)

Then skip the expensive operations:
- `alloc_compute_buffer()` — already allocated
- `reset_compute_ctx()` — context preserved
- `get_compute_graph()` — use cached graph
- `ggml_gallocr_alloc_graph()` — already allocated

Only `copy_data_to_backend_tensor()` and `ggml_backend_graph_compute()` are executed, eliminating redundant graph construction overhead.

### OP-01: Flash Attention Auto-Enable

Detection logic uses `ggml_backend_is_cpu()` to identify GPU backends. When false, Flash Attention is automatically enabled for all runners that support it (WanRunner, WanVAERunner, CLIPVisionModelProjectionRunner). This eliminates manual configuration and ensures optimal performance on GPU backends.

### CG-02: GGML_CUDA_USE_GRAPHS

The compile flag enables GGML's built-in CUDA graph capture/replay mechanism. When defined, GGML will automatically capture compute graphs and replay them on subsequent iterations, reducing kernel launch overhead. The flag is controlled by the `WAN_CUDA_GRAPHS` CMake option (default ON), allowing users to disable if needed.

## Files Modified

| File | Lines Changed | Purpose |
|------|---------------|---------|
| src/ggml_extend.hpp | +15 | Graph caching logic, set_graph_structure_stable() |
| src/api/wan-api.cpp | +13 | Flash attention auto-enable |
| CMakeLists.txt | +10 | GGML_CUDA_USE_GRAPHS compile flag |

## Performance Impact

**Expected improvements** (not yet measured):
- **CG-01**: 2-5x speedup in denoising loop (eliminates graph rebuild overhead)
- **OP-01**: 10-20% speedup in attention operations (Flash Attention optimization)
- **CG-02**: 10-30% speedup overall (CUDA graph capture/replay reduces kernel launch overhead)

**Note:** Actual performance gains require:
1. Calling `set_graph_structure_stable(true)` on runners before denoising loop
2. Passing `free_compute_buffer_immediately=false` to `compute()` calls
3. Running on CUDA backend with WAN_CUDA_GRAPHS=ON

## Next Steps

1. **Phase 14 Plan 02**: Implement operator fusion (FUS-02) and additional optimizations (OP-02)
2. **Benchmark**: Measure actual performance improvements with real workloads
3. **Integration**: Update denoising loop to enable graph_structure_stable mode
4. **Documentation**: Add performance tuning guide for users

## Self-Check: PASSED

**Created files:** None (all modifications)

**Modified files:**
- ✅ FOUND: src/ggml_extend.hpp
- ✅ FOUND: src/api/wan-api.cpp
- ✅ FOUND: CMakeLists.txt

**Commits:**
- ✅ FOUND: 73b6355 (feat(14-01): implement buffer persistence and flash attention auto-enable)

**Build artifacts:**
- ✅ FOUND: build/bin/wan-cli (36MB)
- ✅ FOUND: build/bin/wan-convert (2.2MB)
- ✅ FOUND: build/libwan-cpp.a (3.1MB)

All deliverables verified and present.
