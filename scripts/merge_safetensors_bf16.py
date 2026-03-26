import argparse
import sys
from pathlib import Path

import torch
from safetensors.torch import load_file, save_file


def merge_safetensors_to_bf16(input_files, output_file):
    merged = {}
    key_sources = {}

    for input_path in input_files:
        tensors = load_file(str(input_path), device="cpu")
        for key, tensor in tensors.items():
            if key in merged:
                previous_source = key_sources[key]
                raise ValueError(
                    f"Duplicate key collision for '{key}': '{previous_source}' and '{input_path}'"
                )

            if tensor.is_floating_point():
                tensor = tensor.to(torch.bfloat16)

            merged[key] = tensor
            key_sources[key] = str(input_path)

    save_file(merged, str(output_file))


def parse_args():
    parser = argparse.ArgumentParser(
        description="Merge multiple .safetensors files into one output with floating tensors converted to bfloat16"
    )
    parser.add_argument(
        "inputs",
        nargs="+",
        help="Input .safetensors files to merge",
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Path to output merged .safetensors file",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    input_files = [Path(path) for path in args.inputs]
    output_file = Path(args.output)

    try:
        merge_safetensors_to_bf16(input_files, output_file)
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
