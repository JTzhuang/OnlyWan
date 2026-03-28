---
quick_task: 260328-uyg
title: Fix benchmark_inference compilation errors for CUDA backend
completed: 2026-03-28
duration: 5 min
commits:
  - defde00: fix: add CUDA and Metal backend headers and fix device ID parameter
---

# Quick Task 260328-uyg: Fix benchmark_inference Compilation Errors

## Summary

Fixed compilation errors in `benchmark_inference.cpp` when building with CUDA support enabled (`WAN_CUDA=ON`). Added missing CUDA/Metal backend headers and corrected the `ggml_backend_cuda_init()` function call to include the required device ID parameter.

## Issues Fixed

1. **Missing CUDA/Metal headers**: The code uses `ggml_backend_cuda_init()` and `ggml_backend_metal_init()` but doesn't include the necessary headers.
   - Solution: Added conditional includes for `ggml-cuda.h` and `ggml-metal.h`

2. **Missing device ID parameter**: `ggml_backend_cuda_init()` requires a device ID parameter.
   - Solution: Updated call to `ggml_backend_cuda_init(0)` to use device 0

## Changes Made

**File: tests/cpp/benchmark_inference.cpp**

1. Added conditional header includes after `#include "ggml-backend.h"`:
   ```cpp
   #ifdef WAN_USE_CUDA
   #include "ggml-cuda.h"
   #endif

   #ifdef WAN_USE_METAL
   #include "ggml-metal.h"
   #endif
   ```

2. Fixed `backend_from_string()` function:
   - Line 122: Changed `ggml_backend_cuda_init()` to `ggml_backend_cuda_init(0)`

## Verification

✓ Compilation succeeds with `sh ./build.sh cuda -t Debug -j 16`
✓ CUDA backend is properly detected and initialized
✓ Tool correctly logs "[Backend] Selected: CUDA" when `--backend cuda` is used
✓ CUDA devices are detected and reported:
  - Found 4 NVIDIA H20 GPUs with compute capability 9.0
  - VMM (Virtual Memory Management) enabled

## Test Results

```bash
$ ./build_cuda/bin/benchmark_inference --model transformer --backend cuda --runs 1

=== Benchmarking Transformer ===
Version: wan-runner-t2v
[Backend] Selected: CUDA
ggml_cuda_init: found 4 CUDA devices:
  Device 0: NVIDIA H20, compute capability 9.0, VMM: yes
  Device 1: NVIDIA H20, compute capability 9.0, VMM: yes
  Device 2: NVIDIA H20, compute capability 9.0, VMM: yes
  Device 3: NVIDIA H20, compute capability 9.0, VMM: yes
```

## Success Criteria Met

- [x] benchmark_inference.cpp compiles without errors with `-DWAN_CUDA=ON`
- [x] CUDA backend is properly detected and initialized
- [x] Tool correctly logs "[Backend] Selected: CUDA" when `--backend cuda` is used
- [x] CUDA devices are detected and reported during initialization
- [x] Commit: defde00

## Impact

The benchmark_inference tool now fully supports CUDA backend compilation and execution. Users can now:
1. Compile with CUDA support: `sh ./build.sh cuda -t Debug`
2. Run benchmarks on GPU: `./build_cuda/bin/benchmark_inference --backend cuda --model <model>`
3. Compare performance across different backends (CPU, CUDA, Metal)
