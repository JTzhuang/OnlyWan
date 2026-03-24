# OnlyWan Benchmark Framework Guide

## Overview

The OnlyWan Benchmark Framework provides comprehensive performance benchmarking capabilities for testing model inference across various configurations. It supports multi-GPU configurations, detailed metrics collection, and flexible output formats.

## Key Features

- **Configuration Management**: Load and merge YAML config files with CLI parameters
- **Parameter Combinations**: Automatically generate all test combinations from parameters
- **Multi-GPU Support**: Test on single or multiple GPUs with automatic detection
- **Detailed Metrics**: Collect system info, GPU memory, communication bandwidth
- **Multiple Output Formats**: JSON, CSV, and HTML reports
- **Baseline Comparison**: Compare results against baseline benchmarks
- **Flexible CLI Interface**: Override config values from command line

## Quick Start

### Basic Usage

```bash
# Run with default configuration
python3 scripts/benchmark_detailed.py --config scripts/configs/benchmark_default.yaml

# Override specific parameters
python3 scripts/benchmark_detailed.py --config scripts/configs/benchmark_default.yaml \
    --resolution 1024x1024 --steps 50 --num-gpus 4

# Run without config file
python3 scripts/benchmark_detailed.py \
    --model model.safetensors \
    --resolution 512x512 \
    --steps 20 \
    --num-gpus 2 \
    --json --output results.json
```

### Output Options

```bash
# Save as JSON
python3 scripts/benchmark_detailed.py --config config.yaml --json --output results.json

# Save as CSV
python3 scripts/benchmark_detailed.py --config config.yaml --csv --output results.csv

# Generate HTML report
python3 scripts/benchmark_detailed.py --config config.yaml --output results.html

# Multiple formats
python3 scripts/benchmark_detailed.py --config config.yaml \
    --json --csv --output results
```

## Configuration File Format

The benchmark framework uses YAML configuration files. Here's the complete structure:

```yaml
# Model and basic settings
model: path/to/model.safetensors
test_mode: standard

# Test cases - can have multiple test cases
test_cases:
  - name: "standard_tests"
    prompt: "A beautiful sunset over mountains"
    resolutions:
      - "512x512"
      - "768x768"
      - "1024x576"
    steps:
      - 20
      - 50
    num_frames: 1

# GPU configuration
gpu_config:
  auto_detect: true
  min_required: 1
  configurations:
    - 1    # Single GPU
    - 2    # Dual GPU
    - 4    # Quad GPU
  prefer_gpus: [0, 1, 2, 3]  # Preferred GPU IDs

# Execution settings
execution:
  timeout_per_test: 600        # seconds
  retry_on_failure: 0          # number of retries
  parallel_execution: false    # not yet implemented

# Output settings
output:
  save_path: benchmark_results
  format:
    - json
    - csv
    - html
  html_template_path: scripts/templates/report_template.html
```

## CLI Arguments

### Configuration and Model

```
--config PATH              Path to YAML configuration file
--model PATH              Path to model file (safetensors or checkpoint)
--prompt TEXT             Text prompt for generation (default: "A cat playing with a ball")
```

### Generation Parameters

```
--resolution WxH          Resolution in WxH format (e.g., 512x512, 1024x576)
--steps N                 Number of sampling steps
--num-frames N            Number of frames to generate
```

### GPU Configuration

```
--num-gpus N              Number of GPUs to use
--gpu-ids IDS             Comma-separated GPU IDs (e.g., 0,1,2)
```

### Execution Options

```
--timeout SECONDS         Timeout per test in seconds (default: 600)
--retry N                 Number of retries on failure (default: 0)
```

### Output Options

```
--output PATH             Output file path for results
--json                    Output results as JSON
--csv                     Output results as CSV
--verbose                 Verbose output
```

## Output Formats

### JSON Format

The JSON output contains comprehensive benchmark data:

```json
{
  "timestamp": "2026-03-24T10:30:00.000000",
  "total_tests": 12,
  "successful": 12,
  "failed": 0,
  "results": [
    {
      "test_id": "model.safetensors_512x512_20_1gpu_1711270200",
      "model_path": "model.safetensors",
      "resolution": "512x512",
      "steps": 20,
      "num_gpus": 1,
      "gpu_ids": [0],
      "prompt": "A beautiful sunset",
      "timestamp": "2026-03-24T10:30:00.000000",
      "duration": 5.234,
      "memory_peak": 8024.5,
      "memory_allocated": 8024.5,
      "throughput": 0.191,
      "status": "success",
      "error_message": null,
      "system_info": {
        "platform": "Linux",
        "cpu_count": 32,
        "cuda_available": true,
        "gpu_count": 4,
        "gpu_names": ["NVIDIA A100", "NVIDIA A100", "NVIDIA A100", "NVIDIA A100"]
      },
      "gpu_memory_metrics": {
        "gpu_0": {
          "name": "NVIDIA A100",
          "total_memory_mb": 40960.0,
          "free_memory_mb": 32935.5,
          "used_memory_mb": 8024.5,
          "compute_capability": "8.0"
        }
      },
      "communication_bandwidth": 2000.0
    }
  ]
}
```

### CSV Format

Tabular format for easy analysis in spreadsheets:

