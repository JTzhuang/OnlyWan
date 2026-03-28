---
quick_task: 260328-ucx
title: Add backend selection to benchmark_inference tool
completed: 2026-03-28
duration: 5 min
commits:
  - e90ddfa: feat(260328-ucx): add backend parameter to benchmark_inference tool
files_modified:
  - tests/cpp/benchmark_inference.cpp
---

# Quick Task 260328-ucx: Add Backend Selection to benchmark_inference

## Summary

Added `--backend {cpu|cuda|metal}` parameter to benchmark_inference tool for cross-device performance comparison. Backend selection respects compile-time availability flags (SD_USE_CUDA, SD_USE_METAL) and automatically falls back to CPU if requested backend is unavailable.

## Implementation Details

### Changes Made

1. **Backend Selection Helper Function** (`backend_from_string()`)
   - Parses backend name string and returns appropriate ggml_backend_t
   - Checks compile-time flags for CUDA and Metal support
   - Logs selected backend to stderr for visibility
   - Falls back to CPU if requested backend unavailable

2. **Function Signature Updates**
   - Updated all benchmark functions to accept `backend_name` parameter:
     - `benchmark_clip()`
     - `benchmark_t5()`
     - `benchmark_vae()`
     - `benchmark_transformer()`
   - Each function now calls `backend_from_string(backend_name)` instead of hardcoded `ggml_backend_cpu_init()`

3. **Command-Line Parsing**
   - Added `--backend` parameter to argument parser
   - Default value: "cpu"
   - Updated help text to document new option

4. **Main Function Updates**
   - All benchmark function calls now pass `backend` parameter
   - Backend selection logged during execution

### Verification

- Binary builds without errors
- Help text correctly documents `--backend {cpu|cuda|metal}` option
- Backend parameter is recognized and parsed
- Backend selection is logged to stderr: `[Backend] Selected: CPU`
- All benchmark functions use selected backend
- Default behavior (no --backend flag) uses CPU

### Example Usage

```bash
# Benchmark CLIP on CPU (default)
./build/bin/benchmark_inference --model clip --runs 1

# Benchmark all models on CUDA (if available)
./build/bin/benchmark_inference --backend cuda --runs 1

# Benchmark Transformer on Metal (if available)
./build/bin/benchmark_inference --model transformer --backend metal --runs 1
```

## Success Criteria Met

- [x] benchmark_inference accepts --backend parameter with cpu/cuda/metal values
- [x] Help text documents the new parameter
- [x] Backend selection respects compile-time availability flags
- [x] All benchmark functions use the selected backend
- [x] Tool logs which backend was selected during execution
- [x] Default behavior (no --backend flag) uses CPU
- [x] Binary builds successfully

## Deviations from Plan

None - plan executed exactly as written.
