---
phase: 19-i-o-npy-pt-goal-npy-pt-python
plan: 01
subsystem: io
tags: [npy, ggml, tensor-io, python-interop, testing]
dependency_graph:
  requires: []
  provides: [npy-io-utilities, libnpy-thirdparty, test-io-npy]
  affects: [future-tests-needing-tensor-io]
tech_stack:
  added:
    - libnpy (header-only, thirdparty/libnpy/npy.hpp, 340 lines)
  patterns:
    - Header-only C++ library via CMake INTERFACE target
    - NumPy row-major <-> ggml dimension reversal (ne[0]=last axis)
    - ggml_context owning tensors allocated with fixed-size static buffer
key_files:
  created:
    - thirdparty/libnpy/npy.hpp
    - tests/cpp/test_io_utils.hpp
    - tests/cpp/test_io_npy.cpp
    - tests/cpp/test_data/README.md
    - tests/cpp/test_data/.gitignore
    - tests/cpp/test_data/generate_test_data.py
  modified:
    - thirdparty/CMakeLists.txt
    - tests/cpp/CMakeLists.txt
decisions:
  - Used npy::DType enum (not string) for dtype matching in load_npy/save_npy
  - Reversed ggml dimension order vs NumPy (ggml ne[0]=innermost=last numpy axis)
  - libnpy exposed as CMake INTERFACE target so tests link cleanly
  - Generated .npy test data excluded from VCS via .gitignore
metrics:
  duration: 12 minutes
  completed: 2026-03-27
  tasks_completed: 5
  files_created: 6
  files_modified: 2
  tests_passing: 10
---

# Phase 19 Plan 01: .npy File I/O Library Integration Summary

**One-liner:** Header-only libnpy integrated with ggml_tensor load/save via DType enum mapping and reversed dimension ordering.

## What Was Built

### Task 1 - libnpy header-only library (90b2dc2)

Added `thirdparty/libnpy/npy.hpp` (340 lines): a complete, self-contained NumPy
`.npy` file reader/writer supporting float32, float16, int32, and int64 dtypes.
Exposed as a CMake `INTERFACE` target named `libnpy` in `thirdparty/CMakeLists.txt`.

Key API:
```cpp
npy::NpyArray arr = npy::load("file.npy");  // arr.data, arr.shape, arr.dtype
npy::save("out.npy", data_ptr, shape, npy::DType::FLOAT32);
```

### Task 2 - test_io_utils.hpp (224e95c)

`tests/cpp/test_io_utils.hpp` exports `load_npy()` and `save_npy()` bridging
ggml tensors and `.npy` files:

```cpp
// Load .npy file into a ggml_tensor owned by ctx
struct ggml_tensor* t = load_npy(ctx, "data.npy");

// Save a ggml_tensor to .npy file
save_npy("out.npy", tensor);
```

**Dimension mapping strategy:** NumPy stores arrays in row-major (C) order with
shape `(d0, d1, ..., dn)`. ggml's `ne[0]` is the innermost (fastest) dimension,
equivalent to NumPy's last axis. Therefore:
- `ggml ne[0]` = `numpy_shape[-1]` (last axis)
- `ggml ne[1]` = `numpy_shape[-2]`
- etc.

This means a NumPy `(rows, cols)` matrix becomes ggml `{cols, rows}` — no data
copy or transposition required since both use row-major layout.

### Task 3 - test_io_npy.cpp (4995f04)

10 tests covering:
- 1D roundtrip: F32, I32, I64
- 2D roundtrip: F32 (shape preserved), F16
- 3D roundtrip: F32
- Error handling: nonexistent file throws, null tensor throws
- Property checks: nelements after load, nbytes for F16

All 10 tests pass:
```
=== test_io_npy ===
Passed: 10 / 10
```

### Tasks 4 & 5 - test_data directory and generate_test_data.py (d4c5a92)

`tests/cpp/test_data/generate_test_data.py` generates reference `.npy` files:
- `ref_1d_f32.npy`, `ref_1d_f16.npy`, `ref_1d_i32.npy`, `ref_1d_i64.npy`
- `ref_2d_f32.npy`, `ref_3d_f32.npy`
- `ref_known_values.npy` (deterministic values for assertion testing)
- `example_from_pt.npy` (PyTorch .pt → .npy conversion example)

Generated files are `.gitignore`-excluded. README.md documents the full
Python interop workflow.

## Python Interop Workflow

```bash
# Step 1: generate reference data
cd tests/cpp/test_data && python3 generate_test_data.py

# Step 2: C++ loads reference .npy and validates
./build/bin/test_io_npy

# Step 3: C++ saves tensors; Python validates
import numpy as np
arr = np.load('tests/cpp/test_data/output.npy')
print(arr.shape, arr.dtype)
```

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed DType enum vs string mismatch in test_io_utils.hpp**
- **Found during:** Task 3 build (compile error)
- **Issue:** Initial `test_io_utils.hpp` used `const std::string& dtype = arr.dtype` but `arr.dtype` is `npy::DType` enum, not `std::string`. Also `save_npy` called `npy::save()` with wrong argument order.
- **Fix:** Rewrote load/save to use `switch(arr.dtype)` enum comparison and `npy::save(path, tensor->data, shape, dtype_enum)` with correct signature.
- **Files modified:** `tests/cpp/test_io_utils.hpp`
- **Commit:** 4995f04 (included in Task 3 commit)

## Known Stubs

None. All data paths are wired: load reads real `.npy` bytes into real ggml tensor
buffers; save writes real tensor bytes to real `.npy` files. Python script generates
actual NumPy arrays.

## Usage Examples for Future Test Development

```cpp
// In any test file:
#include "test_io_utils.hpp"

// Allocate a small ggml context
ggml_init_params params = { .mem_size = 4*1024*1024, .mem_buffer = nullptr };
struct ggml_context* ctx = ggml_init(params);

// Load reference tensor from Python-generated .npy
struct ggml_tensor* ref = load_npy(ctx, "tests/cpp/test_data/ref_2d_f32.npy");
// ref->ne[0] = cols, ref->ne[1] = rows  (ggml dimension order)
float* data = (float*)ref->data;

// Save C++ output for Python validation
save_npy("/tmp/cpp_output.npy", output_tensor);
// Then in Python: np.testing.assert_allclose(np.load('/tmp/cpp_output.npy'), expected)

ggml_free(ctx);
```

## Self-Check: PASSED

| Item | Status |
|------|--------|
| thirdparty/libnpy/npy.hpp | FOUND |
| tests/cpp/test_io_utils.hpp | FOUND |
| tests/cpp/test_io_npy.cpp | FOUND |
| tests/cpp/test_data/README.md | FOUND |
| tests/cpp/test_data/generate_test_data.py | FOUND |
| tests/cpp/test_data/.gitignore | FOUND |
| thirdparty/CMakeLists.txt | FOUND |
| tests/cpp/CMakeLists.txt | FOUND |
| Commit 90b2dc2 (libnpy) | FOUND |
| Commit 224e95c (test_io_utils.hpp) | FOUND |
| Commit 4995f04 (test_io_npy.cpp) | FOUND |
| Commit d4c5a92 (test_data) | FOUND |
| Tests passing | 10/10 |
