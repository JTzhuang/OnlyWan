# Benchmark Scripts README

## Overview

This directory contains the OnlyWan Benchmark Framework - a comprehensive tool for performance benchmarking of model inference across various configurations.

## Files

- **benchmark_detailed.py**: Main benchmark framework
  - Configuration loading from YAML files
  - CLI argument parsing and merging
  - Parameter combination generation
  - Benchmark execution engine
  - Result collection and output generation

- **baseline_compare.py**: Baseline comparison tool
  - Load and analyze benchmark results
  - Compute performance deltas
  - Generate comparison reports

- **configs/**: Configuration directory
  - benchmark_default.yaml: Default configuration template
  - Other configuration files for different test scenarios

- **templates/**: HTML templates
  - report_template.html: HTML report template

## Quick Reference

### Run Benchmarks

```bash
# With configuration file
python3 benchmark_detailed.py --config configs/benchmark_default.yaml

# Override parameters
python3 benchmark_detailed.py --config configs/benchmark_default.yaml \
    --resolution 1024x1024 --steps 50

# Output to JSON and CSV
python3 benchmark_detailed.py --config configs/benchmark_default.yaml \
    --json --csv --output results
```

### Compare Results

```bash
# Compare against baseline
python3 baseline_compare.py \
    --baseline baseline.json \
    --current current.json \
    --output comparison.html
```

## Configuration

### YAML Configuration Example

```yaml
model: model.safetensors

test_cases:
  - name: "standard"
    resolutions: ["512x512", "1024x576"]
    steps: [20, 50]
    num_frames: 1

gpu_config:
  configurations: [1, 2, 4]
  prefer_gpus: [0, 1, 2, 3]

execution:
  timeout_per_test: 600
  retry_on_failure: 0

output:
  save_path: benchmark_results
  format: [json, csv, html]
```

### CLI Arguments

**Configuration Options:**
- `--config PATH`: YAML configuration file
- `--model PATH`: Model file path
- `--prompt TEXT`: Generation prompt

**Generation Parameters:**
- `--resolution WxH`: Resolution (e.g., 512x512)
- `--steps N`: Number of steps
- `--num-frames N`: Number of frames

**GPU Options:**
- `--num-gpus N`: Number of GPUs
- `--gpu-ids IDS`: GPU IDs (comma-separated)

**Execution:**
- `--timeout SEC`: Timeout in seconds
- `--retry N`: Number of retries

**Output:**
- `--json`: JSON output
- `--csv`: CSV output
- `--output PATH`: Output file path
- `--verbose`: Verbose output

## Output Formats

### JSON
Complete benchmark data with system info and detailed metrics.

### CSV
Tabular format for spreadsheet analysis.

### HTML
Beautiful interactive report with summary and detailed results.

## Features

- **Configuration Management**: Load YAML configs with CLI overrides
- **Parameter Combinations**: Auto-generate all test combinations
- **GPU Support**: Multi-GPU detection and configuration
- **Metrics Collection**:
  - System information (CPU, memory, GPU specs)
  - GPU memory utilization
  - Communication bandwidth estimation
  - Performance metrics (duration, throughput)
- **Output Formats**: JSON, CSV, HTML
- **Baseline Comparison**: Compare against previous results
- **Flexible CLI**: Override any config parameter from command line

## System Requirements

- Python 3.8+
- PyYAML (for config files)
- psutil (for system metrics)
- PyTorch (for GPU detection)
- torch (for CUDA support)

## Installation

```bash
# Install dependencies
pip install pyyaml psutil torch

# Make scripts executable
chmod +x benchmark_detailed.py baseline_compare.py
```

## Testing

Run the test suite:

```bash
pytest tests/test_benchmark_framework.py -v
pytest tests/test_multi_gpu_benchmark.py -v
```

## Examples

### Example 1: Quick Benchmark

```bash
python3 benchmark_detailed.py \
    --model model.safetensors \
    --resolution 512x512 \
    --steps 20 \
    --num-gpus 1 \
    --json --output results.json
```

### Example 2: Multi-Configuration Benchmark

```bash
python3 benchmark_detailed.py \
    --config configs/benchmark_default.yaml \
    --resolution 512x512,1024x1024 \
    --steps 20,50 \
    --num-gpus 1,2,4 \
    --json --csv --output benchmark_run_001
```

### Example 3: Baseline Comparison

```bash
# Run new benchmark
python3 benchmark_detailed.py \
    --config configs/benchmark_default.yaml \
    --json --output current_results.json

# Compare with baseline
python3 baseline_compare.py \
    --baseline baseline_results.json \
    --current current_results.json \
    --output comparison_report.html
```

## Output Directory Structure

```
results/
├── benchmark_run_001.json
├── benchmark_run_001.csv
├── benchmark_run_001.html
└── comparison_report.html
```

## Data Collection

The framework collects:

1. **Test Configuration**:
   - Model path
   - Resolution
   - Steps
   - GPU count and IDs
   - Prompt

2. **Timing Metrics**:
   - Total duration
   - Throughput (images/sec)

3. **Memory Metrics**:
   - Peak memory usage
   - Allocated memory
   - Per-GPU memory breakdown

4. **System Information**:
   - Platform and OS
   - CPU count
   - Total/available memory
   - CUDA version
   - GPU names and specs
   - GPU compute capability

5. **Performance Metrics**:
   - Communication bandwidth estimate
   - GPU utilization

## Troubleshooting

**GPU Not Detected:**
```bash
python3 -c "import torch; print(torch.cuda.is_available())"
```

**Configuration Not Loading:**
```bash
pip install pyyaml
python3 -c "import yaml; yaml.safe_load(open('config.yaml'))"
```

**Out of Memory:**
- Reduce resolution
- Reduce number of steps
- Use fewer GPUs
- Reduce batch size

## Performance Tuning

1. **For Speed**: Use fewer steps and smaller resolution
2. **For Accuracy**: Use more steps and larger resolution
3. **For GPU Memory**: Monitor peak memory and adjust configuration
4. **For Multi-GPU**: Specify GPU IDs explicitly

## API Usage

```python
from scripts.benchmark_detailed import (
    generate_test_combinations,
    run_all_benchmarks,
    generate_outputs,
    load_config_file
)

# Load configuration
config = load_config_file('configs/benchmark_default.yaml')

# Generate test combinations
combinations = generate_test_combinations(config)

# Run benchmarks
results = run_all_benchmarks(config, verbose=True)

# Generate outputs
generate_outputs(results, config)
```

## Contributing

To add new features:

1. Extend `BenchmarkResult` dataclass for new metrics
2. Add collection functions in benchmark_detailed.py
3. Update output generators
4. Add tests

## Support

See BENCHMARK_GUIDE.md in docs/superpowers/guides/ for detailed documentation.

## License

OnlyWan Project - All Rights Reserved
