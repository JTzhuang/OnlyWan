---
status: testing
phase: 06-fix-duplicate-symbols
source: [06-fix-duplicate-symbols-01-SUMMARY.md]
started: 2026-03-16T03:30:00Z
updated: 2026-03-16T03:30:00Z
---

## Current Test

number: 1
name: Library links without duplicate symbol errors
expected: |
  Run: cd /path/to/wan && cmake -B build && cmake --build build
  The build completes with 0 "multiple definition" errors.
  nm build/libwan-cpp.a | grep " T wan_params_create" | wc -l should return 1.
awaiting: user response

## Tests

### 1. Library links without duplicate symbol errors
expected: Build completes with 0 "multiple definition" linker errors. nm shows each wan_params_* symbol appears exactly once.
result: [pending]

### 2. wan_params_* defined in exactly one translation unit
expected: grep -c "wan_params_create" src/api/wan-api.cpp returns 0 (no definitions in wan-api.cpp). grep -c "WAN_API wan_params_create" src/api/wan_config.cpp returns 1.
result: [pending]

### 3. CMakeLists.txt uses explicit source list
expected: grep "GLOB_RECURSE" CMakeLists.txt returns nothing. grep "WAN_LIB_SOURCES" CMakeLists.txt shows an explicit set() list.
result: [pending]

### 4. avi_writer.h include guard fixed
expected: grep "AVI_WRITER_H" examples/cli/avi_writer.h shows the guard defined as AVI_WRITER_H (no leading __ or dot character).
result: [pending]

## Summary

total: 4
passed: 0
issues: 0
pending: 4
skipped: 0

## Gaps

[none yet]