```csv
test_id,model_path,resolution,steps,num_gpus,gpu_ids,duration,memory_peak,throughput,status,timestamp
model.safetensors_512x512_20_1gpu_1711270200,model.safetensors,512x512,20,1,0,5.2340,8024.50,0.1910,success,2026-03-24T10:30:00.000000
```

### HTML Format

Beautiful HTML report with charts and summary statistics:

- Summary cards with key metrics
- Detailed results table
- System configuration information
- Performance analysis sections
- Responsive design for mobile viewing

## Baseline Comparison

Compare current results against baseline results:

```bash
# Basic comparison
python3 scripts/baseline_compare.py \
    --baseline baseline_results.json \
    --current current_results.json

# Generate HTML comparison report
python3 scripts/baseline_compare.py \
    --baseline baseline_results.json \
    --current current_results.json \
    --output comparison.html

# Verbose output
python3 scripts/baseline_compare.py \
    --baseline baseline_results.json \
    --current current_results.json \
    --verbose
```

### Comparison Output

The comparison tool generates:

1. **Summary Statistics**:
   - Total comparisons performed
   - Number of improvements/regressions
   - New tests in current run
   - Missing tests from current run
   - Average performance change

2. **Detailed Metrics**:
   - Duration changes with percentage
   - Memory usage changes
   - Throughput improvements/regressions

3. **HTML Report**:
   - Visual summary cards
   - Detailed comparison table
   - Performance status indicators

## Example Workflows

### Complete Benchmark Run

```bash
# 1. Run benchmarks with specific parameters
python3 scripts/benchmark_detailed.py \
    --config scripts/configs/benchmark_default.yaml \
    --resolution 512x512,1024x1024 \
    --steps 20,50 \
    --num-gpus 1,2,4 \
    --json --csv --output results/run_001

# 2. Compare with baseline
python3 scripts/baseline_compare.py \
    --baseline results/baseline.json \
    --current results/run_001.json \
    --output results/run_001_comparison.html

# 3. Review results
# Open results/run_001.json in your editor
# Open results/run_001.csv in spreadsheet application
# Open results/run_001_comparison.html in web browser
```

### Development Testing

```bash
# Quick test with minimal parameters
python3 scripts/benchmark_detailed.py \
    --model model.safetensors \
    --resolution 512x512 \
    --steps 20 \
    --num-gpus 1 \
    --json --output test_results.json \
    --verbose
```

### Continuous Integration

```bash
# Run full benchmark suite and compare against baseline
./scripts/ci_benchmark.sh baseline_results.json current_results.json
```

## Troubleshooting

### GPU Detection Issues

If GPUs are not detected:

```bash
# Check CUDA availability
python3 -c "import torch; print(torch.cuda.is_available())"

# Check available GPUs
python3 -c "import torch; print(torch.cuda.device_count())"

# Specify GPU IDs explicitly
python3 scripts/benchmark_detailed.py --config config.yaml --gpu-ids 0,1,2
```

### Memory Issues

If running out of memory:

```bash
# Reduce resolution
python3 scripts/benchmark_detailed.py --config config.yaml --resolution 512x512

# Reduce number of steps
python3 scripts/benchmark_detailed.py --config config.yaml --steps 20

# Use fewer GPUs
python3 scripts/benchmark_detailed.py --config config.yaml --num-gpus 1
```

### Configuration Loading Errors

Ensure PyYAML is installed:

```bash
pip install pyyaml
```

Validate YAML syntax:

```bash
python3 -c "import yaml; yaml.safe_load(open('config.yaml'))"
```

## Performance Optimization Tips

1. **Resolution**: Smaller resolutions complete faster (512x512 < 1024x1024)
2. **Steps**: Fewer steps reduce computation (20 < 50)
3. **GPU Count**: Utilize all available GPUs for parallel execution
4. **Memory**: Monitor peak memory usage and adjust batch size if needed

## API Reference

### Main Functions

#### `generate_test_combinations(config: Dict) -> List[Dict]`

Generate all test parameter combinations from configuration.

#### `run_all_benchmarks(config: Dict, verbose: bool) -> List[BenchmarkResult]`

Execute all benchmark tests and return results.

#### `generate_outputs(results: List[BenchmarkResult], config: Dict, verbose: bool) -> None`

Generate output files in specified formats.

### BenchmarkResult Data Class

```python
@dataclass
class BenchmarkResult:
    test_id: str
    model_path: str
    resolution: str
    steps: int
    num_gpus: int
    gpu_ids: List[int]
    prompt: str
    timestamp: str
    duration: float
    memory_peak: float
    memory_allocated: float
    throughput: float
    status: str
    error_message: Optional[str]
    system_info: Dict[str, Any]
    gpu_memory_metrics: Dict[str, Any]
    communication_bandwidth: Optional[float]
```

## Contributing

To extend the benchmark framework:

1. Add new metrics in `collect_*` functions
2. Extend `BenchmarkResult` dataclass with new fields
3. Update output generators for new formats
4. Add tests to verify changes

## Support

For issues or questions:

1. Check the troubleshooting section
2. Review example configurations in `scripts/configs/`
3. Check test files in `tests/` for usage examples
4. Open an issue on the project repository

## License

OnlyWan Benchmark Framework is part of the OnlyWan project.
