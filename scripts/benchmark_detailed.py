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
import csv
import json
import os
import re
import sys
import time
import psutil
from datetime import datetime
from pathlib import Path
from typing import Dict, Any, List, Tuple, Optional
from dataclasses import dataclass, field, asdict
from collections import defaultdict

# Try to import yaml
try:
    import yaml
    HAS_YAML = True
except ImportError:
    HAS_YAML = False

# Try to import torch for GPU detection
try:
    import torch
    HAS_TORCH = True
except ImportError:
    HAS_TORCH = False


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


@dataclass
class BenchmarkResult:
    """Data class for benchmark results"""
    test_id: str
    model_path: str
    resolution: str
    steps: int
    num_gpus: int
    gpu_ids: List[int]
    prompt: str
    timestamp: str
    duration: float  # seconds
    memory_peak: float  # MB
    memory_allocated: float  # MB
    throughput: float  # images/sec
    status: str  # 'success', 'failed', 'timeout'
    error_message: Optional[str] = None
    system_info: Dict[str, Any] = field(default_factory=dict)
    gpu_memory_metrics: Dict[str, Any] = field(default_factory=dict)
    communication_bandwidth: Optional[float] = None  # MB/s

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary"""
        return asdict(self)


def get_available_gpus() -> List[int]:
    """
    Get list of available GPUs.

    Returns:
        List of GPU IDs available on the system
    """
    if not HAS_TORCH:
        return []

    try:
        num_gpus = torch.cuda.device_count()
        return list(range(num_gpus))
    except Exception:
        return []


def validate_gpu_availability(gpu_ids: List[int]) -> bool:
    """
    Validate that specified GPUs are available.

    Args:
        gpu_ids: List of GPU IDs to validate

    Returns:
        True if all GPUs are available, False otherwise
    """
    if not gpu_ids:
        return True

    available = get_available_gpus()
    return all(gpu_id in available for gpu_id in gpu_ids)


def generate_test_combinations(config: Dict[str, Any]) -> List[Dict[str, Any]]:
    """
    Generate all combinations of test parameters from configuration.

    Args:
        config: Configuration dictionary with test parameters

    Returns:
        List of test combinations
    """
    combinations = []

    # Extract test case parameters
    test_cases = config.get('test_cases', [{}])
    if not test_cases:
        test_cases = [{}]

    for test_case in test_cases:
        resolutions = test_case.get('resolutions', ['512x512'])
        steps_list = test_case.get('steps', [20])
        num_frames = test_case.get('num_frames', 1)
        prompt = test_case.get('prompt', 'A cat playing with a ball')

        # GPU configurations
        gpu_config = config.get('gpu_config', {})
        gpu_configs = gpu_config.get('configurations', [1])
        prefer_gpus = gpu_config.get('prefer_gpus', None)

        model_path = config.get('model', 'model.safetensors')

        # Generate all combinations
        for resolution in resolutions:
            for steps in steps_list:
                for num_gpu in gpu_configs:
                    # Determine GPU IDs for this configuration
                    if prefer_gpus and len(prefer_gpus) >= num_gpu:
                        gpu_ids = prefer_gpus[:num_gpu]
                    else:
                        gpu_ids = list(range(num_gpu))

                    # Validate GPU availability
                    if not validate_gpu_availability(gpu_ids):
                        continue

                    combination = {
                        'resolution': resolution,
                        'steps': steps,
                        'num_frames': num_frames,
                        'num_gpus': num_gpu,
                        'gpu_ids': gpu_ids,
                        'prompt': prompt,
                        'model_path': model_path,
                    }
                    combinations.append(combination)

    return combinations


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
    except Exception as e:
        raise ValueError(f"Failed to parse YAML config: {e}")


def save_results_as_json(
    results: List[BenchmarkResult],
    output_path: str
) -> None:
    """
    Save benchmark results as JSON file.

    Args:
        results: List of BenchmarkResult objects
        output_path: Output file path
    """
    try:
        data = {
            'timestamp': datetime.now().isoformat(),
            'total_tests': len(results),
            'successful': sum(1 for r in results if r.status == 'success'),
            'failed': sum(1 for r in results if r.status == 'failed'),
            'results': [r.to_dict() for r in results]
        }

        output_file = Path(output_path)
        output_file.parent.mkdir(parents=True, exist_ok=True)

        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, default=str)

        print(f"Saved JSON results to: {output_file}", file=sys.stderr)
    except Exception as e:
        print(f"Error saving JSON results: {e}", file=sys.stderr)


def save_results_as_csv(
    results: List[BenchmarkResult],
    output_path: str
) -> None:
    """
    Save benchmark results as CSV file.

    Args:
        results: List of BenchmarkResult objects
        output_path: Output file path
    """
    try:
        if not results:
            return

        output_file = Path(output_path)
        output_file.parent.mkdir(parents=True, exist_ok=True)

        # Prepare CSV rows
        fieldnames = [
            'test_id', 'model_path', 'resolution', 'steps', 'num_gpus',
            'gpu_ids', 'duration', 'memory_peak', 'throughput', 'status',
            'timestamp'
        ]

        with open(output_file, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()

            for result in results:
                row = {
                    'test_id': result.test_id,
                    'model_path': result.model_path,
                    'resolution': result.resolution,
                    'steps': result.steps,
                    'num_gpus': result.num_gpus,
                    'gpu_ids': ','.join(map(str, result.gpu_ids)),
                    'duration': f"{result.duration:.4f}",
                    'memory_peak': f"{result.memory_peak:.2f}",
                    'throughput': f"{result.throughput:.4f}",
                    'status': result.status,
                    'timestamp': result.timestamp,
                }
                writer.writerow(row)

        print(f"Saved CSV results to: {output_file}", file=sys.stderr)
    except Exception as e:
        print(f"Error saving CSV results: {e}", file=sys.stderr)


def save_results_as_html(
    results: List[BenchmarkResult],
    output_path: str,
    template_path: Optional[str] = None
) -> None:
    """
    Save benchmark results as HTML file.

    Args:
        results: List of BenchmarkResult objects
        output_path: Output file path
        template_path: Optional path to HTML template
    """
    try:
        output_file = Path(output_path)
        output_file.parent.mkdir(parents=True, exist_ok=True)

        # Generate HTML content
        html_content = generate_html_report(results, template_path)

        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(html_content)

        print(f"Saved HTML results to: {output_file}", file=sys.stderr)
    except Exception as e:
        print(f"Error saving HTML results: {e}", file=sys.stderr)


def generate_html_report(
    results: List[BenchmarkResult],
    template_path: Optional[str] = None
) -> str:
    """
    Generate HTML report from benchmark results.

    Args:
        results: List of BenchmarkResult objects
        template_path: Optional path to HTML template

    Returns:
        HTML content as string
    """
    # Check if template exists
    if template_path and Path(template_path).exists():
        with open(template_path, 'r', encoding='utf-8') as f:
            template = f.read()
        return template

    # Generate default HTML
    successful = sum(1 for r in results if r.status == 'success')
    failed = sum(1 for r in results if r.status == 'failed')
    total_duration = sum(r.duration for r in results)
    avg_memory = sum(r.memory_peak for r in results) / len(results) if results else 0

    rows = ''.join([
        f"""
    <tr>
        <td>{result.test_id[:20]}...</td>
        <td>{result.resolution}</td>
        <td>{result.steps}</td>
        <td>{result.num_gpus}</td>
        <td>{result.duration:.4f}s</td>
        <td>{result.memory_peak:.2f}MB</td>
        <td>{result.throughput:.4f}</td>
        <td><span class="status-{result.status}">{result.status}</span></td>
    </tr>
        """
        for result in results
    ])

    html = f"""
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Benchmark Results Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; }}
        h1 {{ color: #333; }}
        .summary {{ background: #f5f5f5; padding: 10px; margin: 10px 0; border-radius: 5px; }}
        table {{ border-collapse: collapse; width: 100%; margin-top: 20px; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
        th {{ background-color: #4CAF50; color: white; }}
        tr:nth-child(even) {{ background-color: #f9f9f9; }}
        .status-success {{ color: green; font-weight: bold; }}
        .status-failed {{ color: red; font-weight: bold; }}
        .status-timeout {{ color: orange; font-weight: bold; }}
    </style>
</head>
<body>
    <h1>Benchmark Results Report</h1>
    <div class="summary">
        <p><strong>Total Tests:</strong> {len(results)}</p>
        <p><strong>Successful:</strong> {successful} | <strong>Failed:</strong> {failed}</p>
        <p><strong>Total Duration:</strong> {total_duration:.2f}s</p>
        <p><strong>Average Memory:</strong> {avg_memory:.2f}MB</p>
        <p><strong>Generated:</strong> {datetime.now().isoformat()}</p>
    </div>

    <h2>Detailed Results</h2>
    <table>
        <tr>
            <th>Test ID</th>
            <th>Resolution</th>
            <th>Steps</th>
            <th>GPUs</th>
            <th>Duration (s)</th>
            <th>Memory Peak (MB)</th>
            <th>Throughput</th>
            <th>Status</th>
        </tr>
        {rows}
    </table>
</body>
</html>
    """
    return html


def generate_outputs(
    results: List[BenchmarkResult],
    config: Dict[str, Any],
    verbose: bool = False
) -> None:
    """
    Generate all output files based on configuration.

    Args:
        results: List of BenchmarkResult objects
        config: Benchmark configuration
        verbose: Enable verbose output
    """
    output_config = config.get('output', {})
    output_formats = output_config.get('format', ['json'])
    output_path = output_config.get('save_path', 'benchmark_results')

    if not output_path:
        output_path = 'benchmark_results'

    # Ensure path has extension if not specified
    base_path = Path(output_path)
    if base_path.suffix == '':
        # No extension, use as base
        base_path = base_path.with_suffix('')
    else:
        # Has extension, remove it for base
        base_path = base_path.with_suffix('')

    for fmt in output_formats:
        if fmt == 'json':
            output_file = str(base_path.with_suffix('.json'))
            save_results_as_json(results, output_file)
        elif fmt == 'csv':
            output_file = str(base_path.with_suffix('.csv'))
            save_results_as_csv(results, output_file)
        elif fmt == 'html':
            output_file = str(base_path.with_suffix('.html'))
            template_path = config.get('html_template_path')
            save_results_as_html(results, output_file, template_path)

    if verbose:
        print(f"Output generation completed", file=sys.stderr)


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


def collect_system_info() -> Dict[str, Any]:
    """
    Collect system information for benchmarking context.

    Returns:
        Dictionary containing system information
    """
    try:
        import platform
        info = {
            'platform': platform.system(),
            'platform_release': platform.release(),
            'processor': platform.processor(),
            'machine': platform.machine(),
            'cpu_count': os.cpu_count(),
            'timestamp': datetime.now().isoformat(),
        }

        # Add memory info
        try:
            mem = psutil.virtual_memory()
            info['total_memory_mb'] = mem.total / (1024 * 1024)
            info['available_memory_mb'] = mem.available / (1024 * 1024)
        except Exception:
            pass

        # Add GPU info
        if HAS_TORCH:
            try:
                info['cuda_available'] = torch.cuda.is_available()
                if torch.cuda.is_available():
                    info['cuda_version'] = torch.version.cuda
                    info['gpu_count'] = torch.cuda.device_count()
                    info['gpu_names'] = [torch.cuda.get_device_name(i)
                                        for i in range(torch.cuda.device_count())]
            except Exception:
                pass

        return info
    except Exception as e:
        return {'error': str(e), 'timestamp': datetime.now().isoformat()}


def collect_gpu_memory_metrics(gpu_ids: List[int]) -> Dict[str, Any]:
    """
    Collect GPU memory metrics for specified GPUs.

    Args:
        gpu_ids: List of GPU IDs to collect metrics for

    Returns:
        Dictionary containing GPU memory metrics
    """
    metrics = {}

    if not HAS_TORCH or not torch.cuda.is_available():
        return metrics

    try:
        for gpu_id in gpu_ids:
            props = torch.cuda.get_device_properties(gpu_id)
            free, total = torch.cuda.mem_get_info(gpu_id)

            metrics[f'gpu_{gpu_id}'] = {
                'name': props.name,
                'total_memory_mb': total / (1024 * 1024),
                'free_memory_mb': free / (1024 * 1024),
                'used_memory_mb': (total - free) / (1024 * 1024),
                'compute_capability': f"{props.major}.{props.minor}",
            }
    except Exception as e:
        metrics['error'] = str(e)

    return metrics


def estimate_communication_bandwidth(num_gpus: int, gpu_ids: List[int]) -> Optional[float]:
    """
    Estimate communication bandwidth between GPUs.

    Args:
        num_gpus: Number of GPUs
        gpu_ids: List of GPU IDs

    Returns:
        Estimated bandwidth in MB/s, or None if cannot be determined
    """
    if num_gpus <= 1:
        return None

    if not HAS_TORCH or not torch.cuda.is_available():
        return None

    try:
        # Estimate based on GPU type
        device_props = torch.cuda.get_device_properties(gpu_ids[0])
        device_name = device_props.name.lower()

        # Typical bandwidth estimates (MB/s)
        bandwidth_estimates = {
            'a100': 2000.0,
            'a40': 960.0,
            'v100': 900.0,
            'h100': 3500.0,
            'rtx': 576.0,
            'gtx': 320.0,
        }

        for key, bw in bandwidth_estimates.items():
            if key in device_name:
                # Adjust for number of links (approximate)
                return bw * (min(num_gpus, 4) / 2)

        # Conservative estimate
        return 500.0 * (min(num_gpus, 4) / 2)
    except Exception:
        return None


def run_single_benchmark(
    combination: Dict[str, Any],
    timeout: int = 600,
    verbose: bool = False
) -> BenchmarkResult:
    """
    Run a single benchmark test.

    Args:
        combination: Test parameter combination
        timeout: Timeout in seconds
        verbose: Enable verbose output

    Returns:
        BenchmarkResult object with test results
    """
    test_id = f"{combination['model_path']}_{combination['resolution']}_{combination['steps']}_{combination['num_gpus']}gpu_{int(time.time())}"

    try:
        # Collect system info
        system_info = collect_system_info()
        gpu_memory_before = collect_gpu_memory_metrics(combination['gpu_ids'])

        # Simulate benchmark execution (actual implementation would run real inference)
        start_time = time.time()

        # Mock benchmark work
        time.sleep(0.1)  # Simulate some work

        end_time = time.time()
        duration = end_time - start_time

        # Collect GPU memory after
        gpu_memory_after = collect_gpu_memory_metrics(combination['gpu_ids'])

        # Calculate metrics
        memory_peak = 0
        for i, gpu_id in enumerate(combination['gpu_ids']):
            key = f'gpu_{gpu_id}'
            if key in gpu_memory_after:
                memory_peak = max(memory_peak, gpu_memory_after[key]['used_memory_mb'])

        # Calculate throughput (mock)
        resolution = ResolutionConfig.parse(combination['resolution'])
        pixels = resolution.width * resolution.height
        throughput = 1.0 / duration if duration > 0 else 0

        communication_bandwidth = estimate_communication_bandwidth(
            combination['num_gpus'],
            combination['gpu_ids']
        )

        return BenchmarkResult(
            test_id=test_id,
            model_path=combination['model_path'],
            resolution=combination['resolution'],
            steps=combination['steps'],
            num_gpus=combination['num_gpus'],
            gpu_ids=combination['gpu_ids'],
            prompt=combination['prompt'],
            timestamp=datetime.now().isoformat(),
            duration=duration,
            memory_peak=memory_peak,
            memory_allocated=memory_peak,
            throughput=throughput,
            status='success',
            system_info=system_info,
            gpu_memory_metrics=gpu_memory_after,
            communication_bandwidth=communication_bandwidth
        )

    except Exception as e:
        return BenchmarkResult(
            test_id=test_id,
            model_path=combination['model_path'],
            resolution=combination['resolution'],
            steps=combination['steps'],
            num_gpus=combination['num_gpus'],
            gpu_ids=combination['gpu_ids'],
            prompt=combination['prompt'],
            timestamp=datetime.now().isoformat(),
            duration=0,
            memory_peak=0,
            memory_allocated=0,
            throughput=0,
            status='failed',
            error_message=str(e)
        )


def run_all_benchmarks(
    config: Dict[str, Any],
    verbose: bool = False
) -> List[BenchmarkResult]:
    """
    Run all benchmark tests based on configuration.

    Args:
        config: Benchmark configuration
        verbose: Enable verbose output

    Returns:
        List of BenchmarkResult objects
    """
    results = []

    # Generate test combinations
    combinations = generate_test_combinations(config)

    if verbose:
        print(f"Generated {len(combinations)} test combinations", file=sys.stderr)

    # Extract execution settings
    execution_config = config.get('execution', {})
    timeout = execution_config.get('timeout_per_test', 600)

    # Run each test combination
    for i, combination in enumerate(combinations):
        if verbose:
            print(f"Running test {i+1}/{len(combinations)}: {combination['resolution']} "
                  f"{combination['steps']} steps on {combination['num_gpus']} GPU(s)",
                  file=sys.stderr)

        result = run_single_benchmark(combination, timeout=timeout, verbose=verbose)
        results.append(result)

    return results


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

    # Check if we should run benchmarks or just show config
    run_benchmarks = args.json or args.csv or args.output or not args.config

    if not run_benchmarks:
        # Just output merged config
        print("Configuration loaded successfully")
        if args.model:
            print(f"  Model: {args.model}")
        if args.resolution:
            print(f"  Resolution: {args.resolution}")
        if args.steps:
            print(f"  Steps: {args.steps}")
        return

    # Run benchmarks
    if args.verbose:
        print("Starting benchmark tests...", file=sys.stderr)

    results = run_all_benchmarks(merged_config, verbose=args.verbose)

    if args.verbose:
        print(f"Completed {len(results)} tests", file=sys.stderr)
        successful = sum(1 for r in results if r.status == 'success')
        print(f"  Successful: {successful}", file=sys.stderr)
        print(f"  Failed: {len(results) - successful}", file=sys.stderr)

    # Generate outputs
    if args.json or args.csv or args.output:
        generate_outputs(results, merged_config, verbose=args.verbose)
    else:
        # Print summary
        print(f"Benchmark Summary:")
        print(f"  Total tests: {len(results)}")
        successful = sum(1 for r in results if r.status == 'success')
        print(f"  Successful: {successful}")
        print(f"  Failed: {len(results) - successful}")

        if results:
            total_duration = sum(r.duration for r in results)
            avg_memory = sum(r.memory_peak for r in results) / len(results)
            print(f"  Total duration: {total_duration:.2f}s")
            print(f"  Average memory: {avg_memory:.2f}MB")


if __name__ == '__main__':
    main()
