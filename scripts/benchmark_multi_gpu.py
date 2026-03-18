#!/usr/bin/env python3
"""
Multi-GPU Performance Benchmark Script

Measures step latency, memory usage, and communication bandwidth
for multi-GPU inference configurations.

Usage:
    python3 benchmark_multi_gpu.py --cli ./build_cuda/bin/wan-cli --model MODEL_PATH --steps 5 --json
"""

import argparse
import json
import subprocess
import time
import re
import sys
from typing import Dict, List, Optional


def parse_args():
    parser = argparse.ArgumentParser(description='Benchmark multi-GPU inference performance')
    parser.add_argument('--cli', required=True, help='Path to wan-cli executable')
    parser.add_argument('--model', required=True, help='Path to model file')
    parser.add_argument('--prompt', default='A cat playing with a ball', help='Text prompt')
    parser.add_argument('--steps', type=int, default=10, help='Number of sampling steps')
    parser.add_argument('--num-gpus', type=int, default=2, help='Number of GPUs to use')
    parser.add_argument('--gpu-ids', help='Comma-separated GPU IDs (e.g., 0,1)')
    parser.add_argument('--output', default='/tmp/benchmark_output.avi', help='Output video path')
    parser.add_argument('--json', action='store_true', help='Output results as JSON')
    parser.add_argument('--verbose', action='store_true', help='Verbose output')
    return parser.parse_args()


def run_inference(cli_path: str, model_path: str, prompt: str, steps: int,
                  num_gpus: Optional[int], gpu_ids: Optional[str],
                  output_path: str, verbose: bool) -> Dict:
    """Run inference and capture timing metrics"""

    cmd = [
        cli_path,
        '--model', model_path,
        '--prompt', prompt,
        '--steps', str(steps),
        '--output', output_path,
        '--backend', 'cuda'
    ]

    if gpu_ids:
        cmd.extend(['--gpu-ids', gpu_ids])
    elif num_gpus:
        cmd.extend(['--num-gpus', str(num_gpus)])

    if verbose:
        print(f"Running command: {' '.join(cmd)}", file=sys.stderr)

    start_time = time.time()

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout
        )

        end_time = time.time()
        total_time = end_time - start_time

        if result.returncode != 0:
            return {
                'success': False,
                'error': result.stderr,
                'total_time': total_time
            }

        # Parse output for step timing
        step_times = []
        step_pattern = re.compile(r'Step (\d+)/\d+ \([\d.]+%\)')

        lines = result.stdout.split('\n') + result.stderr.split('\n')
        for line in lines:
            if step_pattern.search(line):
                step_times.append(time.time())

        # Calculate metrics
        step_latency = total_time / steps if steps > 0 else 0

        return {
            'success': True,
            'total_time': total_time,
            'steps': steps,
            'step_latency': step_latency,
            'throughput': steps / total_time if total_time > 0 else 0,
            'stdout': result.stdout if verbose else '',
            'stderr': result.stderr if verbose else ''
        }

    except subprocess.TimeoutExpired:
        return {
            'success': False,
            'error': 'Inference timed out after 300 seconds'
        }
    except Exception as e:
        return {
            'success': False,
            'error': str(e)
        }


def get_gpu_memory_usage(gpu_ids: Optional[str]) -> Dict[int, float]:
    """Query GPU memory usage using nvidia-smi"""
    try:
        result = subprocess.run(
            ['nvidia-smi', '--query-gpu=index,memory.used', '--format=csv,noheader,nounits'],
            capture_output=True,
            text=True,
            timeout=5
        )

        if result.returncode != 0:
            return {}

        memory_usage = {}
        for line in result.stdout.strip().split('\n'):
            if line:
                parts = line.split(',')
                if len(parts) == 2:
                    gpu_id = int(parts[0].strip())
                    memory_mb = float(parts[1].strip())
                    memory_usage[gpu_id] = memory_mb

        return memory_usage

    except Exception:
        return {}


def estimate_comm_bandwidth(total_time: float, num_gpus: int, steps: int) -> float:
    """
    Estimate inter-GPU communication bandwidth

    This is a rough estimate based on timing differences.
    For accurate measurement, would need NCCL profiling or CUDA events.
    """
    if num_gpus <= 1 or total_time <= 0:
        return 0.0

    # Rough estimate: assume 10% of time is communication overhead
    comm_time = total_time * 0.1

    # Estimate data transfer size (very rough)
    # Assume ~100MB per step per GPU pair
    estimated_data_mb = 100 * steps * (num_gpus - 1)

    bandwidth_mbps = estimated_data_mb / comm_time if comm_time > 0 else 0

    return bandwidth_mbps


def main():
    args = parse_args()

    # Run inference
    if args.verbose:
        print("Starting benchmark...", file=sys.stderr)

    result = run_inference(
        args.cli,
        args.model,
        args.prompt,
        args.steps,
        args.num_gpus,
        args.gpu_ids,
        args.output,
        args.verbose
    )

    if not result['success']:
        print(f"Benchmark failed: {result.get('error', 'Unknown error')}", file=sys.stderr)
        sys.exit(1)

    # Get memory usage
    memory_usage = get_gpu_memory_usage(args.gpu_ids)

    # Calculate communication bandwidth estimate
    num_gpus = len(args.gpu_ids.split(',')) if args.gpu_ids else args.num_gpus
    comm_bandwidth = estimate_comm_bandwidth(result['total_time'], num_gpus, args.steps)

    # Prepare output
    output = {
        'config': {
            'model': args.model,
            'prompt': args.prompt,
            'steps': args.steps,
            'num_gpus': num_gpus,
            'gpu_ids': args.gpu_ids
        },
        'metrics': {
            'total_time': result['total_time'],
            'step_latency': result['step_latency'],
            'throughput': result['throughput'],
            'memory_usage': memory_usage,
            'comm_bandwidth_mbps': comm_bandwidth
        }
    }

    if args.json:
        print(json.dumps(output, indent=2))
    else:
        print("\n=== Multi-GPU Benchmark Results ===")
        print(f"Configuration:")
        print(f"  Model: {args.model}")
        print(f"  GPUs: {num_gpus} ({args.gpu_ids or 'auto'})")
        print(f"  Steps: {args.steps}")
        print(f"\nPerformance:")
        print(f"  Total Time: {result['total_time']:.2f}s")
        print(f"  Step Latency: {result['step_latency']:.3f}s/step")
        print(f"  Throughput: {result['throughput']:.2f} steps/s")
        print(f"  Est. Comm Bandwidth: {comm_bandwidth:.1f} MB/s")

        if memory_usage:
            print(f"\nMemory Usage:")
            for gpu_id, mem_mb in sorted(memory_usage.items()):
                print(f"  GPU {gpu_id}: {mem_mb:.0f} MB")


if __name__ == '__main__':
    main()
