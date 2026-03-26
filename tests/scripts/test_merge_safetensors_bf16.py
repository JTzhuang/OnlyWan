from pathlib import Path
import subprocess

import pytest
import torch
from safetensors.torch import load_file, save_file


SCRIPT_PATH = Path(__file__).resolve().parents[2] / "scripts" / "merge_safetensors_bf16.py"


def run_merge(inputs, output_path):
    cmd = ["python3", str(SCRIPT_PATH), *[str(p) for p in inputs], "--output", str(output_path)]
    return subprocess.run(cmd, capture_output=True, text=True)


def write_safetensors(path, tensors):
    save_file(tensors, str(path))


def test_merge_two_files_success(tmp_path):
    input_a = tmp_path / "part_a.safetensors"
    input_b = tmp_path / "part_b.safetensors"
    output_file = tmp_path / "merged.safetensors"

    write_safetensors(input_a, {"layer_a.weight": torch.randn(2, 3, dtype=torch.float32)})
    write_safetensors(input_b, {"layer_b.weight": torch.randn(3, 4, dtype=torch.float32)})

    result = run_merge([input_a, input_b], output_file)

    assert result.returncode == 0, result.stderr
    assert output_file.exists()

    merged = load_file(str(output_file))
    assert set(merged.keys()) == {"layer_a.weight", "layer_b.weight"}


def test_floating_tensors_are_converted_to_bf16(tmp_path):
    input_a = tmp_path / "float32.safetensors"
    input_b = tmp_path / "float16.safetensors"
    output_file = tmp_path / "merged_fp.safetensors"

    write_safetensors(input_a, {"f32": torch.randn(4, 4, dtype=torch.float32)})
    write_safetensors(input_b, {"f16": torch.randn(4, 4, dtype=torch.float16)})

    result = run_merge([input_a, input_b], output_file)

    assert result.returncode == 0, result.stderr

    merged = load_file(str(output_file))
    assert merged["f32"].dtype == torch.bfloat16
    assert merged["f16"].dtype == torch.bfloat16


def test_non_floating_tensors_keep_original_dtype(tmp_path):
    input_a = tmp_path / "non_float_a.safetensors"
    input_b = tmp_path / "non_float_b.safetensors"
    output_file = tmp_path / "merged_non_float.safetensors"

    write_safetensors(input_a, {"token_ids": torch.tensor([1, 2, 3], dtype=torch.int64)})
    write_safetensors(input_b, {"mask": torch.tensor([True, False, True], dtype=torch.bool)})

    result = run_merge([input_a, input_b], output_file)

    assert result.returncode == 0, result.stderr

    merged = load_file(str(output_file))
    assert merged["token_ids"].dtype == torch.int64
    assert merged["mask"].dtype == torch.bool


def test_duplicate_key_fails_with_non_zero_exit(tmp_path):
    input_a = tmp_path / "dup_a.safetensors"
    input_b = tmp_path / "dup_b.safetensors"
    output_file = tmp_path / "dup_merged.safetensors"

    write_safetensors(input_a, {"shared.weight": torch.randn(2, 2, dtype=torch.float32)})
    write_safetensors(input_b, {"shared.weight": torch.randn(2, 2, dtype=torch.float16)})

    result = run_merge([input_a, input_b], output_file)

    assert result.returncode != 0
    combined_output = f"{result.stdout}\n{result.stderr}".lower()
    assert "duplicate" in combined_output or "collision" in combined_output
    assert "shared.weight" in combined_output
    assert not output_file.exists()


def test_cli_help_mentions_inputs_and_output():
    result = subprocess.run(
        ["python3", str(SCRIPT_PATH), "--help"],
        capture_output=True,
        text=True,
    )

    assert result.returncode == 0
    assert "inputs" in result.stdout.lower()
    assert "--output" in result.stdout
