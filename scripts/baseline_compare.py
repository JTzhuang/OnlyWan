#!/usr/bin/env python3
"""
Baseline Comparison Tool for Benchmark Results

Compares current benchmark results against baseline results and generates
a comparison report showing performance improvements or regressions.

Usage:
    python3 baseline_compare.py --baseline baseline_results.json --current current_results.json
    python3 baseline_compare.py --baseline baseline_results.json --current current_results.json --output comparison.html
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, Any, List, Tuple, Optional
from collections import defaultdict
from datetime import datetime


def load_benchmark_result(file_path: str) -> Dict[str, Any]:
    """
    Load benchmark result from JSON file.

    Args:
        file_path: Path to benchmark result JSON file

    Returns:
        Dictionary containing benchmark results

    Raises:
        FileNotFoundError: If file does not exist
        json.JSONDecodeError: If file is not valid JSON
    """
    result_file = Path(file_path)
    if not result_file.exists():
        raise FileNotFoundError(f"Result file not found: {file_path}")

    with open(result_file, 'r', encoding='utf-8') as f:
        data = json.load(f)

    return data


def extract_metrics_by_config(
    results: Dict[str, Any]
) -> Dict[str, Dict[str, float]]:
    """
    Extract metrics grouped by test configuration.

    Args:
        results: Benchmark results dictionary

    Returns:
        Dictionary mapping configuration key to metrics
    """
    metrics_by_config = defaultdict(dict)

    for result in results.get('results', []):
        # Create config key
        config_key = (
            f"{result['resolution']}_"
            f"{result['steps']}_steps_"
            f"{result['num_gpus']}gpu"
        )

        if result['status'] == 'success':
            metrics_by_config[config_key] = {
                'duration': result['duration'],
                'memory_peak': result['memory_peak'],
                'throughput': result['throughput'],
                'timestamp': result['timestamp'],
            }

    return dict(metrics_by_config)


def compute_deltas(
    baseline_metrics: Dict[str, Dict[str, float]],
    current_metrics: Dict[str, Dict[str, float]]
) -> Dict[str, Dict[str, Any]]:
    """
    Compute performance deltas between baseline and current results.

    Args:
        baseline_metrics: Metrics from baseline results
        current_metrics: Metrics from current results

    Returns:
        Dictionary containing deltas and comparisons
    """
    deltas = {}

    # Compare common configurations
    for config_key in baseline_metrics.keys():
        if config_key not in current_metrics:
            deltas[config_key] = {
                'status': 'missing_in_current',
                'message': 'This configuration was not tested in current run'
            }
            continue

        baseline = baseline_metrics[config_key]
        current = current_metrics[config_key]

        # Calculate performance changes
        duration_delta = current['duration'] - baseline['duration']
        duration_percent = (duration_delta / baseline['duration'] * 100) if baseline['duration'] > 0 else 0

        memory_delta = current['memory_peak'] - baseline['memory_peak']
        memory_percent = (memory_delta / baseline['memory_peak'] * 100) if baseline['memory_peak'] > 0 else 0

        throughput_delta = current['throughput'] - baseline['throughput']
        throughput_percent = (throughput_delta / baseline['throughput'] * 100) if baseline['throughput'] > 0 else 0

        deltas[config_key] = {
            'duration': {
                'baseline': baseline['duration'],
                'current': current['duration'],
                'delta': duration_delta,
                'percent_change': duration_percent,
                'status': 'better' if duration_delta < 0 else 'worse'
            },
            'memory': {
                'baseline': baseline['memory_peak'],
                'current': current['memory_peak'],
                'delta': memory_delta,
                'percent_change': memory_percent,
                'status': 'better' if memory_delta < 0 else 'worse'
            },
            'throughput': {
                'baseline': baseline['throughput'],
                'current': current['throughput'],
                'delta': throughput_delta,
                'percent_change': throughput_percent,
                'status': 'better' if throughput_delta > 0 else 'worse'
            }
        }

    # Check for new configurations in current
    for config_key in current_metrics.keys():
        if config_key not in baseline_metrics:
            deltas[config_key] = {
                'status': 'new_in_current',
                'message': 'This configuration is new and has no baseline'
            }

    return deltas


def generate_comparison_report(
    baseline_file: str,
    current_file: str,
    output_file: Optional[str] = None,
    verbose: bool = False
) -> Dict[str, Any]:
    """
    Generate comparison report between baseline and current results.

    Args:
        baseline_file: Path to baseline results file
        current_file: Path to current results file
        output_file: Optional output file path for HTML report
        verbose: Enable verbose output

    Returns:
        Dictionary containing comparison report data
    """
    # Load results
    try:
        baseline_results = load_benchmark_result(baseline_file)
        current_results = load_benchmark_result(current_file)
    except Exception as e:
        print(f"Error loading results: {e}", file=sys.stderr)
        sys.exit(1)

    if verbose:
        print(f"Loaded baseline results from: {baseline_file}", file=sys.stderr)
        print(f"Loaded current results from: {current_file}", file=sys.stderr)

    # Extract metrics
    baseline_metrics = extract_metrics_by_config(baseline_results)
    current_metrics = extract_metrics_by_config(current_results)

    if verbose:
        print(f"Baseline configurations: {len(baseline_metrics)}", file=sys.stderr)
        print(f"Current configurations: {len(current_metrics)}", file=sys.stderr)

    # Compute deltas
    deltas = compute_deltas(baseline_metrics, current_metrics)

    # Generate summary statistics
    successful_comparisons = sum(1 for d in deltas.values() if 'duration' in d)
    new_tests = sum(1 for d in deltas.values() if d.get('status') == 'new_in_current')
    missing_tests = sum(1 for d in deltas.values() if d.get('status') == 'missing_in_current')

    improvements = sum(
        1 for d in deltas.values()
        if 'duration' in d and d['duration']['status'] == 'better'
    )

    regressions = sum(
        1 for d in deltas.values()
        if 'duration' in d and d['duration']['status'] == 'worse'
    )

    avg_duration_change = 0
    if successful_comparisons > 0:
        avg_duration_change = sum(
            d['duration']['percent_change'] for d in deltas.values() if 'duration' in d
        ) / successful_comparisons

    report = {
        'timestamp': datetime.now().isoformat(),
        'baseline_file': baseline_file,
        'current_file': current_file,
        'baseline_timestamp': baseline_results.get('timestamp'),
        'current_timestamp': current_results.get('timestamp'),
        'comparison_summary': {
            'total_comparisons': successful_comparisons,
            'improvements': improvements,
            'regressions': regressions,
            'new_tests': new_tests,
            'missing_tests': missing_tests,
            'average_duration_change_percent': avg_duration_change,
        },
        'detailed_results': deltas
    }

    # Generate HTML report if requested
    if output_file:
        _generate_html_report(report, output_file)

    return report


def _generate_html_report(report: Dict[str, Any], output_file: str) -> None:
    """
    Generate HTML comparison report.

    Args:
        report: Comparison report data
        output_file: Output file path
    """
    try:
        summary = report['comparison_summary']
        rows = _generate_comparison_table_rows(report['detailed_results'])

        html_content = f"""
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Benchmark Comparison Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }}
        h1 {{ color: #333; }}
        .summary {{ background: white; padding: 15px; margin: 20px 0; border-radius: 5px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        .summary h2 {{ color: #4CAF50; }}
        .metric {{ display: inline-block; margin-right: 30px; }}
        .metric-label {{ color: #666; font-weight: bold; }}
        .metric-value {{ font-size: 24px; color: #333; font-weight: bold; }}
        .better {{ color: green; }}
        .worse {{ color: red; }}
        .neutral {{ color: #666; }}
        table {{ width: 100%; border-collapse: collapse; background: white; margin-top: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        th, td {{ border: 1px solid #ddd; padding: 12px; text-align: left; }}
        th {{ background-color: #4CAF50; color: white; }}
        tr:nth-child(even) {{ background-color: #f9f9f9; }}
        .config-key {{ font-family: monospace; font-size: 12px; }}
    </style>
</head>
<body>
    <h1>Benchmark Comparison Report</h1>

    <div class="summary">
        <h2>Summary</h2>
        <div class="metric">
            <div class="metric-label">Total Comparisons</div>
            <div class="metric-value">{summary['total_comparisons']}</div>
        </div>
        <div class="metric">
            <div class="metric-label">Improvements</div>
            <div class="metric-value better">{summary['improvements']}</div>
        </div>
        <div class="metric">
            <div class="metric-label">Regressions</div>
            <div class="metric-value worse">{summary['regressions']}</div>
        </div>
        <div class="metric">
            <div class="metric-label">New Tests</div>
            <div class="metric-value">{summary['new_tests']}</div>
        </div>
        <div class="metric">
            <div class="metric-label">Missing Tests</div>
            <div class="metric-value">{summary['missing_tests']}</div>
        </div>
        <div style="margin-top: 10px;">
            <p><strong>Average Duration Change:</strong> <span class="{'better' if summary['average_duration_change_percent'] < 0 else 'worse'}">{summary['average_duration_change_percent']:.2f}%</span></p>
        </div>
    </div>

    <h2>Detailed Results</h2>
    <table>
        <tr>
            <th>Configuration</th>
            <th>Metric</th>
            <th>Baseline</th>
            <th>Current</th>
            <th>Delta</th>
            <th>Change (%)</th>
            <th>Status</th>
        </tr>
        {rows}
    </table>

    <p style="color: #666; font-size: 12px; margin-top: 20px;">
        Generated: {datetime.now().isoformat()}
    </p>
</body>
</html>
        """

        output_path = Path(output_file)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(html_content)

        print(f"Saved HTML comparison report to: {output_path}", file=sys.stderr)

    except Exception as e:
        print(f"Error generating HTML report: {e}", file=sys.stderr)


def _generate_comparison_table_rows(deltas: Dict[str, Dict[str, Any]]) -> str:
    """Generate HTML table rows for comparison results."""
    rows = []

    for config_key, data in sorted(deltas.items()):
        if data.get('status'):
            # Status-based results
            rows.append(f"""
    <tr>
        <td class="config-key">{config_key}</td>
        <td colspan="6">{data.get('message', data.get('status'))}</td>
    </tr>
            """)
        else:
            # Detailed metric results
            for metric in ['duration', 'memory', 'throughput']:
                if metric in data:
                    m = data[metric]
                    status_class = m['status']
                    rows.append(f"""
    <tr>
        <td class="config-key">{config_key}</td>
        <td>{metric.capitalize()}</td>
        <td>{m['baseline']:.4f}</td>
        <td>{m['current']:.4f}</td>
        <td class="{status_class}">{m['delta']:+.4f}</td>
        <td class="{status_class}">{m['percent_change']:+.2f}%</td>
        <td class="{status_class}">{status_class.upper()}</td>
    </tr>
                    """)

    return ''.join(rows)


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='Compare benchmark results against baseline',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Compare current results with baseline
  python3 baseline_compare.py --baseline baseline.json --current current.json

  # Generate HTML comparison report
  python3 baseline_compare.py --baseline baseline.json --current current.json \\
    --output comparison.html

  # Verbose output
  python3 baseline_compare.py --baseline baseline.json --current current.json --verbose
        """
    )

    parser.add_argument(
        '--baseline',
        type=str,
        required=True,
        help='Path to baseline benchmark results JSON file'
    )
    parser.add_argument(
        '--current',
        type=str,
        required=True,
        help='Path to current benchmark results JSON file'
    )
    parser.add_argument(
        '--output',
        type=str,
        help='Output file path for HTML comparison report'
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Enable verbose output'
    )

    args = parser.parse_args()

    # Generate comparison report
    report = generate_comparison_report(
        args.baseline,
        args.current,
        output_file=args.output,
        verbose=args.verbose
    )

    # Print summary to stdout
    summary = report['comparison_summary']
    print("Benchmark Comparison Summary")
    print("=" * 50)
    print(f"Total comparisons: {summary['total_comparisons']}")
    print(f"Improvements: {summary['improvements']}")
    print(f"Regressions: {summary['regressions']}")
    print(f"New tests: {summary['new_tests']}")
    print(f"Missing tests: {summary['missing_tests']}")
    print(f"Average duration change: {summary['average_duration_change_percent']:.2f}%")


if __name__ == '__main__':
    main()
