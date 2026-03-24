"""
Integration tests for benchmark framework
"""

import pytest
import json
import sys
import tempfile
from pathlib import Path
from unittest.mock import patch, MagicMock
from scripts.benchmark_detailed import (
    generate_test_combinations,
    run_all_benchmarks,
    generate_outputs,
    save_results_as_json,
    save_results_as_csv,
    save_results_as_html,
    collect_system_info,
    collect_gpu_memory_metrics,
    estimate_communication_bandwidth,
    BenchmarkResult,
    ResolutionConfig
)


class TestParameterCombinations:
    """Test parameter combination generation"""

    def test_single_configuration(self):
        """Test generation with single configuration"""
        config = {
            'model': 'test_model.safetensors',
            'test_cases': [{
                'resolutions': ['512x512'],
                'steps': [20],
                'num_frames': 1,
                'prompt': 'test'
            }],
            'gpu_config': {
                'configurations': [1]
            }
        }

        combinations = generate_test_combinations(config)

        assert len(combinations) == 1
        assert combinations[0]['resolution'] == '512x512'
        assert combinations[0]['steps'] == 20
        assert combinations[0]['num_gpus'] == 1

    def test_multiple_resolutions(self):
        """Test generation with multiple resolutions"""
        config = {
            'model': 'test_model.safetensors',
            'test_cases': [{
                'resolutions': ['512x512', '768x768', '1024x576'],
                'steps': [20],
                'num_frames': 1,
                'prompt': 'test'
            }],
            'gpu_config': {
                'configurations': [1]
            }
        }

        combinations = generate_test_combinations(config)

        assert len(combinations) == 3
        resolutions = [c['resolution'] for c in combinations]
        assert '512x512' in resolutions
        assert '768x768' in resolutions
        assert '1024x576' in resolutions

    def test_multiple_gpu_configs(self):
        """Test generation with multiple GPU configurations"""
        config = {
            'model': 'test_model.safetensors',
            'test_cases': [{
                'resolutions': ['512x512'],
                'steps': [20],
                'num_frames': 1,
                'prompt': 'test'
            }],
            'gpu_config': {
                'configurations': [1, 2, 4]
            }
        }

        combinations = generate_test_combinations(config)

        assert len(combinations) == 3
        gpu_counts = [c['num_gpus'] for c in combinations]
        assert 1 in gpu_counts
        assert 2 in gpu_counts
        assert 4 in gpu_counts

    def test_combination_explosion(self):
        """Test generation with many parameters"""
        config = {
            'model': 'test_model.safetensors',
            'test_cases': [{
                'resolutions': ['512x512', '768x768'],
                'steps': [20, 50],
                'num_frames': 1,
                'prompt': 'test'
            }],
            'gpu_config': {
                'configurations': [1, 2]
            }
        }

        combinations = generate_test_combinations(config)

        # 2 resolutions × 2 steps × 2 gpu configs = 8 combinations
        assert len(combinations) == 8


class TestSystemMetrics:
    """Test system metrics collection"""

    def test_collect_system_info(self):
        """Test system information collection"""
        info = collect_system_info()

        assert 'platform' in info
        assert 'cpu_count' in info
        assert 'timestamp' in info

    def test_collect_system_info_error_handling(self):
        """Test error handling in system info collection"""
        # Should not raise exception even if some data is unavailable
        info = collect_system_info()
        assert isinstance(info, dict)


class TestBenchmarkResults:
    """Test benchmark result handling"""

    def test_benchmark_result_creation(self):
        """Test BenchmarkResult dataclass"""
        result = BenchmarkResult(
            test_id='test_001',
            model_path='model.safetensors',
            resolution='512x512',
            steps=20,
            num_gpus=1,
            gpu_ids=[0],
            prompt='test prompt',
            timestamp='2026-03-24T10:00:00',
            duration=5.0,
            memory_peak=8000.0,
            memory_allocated=8000.0,
            throughput=0.2,
            status='success'
        )

        assert result.test_id == 'test_001'
        assert result.status == 'success'
        assert result.duration == 5.0

    def test_benchmark_result_to_dict(self):
        """Test BenchmarkResult conversion to dict"""
        result = BenchmarkResult(
            test_id='test_001',
            model_path='model.safetensors',
            resolution='512x512',
            steps=20,
            num_gpus=1,
            gpu_ids=[0],
            prompt='test prompt',
            timestamp='2026-03-24T10:00:00',
            duration=5.0,
            memory_peak=8000.0,
            memory_allocated=8000.0,
            throughput=0.2,
            status='success'
        )

        result_dict = result.to_dict()

        assert result_dict['test_id'] == 'test_001'
        assert result_dict['status'] == 'success'
        assert isinstance(result_dict, dict)


