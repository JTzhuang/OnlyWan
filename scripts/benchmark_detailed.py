#!/usr/bin/env python3
"""
Detailed Performance Benchmark Framework

Provides configuration loading, CLI argument parsing, and parameter override system
for comprehensive performance benchmarking.

Usage:
    python3 benchmark_detailed.py --config configs/benchmark_default.yaml
    python3 benchmark_detailed.py --config configs/benchmark_default.yaml --resolution 1024x1024 --steps 50
"""

import argparse
import json
import csv
import subprocess
import time
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Any
from dataclasses import dataclass
from datetime import datetime
import itertools

# Try to import yaml
try:
    import yaml
    HAS_YAML = True
except ImportError:
    HAS_YAML = False


@dataclass
class ResolutionConfig:
    """Configuration for image resolution in WxH format"""
    width: int
    height: int

    @staticmethod
    def parse(resolution_str: str) -> 'ResolutionConfig':
        """
        Parse resolution string in WxH format.

        Args:
            resolution_str: Resolution string like "512x512" or "1024x576"

        Returns:
            ResolutionConfig with parsed width and height

        Raises:
            ValueError: If format is invalid
        """
        match = re.match(r'^(\d+)x(\d+)$', resolution_str.strip())
        if not match:
            raise ValueError(
                f"Invalid resolution format: '{resolution_str}'. "
                f"Expected format: WxH (e.g., 512x512)"
            )

        width = int(match.group(1))
        height = int(match.group(2))

        if width <= 0 or height <= 0:
            raise ValueError(
                f"Resolution dimensions must be positive: {width}x{height}"
            )

        return ResolutionConfig(width=width, height=height)

    def __str__(self) -> str:
        return f"{self.width}x{self.height}"


def load_config_file(config_path: str) -> Dict[str, Any]:
    """
    Load configuration from YAML file.

    Args:
        config_path: Path to YAML configuration file

    Returns:
        Dictionary containing configuration

    Raises:
        ImportError: If PyYAML is not installed
        FileNotFoundError: If config file does not exist
        yaml.YAMLError: If YAML parsing fails
    """
    if not HAS_YAML:
        raise ImportError(
            "PyYAML is required to load config files. "
            "Install it with: pip install pyyaml"
        )

    config_file = Path(config_path)
    if not config_file.exists():
        raise FileNotFoundError(f"Config file not found: {config_path}")

    try:
        with open(config_file, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)

        if config is None:
            config = {}

        return config
    except yaml.YAMLError as e:
        raise yaml.YAMLError(f"Failed to parse YAML config: {e}")


