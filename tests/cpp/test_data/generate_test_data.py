#!/usr/bin/env python3
"""generate_test_data.py - Generate reference .npy test files for C++ I/O tests.

This script creates .npy files that can be loaded by load_npy() in
tests/cpp/test_io_utils.hpp and verified by the C++ test suite.

It also demonstrates the PyTorch .pt -> .npy conversion workflow for
using real model weights as test fixtures.

Usage:
    cd tests/cpp/test_data
    python generate_test_data.py

Requirements:
    pip install numpy
    pip install torch  # optional, only for .pt conversion examples

Generated files (all in C order, little-endian):
    ref_1d_f32.npy     - 1D float32 array [0.0, 1.0, 2.0, 3.0, 4.0]
    ref_2d_f32.npy     - 2D float32 matrix shape (3, 4)
    ref_3d_f32.npy     - 3D float32 tensor shape (2, 3, 4)
    ref_1d_f16.npy     - 1D float16 array
    ref_1d_i32.npy     - 1D int32 array
    ref_1d_i64.npy     - 1D int64 array
    ref_known_values.npy - 2D float32 with known values for bit-exact checks
"""

import os
import sys
import numpy as np


OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))


def save(name: str, arr: np.ndarray) -> None:
    """Save array to .npy file and print summary."""
    path = os.path.join(OUTPUT_DIR, name)
    np.save(path, arr)
    print(f"  Saved {name}: shape={arr.shape} dtype={arr.dtype} nbytes={arr.nbytes}")


def generate_basic_fixtures() -> None:
    """Generate basic reference fixtures covering all supported dtypes."""
    print("Generating basic fixtures...")

    # 1D float32
    arr = np.arange(5, dtype=np.float32)
    save("ref_1d_f32.npy", arr)

    # 2D float32 - shape (3, 4), values 0..11
    arr = np.arange(12, dtype=np.float32).reshape(3, 4)
    save("ref_2d_f32.npy", arr)

    # 3D float32 - shape (2, 3, 4), values 0..23
    arr = np.arange(24, dtype=np.float32).reshape(2, 3, 4)
    save("ref_3d_f32.npy", arr)

    # 1D float16
    arr = np.array([0.0, 0.5, 1.0, 1.5, 2.0], dtype=np.float16)
    save("ref_1d_f16.npy", arr)

    # 1D int32
    arr = np.array([10, 20, 30, 40, 50], dtype=np.int32)
    save("ref_1d_i32.npy", arr)

    # 1D int64
    arr = np.array([100, 200, 300], dtype=np.int64)
    save("ref_1d_i64.npy", arr)

    # 2D float32 with known values for bit-exact assertion in C++
    arr = np.array([
        [1.0, 2.0, 3.0],
        [4.0, 5.0, 6.0],
    ], dtype=np.float32)
    save("ref_known_values.npy", arr)


def generate_pytorch_example() -> None:
    """Example: convert a PyTorch .pt checkpoint tensor to .npy.

    This is the standard workflow for using model weights as test fixtures:
      1. Load the .pt file with torch.load()
      2. Extract the specific tensor by key
      3. Convert to NumPy and save as .npy
      4. Load in C++ with load_npy() for comparison

    Requires: pip install torch
    """
    try:
        import torch
    except ImportError:
        print("  [skip] PyTorch not installed - skipping .pt conversion example")
        print("         Install with: pip install torch")
        return

    print("Generating PyTorch conversion example...")

    # Create a synthetic tensor as a stand-in for a real .pt checkpoint
    t = torch.randn(4, 8, dtype=torch.float32)
    pt_path = os.path.join(OUTPUT_DIR, "example_pt_tensor.pt")
    torch.save(t, pt_path)
    print(f"  Saved example_pt_tensor.pt: shape={tuple(t.shape)}")

    # Convert: .pt -> numpy -> .npy
    # For a real checkpoint with state_dict:
    #   state = torch.load('model.pt', map_location='cpu')
    #   arr = state['weight'].detach().float().numpy()
    arr = t.detach().float().numpy()           # float32
    arr = np.ascontiguousarray(arr)            # ensure C order
    save("example_from_pt.npy", arr)

    # Verify roundtrip
    loaded = np.load(os.path.join(OUTPUT_DIR, "example_from_pt.npy"))
    assert np.allclose(arr, loaded), "Roundtrip mismatch!"
    print("  Verified .pt -> .npy roundtrip OK")


def verify_c_loadable() -> None:
    """Verify all generated .npy files can be re-loaded by NumPy."""
    print("Verifying generated files...")
    expected = [
        ("ref_1d_f32.npy",        (5,),       np.float32),
        ("ref_2d_f32.npy",        (3, 4),     np.float32),
        ("ref_3d_f32.npy",        (2, 3, 4),  np.float32),
        ("ref_1d_f16.npy",        (5,),       np.float16),
        ("ref_1d_i32.npy",        (5,),       np.int32),
        ("ref_1d_i64.npy",        (3,),       np.int64),
        ("ref_known_values.npy",  (2, 3),     np.float32),
    ]
    all_ok = True
    for name, shape, dtype in expected:
        path = os.path.join(OUTPUT_DIR, name)
        arr = np.load(path)
        ok = (arr.shape == shape) and (arr.dtype == dtype)
        status = "OK" if ok else "FAIL"
        print(f"  [{status}] {name}: shape={arr.shape} dtype={arr.dtype}")
        if not ok:
            all_ok = False
    if not all_ok:
        print("FAILED: some files did not match expected shape/dtype")
        sys.exit(1)
    print("All files verified OK")


if __name__ == "__main__":
    print(f"Generating test data in: {OUTPUT_DIR}")
    generate_basic_fixtures()
    generate_pytorch_example()
    verify_c_loadable()
    print("Done.")