class TestOutputGeneration:
    """Test output file generation"""

    def test_save_json_output(self):
        """Test JSON output generation"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_file = Path(tmpdir) / 'results.json'

            results = [
                BenchmarkResult(
                    test_id='test_001',
                    model_path='model.safetensors',
                    resolution='512x512',
                    steps=20,
                    num_gpus=1,
                    gpu_ids=[0],
                    prompt='test',
                    timestamp='2026-03-24T10:00:00',
                    duration=5.0,
                    memory_peak=8000.0,
                    memory_allocated=8000.0,
                    throughput=0.2,
                    status='success'
                )
            ]

            save_results_as_json(results, str(output_file))

            assert output_file.exists()

            with open(output_file, 'r') as f:
                data = json.load(f)

            assert 'results' in data
            assert len(data['results']) == 1
            assert data['results'][0]['test_id'] == 'test_001'

    def test_save_csv_output(self):
        """Test CSV output generation"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_file = Path(tmpdir) / 'results.csv'

            results = [
                BenchmarkResult(
                    test_id='test_001',
                    model_path='model.safetensors',
                    resolution='512x512',
                    steps=20,
                    num_gpus=1,
                    gpu_ids=[0],
                    prompt='test',
                    timestamp='2026-03-24T10:00:00',
                    duration=5.0,
                    memory_peak=8000.0,
                    memory_allocated=8000.0,
                    throughput=0.2,
                    status='success'
                )
            ]

            save_results_as_csv(results, str(output_file))

            assert output_file.exists()

            with open(output_file, 'r') as f:
                lines = f.readlines()

            assert len(lines) >= 2  # Header + at least one row

    def test_save_html_output(self):
        """Test HTML output generation"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_file = Path(tmpdir) / 'results.html'

            results = [
                BenchmarkResult(
                    test_id='test_001',
                    model_path='model.safetensors',
                    resolution='512x512',
                    steps=20,
                    num_gpus=1,
                    gpu_ids=[0],
                    prompt='test',
                    timestamp='2026-03-24T10:00:00',
                    duration=5.0,
                    memory_peak=8000.0,
                    memory_allocated=8000.0,
                    throughput=0.2,
                    status='success'
                )
            ]

            save_results_as_html(results, str(output_file))

            assert output_file.exists()

            with open(output_file, 'r') as f:
                content = f.read()

            assert '<!DOCTYPE html>' in content or '<html' in content

    def test_generate_outputs_multiple_formats(self):
        """Test generating multiple output formats"""
        with tempfile.TemporaryDirectory() as tmpdir:
            config = {
                'output': {
                    'save_path': str(Path(tmpdir) / 'results'),
                    'format': ['json', 'csv', 'html']
                }
            }

            results = [
                BenchmarkResult(
                    test_id='test_001',
                    model_path='model.safetensors',
                    resolution='512x512',
                    steps=20,
                    num_gpus=1,
                    gpu_ids=[0],
                    prompt='test',
                    timestamp='2026-03-24T10:00:00',
                    duration=5.0,
                    memory_peak=8000.0,
                    memory_allocated=8000.0,
                    throughput=0.2,
                    status='success'
                )
            ]

            generate_outputs(results, config)

            base_path = Path(tmpdir) / 'results'
            assert (base_path.with_suffix('.json')).exists()
            assert (base_path.with_suffix('.csv')).exists()
            assert (base_path.with_suffix('.html')).exists()


class TestEndToEnd:
    """End-to-end integration tests"""

    def test_full_benchmark_workflow(self):
        """Test complete benchmark workflow"""
        config = {
            'model': 'test_model.safetensors',
            'test_cases': [{
                'resolutions': ['512x512'],
                'steps': [20],
                'num_frames': 1,
                'prompt': 'test'
            }],
            'gpu_config': {
                'configurations': [1]
            },
            'execution': {
                'timeout_per_test': 600
            }
        }

        # Generate combinations
        combinations = generate_test_combinations(config)
        assert len(combinations) > 0

        # Run benchmarks
        results = run_all_benchmarks(config)
        assert len(results) == len(combinations)

        # Check results are valid
        for result in results:
            assert result.test_id is not None
            assert result.status in ['success', 'failed', 'timeout']

    def test_benchmark_output_integration(self):
        """Test benchmark with output generation"""
        with tempfile.TemporaryDirectory() as tmpdir:
            config = {
                'model': 'test_model.safetensors',
                'test_cases': [{
                    'resolutions': ['512x512'],
                    'steps': [20],
                    'num_frames': 1,
                    'prompt': 'test'
                }],
                'gpu_config': {
                    'configurations': [1]
                },
                'execution': {
                    'timeout_per_test': 600
                },
                'output': {
                    'save_path': str(Path(tmpdir) / 'results'),
                    'format': ['json', 'csv']
                }
            }

            # Run full workflow
            results = run_all_benchmarks(config)
            assert len(results) > 0

            # Generate outputs
            generate_outputs(results, config)

            # Verify outputs
            base_path = Path(tmpdir) / 'results'
            assert (base_path.with_suffix('.json')).exists()
            assert (base_path.with_suffix('.csv')).exists()


class TestResolutionParsing:
    """Test resolution parsing with combinations"""

    def test_combined_resolution_parsing(self):
        """Test resolution parsing in combination context"""
        config = {
            'model': 'test.safetensors',
            'test_cases': [{
                'resolutions': ['512x512', '1024x576', '768x768'],
                'steps': [20],
                'num_frames': 1,
                'prompt': 'test'
            }],
            'gpu_config': {
                'configurations': [1]
            }
        }

        combinations = generate_test_combinations(config)

        for combo in combinations:
            # Should be able to parse all resolutions
            res_config = ResolutionConfig.parse(combo['resolution'])
            assert res_config.width > 0
            assert res_config.height > 0


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
