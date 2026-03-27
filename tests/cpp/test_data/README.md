# tests/cpp/test_data

This directory holds reference `.npy` files used by C++ I/O tests and the
Python interop workflow.

## Contents

| File | Description | Generator |
|------|-------------|----------|
| *(generated)* | `.npy` test fixtures produced by `generate_test_data.py` | `python generate_test_data.py` |

Generated files are **not committed** to the repository (see `.gitignore`).
Run the generation script once before running tests that need pre-saved `.npy` files.

## Generating Test Data

```bash
cd tests/cpp/test_data
python generate_test_data.py
```

Requires Python >= 3.8 and NumPy:

```bash
pip install numpy
```

## File Format

All files use the `.npy` format (NumPy 1.0 / 2.0, little-endian, C order).
They can be inspected in Python:

```python
import numpy as np
arr = np.load('some_file.npy')
print(arr.shape, arr.dtype, arr)
```

## Python Interop Workflow

The typical workflow for validating C++ output against Python/NumPy:

1. **C++ saves a tensor** to `.npy` via `save_npy()` in `test_io_utils.hpp`
2. **Python loads it** with `np.load()` and computes the reference result
3. **Python saves the reference** as `.npy`
4. **C++ loads the reference** via `load_npy()` and asserts values match

This pattern is used in `test_io_npy.cpp` roundtrip tests and can be extended
for validating model outputs (e.g., loading PyTorch `.pt` checkpoints converted
to `.npy` via `generate_test_data.py`).

## Converting PyTorch .pt → .npy

See `generate_test_data.py` for an example using `torch.load()` +
`tensor.numpy()` + `np.save()` to convert PyTorch tensors to `.npy`.

## .gitignore

Generated `.npy` files in this directory are excluded from version control.
Only the generator script and this README are committed.