def parse_args() -> argparse.Namespace:
    """
    Parse command-line arguments.

    Returns:
        Parsed arguments namespace
    """
    parser = argparse.ArgumentParser(
        description='Detailed Performance Benchmark Framework',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Run with default config
  python3 benchmark_detailed.py --config configs/benchmark_default.yaml

  # Override resolution and steps
  python3 benchmark_detailed.py --config configs/benchmark_default.yaml \\
    --resolution 1024x1024 --steps 50

  # Run without config file (CLI-only mode)
  python3 benchmark_detailed.py --model model.safetensors --steps 20 \\
    --resolution 512x512 --num-frames 16

  # Output results as JSON
  python3 benchmark_detailed.py --config configs/benchmark_default.yaml \\
    --json --output results.json
        """
    )

    # Configuration
    parser.add_argument(
        '--config',
        type=str,
        help='Path to YAML configuration file'
    )

    # Model and input
    parser.add_argument(
        '--model',
        type=str,
        help='Path to model file (safetensors or checkpoint)'
    )
    parser.add_argument(
        '--prompt',
        type=str,
        default='A cat playing with a ball',
        help='Text prompt for generation (default: %(default)s)'
    )

    # Generation parameters
    parser.add_argument(
        '--resolution',
        type=str,
        help='Resolution in WxH format (e.g., 512x512, 1024x576)'
    )
    parser.add_argument(
        '--steps',
        type=int,
        help='Number of sampling steps'
    )
    parser.add_argument(
        '--num-frames',
        type=int,
        help='Number of frames to generate'
    )

    # GPU configuration
    parser.add_argument(
        '--num-gpus',
        type=int,
        help='Number of GPUs to use'
    )
    parser.add_argument(
        '--gpu-ids',
        type=str,
        help='Comma-separated GPU IDs (e.g., 0,1,2)'
    )

    # Execution options
    parser.add_argument(
        '--timeout',
        type=int,
        default=600,
        help='Timeout per test in seconds (default: %(default)s)'
    )
    parser.add_argument(
        '--retry',
        type=int,
        default=0,
        help='Number of retries on failure (default: %(default)s)'
    )

    # Output options
    parser.add_argument(
        '--output',
        type=str,
        help='Output file path for results'
    )
    parser.add_argument(
        '--json',
        action='store_true',
        help='Output results as JSON'
    )
    parser.add_argument(
        '--csv',
        action='store_true',
        help='Output results as CSV'
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Verbose output'
    )

    return parser.parse_args()


def merge_config_with_args(
    config: Dict[str, Any],
    args: argparse.Namespace
) -> Dict[str, Any]:
    """
    Merge configuration file with CLI arguments.

    CLI arguments take precedence over config file values.

    Args:
        config: Configuration dictionary from file
        args: Parsed command-line arguments

    Returns:
        Merged configuration dictionary
    """
    merged = config.copy() if config else {}

    # Flatten nested config for easier merging
    if 'test_cases' not in merged:
        merged['test_cases'] = [{}]

    test_case = merged['test_cases'][0] if merged['test_cases'] else {}

    # Override with CLI arguments (only if provided)
    if args.model:
        merged['model'] = args.model

    if args.prompt:
        test_case['prompt'] = args.prompt

    if args.resolution:
        if 'resolutions' not in test_case:
            test_case['resolutions'] = []
        test_case['resolutions'] = [args.resolution]

    if args.steps:
        if 'steps' not in test_case:
            test_case['steps'] = []
        test_case['steps'] = [args.steps]

    if args.num_frames:
        test_case['num_frames'] = args.num_frames

    if args.num_gpus:
        if 'gpu_config' not in merged:
            merged['gpu_config'] = {}
        merged['gpu_config']['configurations'] = [args.num_gpus]

    if args.gpu_ids:
        if 'gpu_config' not in merged:
            merged['gpu_config'] = {}
        merged['gpu_config']['prefer_gpus'] = [
            int(x.strip()) for x in args.gpu_ids.split(',')
        ]

    if args.timeout:
        if 'execution' not in merged:
            merged['execution'] = {}
        merged['execution']['timeout_per_test'] = args.timeout

    if args.retry:
        if 'execution' not in merged:
            merged['execution'] = {}
        merged['execution']['retry_on_failure'] = args.retry

    if args.output:
        if 'output' not in merged:
            merged['output'] = {}
        merged['output']['save_path'] = args.output

    # Handle output formats
    output_formats = []
    if args.json:
        output_formats.append('json')
    if args.csv:
        output_formats.append('csv')

    if output_formats:
        if 'output' not in merged:
            merged['output'] = {}
        merged['output']['format'] = output_formats

    # Store test case back
    if merged['test_cases']:
        merged['test_cases'][0] = test_case

    return merged


def main():
    """Main entry point"""
    args = parse_args()

    # Load config file if provided
    config = {}
    if args.config:
        try:
            config = load_config_file(args.config)
            if args.verbose:
                print(f"Loaded config from: {args.config}", file=sys.stderr)
        except Exception as e:
            print(f"Error loading config: {e}", file=sys.stderr)
            sys.exit(1)

    # Merge config with CLI arguments
    merged_config = merge_config_with_args(config, args)

    if args.verbose:
        print("Merged configuration:", file=sys.stderr)
        print(json.dumps(merged_config, indent=2, default=str), file=sys.stderr)

    # Output merged config
    if args.json:
        print(json.dumps(merged_config, indent=2, default=str))
    else:
        print("Configuration loaded successfully")
        if args.model:
            print(f"  Model: {args.model}")
        if args.resolution:
            print(f"  Resolution: {args.resolution}")
        if args.steps:
            print(f"  Steps: {args.steps}")


if __name__ == '__main__':
    main()
