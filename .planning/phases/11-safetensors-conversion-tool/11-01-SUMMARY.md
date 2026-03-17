---
phase: 11-safetensors-conversion-tool
plan: "01"
subsystem: conversion-tool
tags: [conversion, gguf, safetensors, cli]
dependency_graph:
  requires: [10-01]
  provides: [wan-convert-binary, save_to_gguf_file-metadata-overload]
  affects: [examples/convert, src/model.h, src/model.cpp]
tech_stack:
  added: []
  patterns: [ModelLoader::save_to_gguf_file overload, gguf_set_val_str metadata injection]
key_files:
  created:
    - examples/convert/main.cpp
    - examples/convert/CMakeLists.txt
  modified:
    - src/model.h
    - src/model.cpp
    - examples/CMakeLists.txt
decisions:
  - "4-argument save_to_gguf_file overload: backward-compatible addition; original 3-arg unchanged"
  - "SUBMODEL_META map in main.cpp: maps --type string to arch/version strings matching is_wan_gguf() expectations"
  - "convert_tensors_name() called before save: normalises HF tensor names to internal WAN names"
metrics:
  duration: "6 min"
  completed: "2026-03-17T03:03:56Z"
  tasks_completed: 2
  files_changed: 5
---

# Phase 11 Plan 01: Safetensors Conversion Tool Summary

wan-convert CLI tool that converts WAN2.1/2.2 HuggingFace safetensors sub-model files into GGUF files loadable by wan_load_model, with metadata injection via a new save_to_gguf_file overload.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Extend save_to_gguf_file with metadata map | 50edbd7 | src/model.h, src/model.cpp |
| 2 | Create wan-convert CLI and wire CMake | edef8c0 | examples/convert/main.cpp, examples/convert/CMakeLists.txt, examples/CMakeLists.txt |

## Decisions Made

1. 4-argument `save_to_gguf_file` overload: backward-compatible addition; original 3-arg overload unchanged.
2. `SUBMODEL_META` map in main.cpp maps `--type` string to `arch`/`version` strings matching `is_wan_gguf()` key expectations (`general.model_type`, `general.architecture`, `general.wan.version`).
3. `convert_tensors_name()` called before save to normalise HF tensor names to internal WAN names.

## Verification Results

- `cmake --build build --target wan-convert` exits 0
- `./build/bin/wan-convert --help` exits 0, prints all flags and all 6 sub-model types
- `./build/bin/wan-convert` (no args) exits 1, stderr contains required-args error
- `./build/bin/wan-convert --input x --output y --type bad` exits 1, prints valid types

## Deviations from Plan

None - plan executed exactly as written.

## Self-Check: PASSED

- [x] examples/convert/main.cpp exists
- [x] examples/convert/CMakeLists.txt exists
- [x] examples/CMakeLists.txt contains add_subdirectory(convert)
- [x] src/model.h declares 4-argument save_to_gguf_file overload
- [x] src/model.cpp implements metadata injection via gguf_set_val_str
- [x] Commit 50edbd7 exists (Task 1)
- [x] Commit edef8c0 exists (Task 2)
