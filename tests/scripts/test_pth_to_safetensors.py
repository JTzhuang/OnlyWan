import torch
import os
import subprocess
from safetensors.torch import load_file
import pytest

def test_conversion_basic():
    input_pth = "test_model.pth"
    output_st = "test_model.safetensors"

    # Create mock weights
    weights = {"layer1.weight": torch.randn(10, 10)}
    torch.save(weights, input_pth)

    try:
        # Run conversion script (expected to fail as script is not yet created)
        subprocess.run(["python3", "scripts/pth_to_safetensors.py", input_pth, output_st, "--dtype", "fp32"], check=True)

        # Verify results
        loaded = load_file(output_st)
        assert torch.allclose(weights["layer1.weight"], loaded["layer1.weight"])
    finally:
        if os.path.exists(input_pth): os.remove(input_pth)
        if os.path.exists(output_st): os.remove(output_st)

def test_conversion_dtype():
    input_pth = "test_dtype.pth"
    output_st = "test_dtype.safetensors"
    weights = {"w": torch.randn(5, 5).to(torch.float32)}
    torch.save(weights, input_pth)

    try:
        subprocess.run(["python3", "scripts/pth_to_safetensors.py", input_pth, output_st, "--dtype", "fp16"], check=True)
        loaded = load_file(output_st)
        assert loaded["w"].dtype == torch.float16
    finally:
        if os.path.exists(input_pth): os.remove(input_pth)
        if os.path.exists(output_st): os.remove(output_st)

if __name__ == "__main__":
    test_conversion_basic()
