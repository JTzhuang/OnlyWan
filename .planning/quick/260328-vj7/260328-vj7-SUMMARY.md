---
phase: quick
plan: 260328-vj7
subsystem: benchmark
tags: [benchmark, inference, model-loading]
duration: 5m
completed_date: 2026-03-28
key_files:
  - tests/cpp/benchmark_inference.cpp
decisions: []
---

# Quick Task 260328-vj7: Add --model-path Parameter to benchmark_inference

## Summary
Added `--model-path` parameter support to benchmark_inference tool, enabling users to load real model weights from file for complete inference benchmarking instead of just compute graph construction.

## Implementation

### Changes Made
1. **Function Signatures Updated**: Added `const std::string& model_path = ""` parameter to all 4 benchmark functions:
   - `benchmark_clip()`
   - `benchmark_t5()`
   - `benchmark_vae()`
   - `benchmark_transformer()`

2. **Weight Loading Logic**: Each benchmark function now:
   - Checks if `model_path` is not empty
   - Calls `runner->load_from_file(model_path)` if path provided
   - Falls back to `alloc_params_buffer()` for empty weights (backward compatible)
   - Includes error handling for file loading failures

3. **Command-Line Parsing**:
   - Added `std::string model_path = "";` variable in main()
   - Added parsing for `--model-path PATH` argument
   - Updated help text to document the new option

4. **Function Calls Updated**: All benchmark function calls in main() now pass the `model_path` parameter

### Key Features
- Backward compatible: works without `--model-path` (uses empty weights)
- Proper error handling: throws exception if file loading fails
- Consistent across all 4 model types
- Help text clearly documents the new option

## Files Modified
- `tests/cpp/benchmark_inference.cpp` — Added model_path parameter support throughout

## Commits
- `59118e7` — feat(quick-260328-vj7): add --model-path parameter to benchmark_inference tool

## Deviations from Plan
None - plan executed exactly as written.

## Self-Check: PASSED
- File exists: `/data/zhongwang2/jtzhuang/projects/OnlyWan/tests/cpp/benchmark_inference.cpp`
- Commit exists: `59118e7`
- All 4 benchmark functions updated with model_path parameter
- Command-line parsing implemented
- Help text updated
- Backward compatibility maintained
