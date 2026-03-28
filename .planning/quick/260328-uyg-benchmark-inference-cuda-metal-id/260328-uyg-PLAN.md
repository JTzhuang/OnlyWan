---
quick_task: 260328-uyg
title: Fix benchmark_inference compilation errors for CUDA backend
date: 2026-03-28
---

# Quick Task 260328-uyg: Fix benchmark_inference Compilation Errors

## Objective

Fix compilation errors in `benchmark_inference.cpp` when building with CUDA support enabled (`WAN_CUDA=ON`).

## Issues Fixed

1. **Missing CUDA/Metal headers**: The code uses `ggml_backend_cuda_init()` and `ggml_backend_metal_init()` but doesn't include the necessary headers.
2. **Missing device ID parameter**: `ggml_backend_cuda_init()` requires a device ID parameter (0 for first GPU).

## Tasks

### Task 1: Add CUDA and Metal backend headers
- Include `ggml-cuda.h` when `WAN_USE_CUDA` is defined
- Include `ggml-metal.h` when `WAN_USE_METAL` is defined
- Conditionally include headers to avoid compilation errors when backends are disabled

### Task 2: Fix CUDA device initialization
- Update `ggml_backend_cuda_init()` call to pass device ID parameter (0)
- Ensures proper GPU device selection

## Success Criteria

- [x] benchmark_inference.cpp compiles without errors with `-DWAN_CUDA=ON`
- [x] CUDA backend is properly detected and initialized
- [x] Tool correctly logs "[Backend] Selected: CUDA" when `--backend cuda` is used
- [x] CUDA devices are detected and reported during initialization
