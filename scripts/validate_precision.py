#!/usr/bin/env python3
"""
Precision Validation Script for Multi-GPU Inference

Compares single-GPU vs multi-GPU output to ensure numerical consistency.
Threshold: < 0.01% deviation (default).

Usage:
    python3 validate_precision.py --cli ./build_cuda/bin/wan-cli --model MODEL_PATH --steps 3
"""

import argparse
import subprocess
import sys
import os
import tempfile
from typing import Tuple


def parse_args():
    parser = argparse.ArgumentParser(description='Validate multi-GPU precision')
    parser.add_argument('--cli', required=True, help='Path to wan-cli executable')
    parser.add_argument('--model', required=True, help='Path to model file')
    parser.add_argument('--prompt', default='A cat playing', help='Text prompt')
    parser.add_argument('--steps', type=int, default=5, help='Number of sampling steps')
    parser.add_argument('--seed', type=int, default=42, help='Random seed')
    parser.add_argument('--num-gpus', type=int, default=2, help='Number of GPUs for multi-GPU test')
    parser.add_argument('--gpu-ids', help='Comma-separated GPU IDs (e.g., 0,1)')
    parser.add_argument('--threshold', type=float, default=0.01, help='Max deviation percentage (default: 0.01)')
    parser.add_argument('--verbose', action='store_true', help='Verbose output')
    return parser.parse_args()


def run_inference(cli_path: str, model_path: str, prompt: str, steps: int, seed: int,
                  output_path: str, num_gpus: int = None, gpu_ids: str = None,
                  verbose: bool = False) -> Tuple[bool, str]:
    """Run inference and return success status and error message"""

    cmd = [
        cli_path,
        '--model', model_path,
        '--prompt', prompt,
        '--steps', str(steps),
        '--seed', str(seed),
        '--output', output_path,
        '--backend', 'cuda'
    ]

    if gpu_ids:
        cmd.extend(['--gpu-ids', gpu_ids])
    elif num_gpus and num_gpus > 1:
        cmd.extend(['--num-gpus', str(num_gpus)])

    if verbose:
        print(f"Running: {' '.join(cmd)}", file=sys.stderr)

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=300
        )

        if result.returncode != 0:
            return False, result.stderr or "Inference failed"

        return True, ""

    except subprocess.TimeoutExpired:
        return False, "Inference timed out"
    except Exception as e:
        return False, str(e)


def compare_files(file_a: str, file_b: str) -> Tuple[bool, float, int, int]:
    """
    Compare two files byte-by-byte

    Returns:
        (identical, deviation_pct, diff_bytes, total_bytes)
    """
    try:
        with open(file_a, 'rb') as fa, open(file_b, 'rb') as fb:
            data_a = fa.read()
            data_b = fb.read()

        if len(data_a) != len(data_b):
            size_diff = abs(len(data_a) - len(data_b))
            total = max(len(data_a), len(data_b))
            deviation = (size_diff / total) * 100 if total > 0 else 100.0
            return False, deviation, size_diff, total

        if data_a == data_b:
            return True, 0.0, 0, len(data_a)

        # Count differing bytes
        diff_count = sum(1 for a, b in zip(data_a, data_b) if a != b)
        total = len(data_a)
        deviation = (diff_count / total) * 100 if total > 0 else 0.0

        return False, deviation, diff_count, total

    except Exception as e:
        print(f"Error comparing files: {e}", file=sys.stderr)
        return False, 100.0, 0, 0


def main():
    args = parse_args()

    with tempfile.TemporaryDirectory() as tmpdir:
        single_gpu_output = os.path.join(tmpdir, 'single_gpu.avi')
        multi_gpu_output = os.path.join(tmpdir, 'multi_gpu.avi')

        # Run single-GPU inference
        print(f"Running single-GPU inference (seed={args.seed}, steps={args.steps})...")
        success, error = run_inference(
            args.cli, args.model, args.prompt, args.steps, args.seed,
            single_gpu_output, num_gpus=1, verbose=args.verbose
        )

        if not success:
            print(f"FAIL: Single-GPU inference failed: {error}", file=sys.stderr)
            sys.exit(1)

        if not os.path.exists(single_gpu_output):
            print(f"FAIL: Single-GPU output file not created", file=sys.stderr)
            sys.exit(1)

        # Run multi-GPU inference
        print(f"Running {args.num_gpus}-GPU inference (seed={args.seed}, steps={args.steps})...")
        success, error = run_inference(
            args.cli, args.model, args.prompt, args.steps, args.seed,
            multi_gpu_output, num_gpus=args.num_gpus, gpu_ids=args.gpu_ids,
            verbose=args.verbose
        )

        if not success:
            print(f"FAIL: Multi-GPU inference failed: {error}", file=sys.stderr)
            sys.exit(1)

        if not os.path.exists(multi_gpu_output):
            print(f"FAIL: Multi-GPU output file not created", file=sys.stderr)
            sys.exit(1)

        # Compare outputs
        print("Comparing outputs...")
        identical, deviation, diff_bytes, total_bytes = compare_files(
            single_gpu_output, multi_gpu_output
        )

        if identical:
            print("PASS: Outputs are byte-identical")
            sys.exit(0)

        print(f"Deviation: {deviation:.6f}% ({diff_bytes}/{total_bytes} bytes differ)")

        if deviation < args.threshold:
            print(f"PASS: Deviation {deviation:.6f}% < threshold {args.threshold}%")
            sys.exit(0)
        else:
            print(f"FAIL: Deviation {deviation:.6f}% >= threshold {args.threshold}%", file=sys.stderr)
            sys.exit(1)


if __name__ == '__main__':
    main()
