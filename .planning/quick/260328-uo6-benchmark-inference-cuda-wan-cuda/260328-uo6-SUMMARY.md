---
phase: quick
plan: 260328-uo6
type: execute
completed_date: 2026-03-28
duration: 2 min
tasks_completed: 1
files_modified: 1
commits:
  - hash: 5fe2de4
    message: "fix(260328-uo6): correct preprocessor defines in backend_from_string()"
---

# Quick Task 260328-uo6: Fix benchmark_inference CUDA Backend Detection

## Summary

Fixed preprocessor define mismatch in `benchmark_inference.cpp` that prevented CUDA backend detection when compiled with `WAN_CUDA=ON`. Changed `SD_USE_CUDA` and `SD_USE_METAL` to `WAN_USE_CUDA` and `WAN_USE_METAL` to align with CMakeLists.txt defines.

## Task Completed

**Task 1: Fix preprocessor conditionals in backend_from_string()**

- Updated line 111: `#ifdef SD_USE_CUDA` → `#ifdef WAN_USE_CUDA`
- Updated line 117: `#ifdef SD_USE_METAL` → `#ifdef WAN_USE_METAL`

This aligns the backend detection logic with the actual preprocessor defines set by CMakeLists.txt (lines 56 and 67).

## Verification

✓ Preprocessor defines corrected: grep confirms `WAN_USE_CUDA` and `WAN_USE_METAL` at lines 111 and 117
✓ No `SD_USE_*` references remain in `backend_from_string()`
✓ Commit: 5fe2de4

## Success Criteria Met

- [x] benchmark_inference.cpp lines 111 and 117 use WAN_USE_CUDA and WAN_USE_METAL
- [x] No SD_USE_* preprocessor checks remain in the file
- [x] Changes committed atomically

## Impact

The benchmark_inference tool will now correctly detect and use CUDA backend when:
1. Compiled with `-DWAN_CUDA=ON`
2. User specifies `--backend cuda` flag

Previously, the tool would always fall back to CPU due to the preprocessor define mismatch.
