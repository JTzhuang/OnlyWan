import argparse
import torch
from safetensors.torch import save_file

def main():
    parser = argparse.ArgumentParser(description="Convert .pth to .safetensors")
    parser.add_argument("input", help="Path to input .pth file")
    parser.add_argument("output", help="Path to output .safetensors file")
    parser.add_argument("--dtype", choices=["fp16", "bf16", "fp32"], default="fp32")
    args = parser.parse_args()

    dtype_map = {
        "fp16": torch.float16,
        "bf16": torch.bfloat16,
        "fp32": torch.float32
    }
    target_dtype = dtype_map[args.dtype]

    print(f"Loading {args.input}...")
    state_dict = torch.load(args.input, map_location="cpu", weights_only=True)

    # If it's a full checkpoint, extract state_dict
    if "state_dict" in state_dict:
        state_dict = state_dict["state_dict"]

    converted_dict = {}
    for k, v in state_dict.items():
        if isinstance(v, torch.Tensor):
            if v.is_floating_point():
                v = v.to(target_dtype)
            converted_dict[k] = v

    print(f"Saving to {args.output}...")
    save_file(converted_dict, args.output)

if __name__ == "__main__":
    main()
