"""
Unit tests for benchmark framework configuration system
"""

import pytest
import tempfile
from pathlib import Path
from scripts.benchmark_detailed import (
    ResolutionConfig,
    load_config_file,
    parse_args,
    merge_config_with_args
)


class TestResolutionConfig:
    """Tests for ResolutionConfig parser"""

    def test_resolution_parse_valid(self):
        """Test parsing valid resolution strings"""
        res = ResolutionConfig.parse("512x512")
        assert res.width == 512
        assert res.height == 512

        res = ResolutionConfig.parse("1024x576")
        assert res.width == 1024
        assert res.height == 576

        res = ResolutionConfig.parse("768x768")
        assert res.width == 768
        assert res.height == 768

    def test_resolution_parse_with_spaces(self):
        """Test parsing resolution with whitespace"""
        res = ResolutionConfig.parse("  512x512  ")
        assert res.width == 512
        assert res.height == 512

    def test_resolution_parse_invalid_format(self):
        """Test parsing invalid resolution formats"""
        with pytest.raises(ValueError, match="Invalid resolution format"):
            ResolutionConfig.parse("invalid")

        with pytest.raises(ValueError, match="Invalid resolution format"):
            ResolutionConfig.parse("512")

        with pytest.raises(ValueError, match="Invalid resolution format"):
            ResolutionConfig.parse("512x")

        with pytest.raises(ValueError, match="Invalid resolution format"):
            ResolutionConfig.parse("x512")

        with pytest.raises(ValueError, match="Invalid resolution format"):
            ResolutionConfig.parse("512-512")

    def test_resolution_parse_invalid_dimensions(self):
        """Test parsing with invalid dimension values"""
        with pytest.raises(ValueError, match="must be positive"):
            ResolutionConfig.parse("0x512")

        with pytest.raises(ValueError, match="must be positive"):
            ResolutionConfig.parse("512x0")

        with pytest.raises(ValueError, match="Invalid resolution format"):
            ResolutionConfig.parse("-512x512")

    def test_resolution_str_representation(self):
        """Test string representation"""
        res = ResolutionConfig.parse("512x512")
        assert str(res) == "512x512"


class TestConfigLoading:
    """Tests for configuration file loading"""

    def test_load_yaml_config(self):
        """Test loading valid YAML config"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
            f.write("""
test_mode: "standard"
test_cases:
  - name: "test1"
    resolutions:
      - "512x512"
    steps:
      - 20
gpu_config:
  auto_detect: true
  min_required: 1
""")
            f.flush()
            config_path = f.name

        try:
            config = load_config_file(config_path)
            assert config['test_mode'] == 'standard'
            assert config['test_cases'][0]['name'] == 'test1'
            assert '512x512' in config['test_cases'][0]['resolutions']
            assert config['gpu_config']['auto_detect'] is True
        finally:
            Path(config_path).unlink()

    def test_load_empty_yaml_config(self):
        """Test loading empty YAML file"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
            f.write("")
            f.flush()
            config_path = f.name

        try:
            config = load_config_file(config_path)
            assert config == {}
        finally:
            Path(config_path).unlink()

    def test_config_file_not_found(self):
        """Test loading non-existent config file"""
        with pytest.raises(FileNotFoundError, match="Config file not found"):
            load_config_file("/nonexistent/path/config.yaml")

    def test_load_config_without_yaml(self, monkeypatch):
        """Test loading config when PyYAML is not available"""
        # Temporarily disable HAS_YAML
        import scripts.benchmark_detailed as bd
        original_has_yaml = bd.HAS_YAML
        bd.HAS_YAML = False

        try:
            with pytest.raises(ImportError, match="PyYAML is required"):
                load_config_file("dummy.yaml")
        finally:
            bd.HAS_YAML = original_has_yaml


class TestConfigMerging:
    """Tests for configuration merging with CLI arguments"""

    def test_merge_empty_config_with_args(self):
        """Test merging empty config with CLI arguments"""
        import sys
        from unittest.mock import patch

        with patch.object(sys, 'argv', [
            'benchmark_detailed.py',
            '--model', 'model.safetensors',
            '--resolution', '512x512',
            '--steps', '20'
        ]):
            args = parse_args()
            merged = merge_config_with_args({}, args)

            assert merged['model'] == 'model.safetensors'
            assert merged['test_cases'][0]['resolutions'] == ['512x512']
            assert merged['test_cases'][0]['steps'] == [20]

    def test_merge_config_cli_priority(self):
        """Test that CLI arguments override config file values"""
        import sys
        from unittest.mock import patch

        config = {
            'model': 'old_model.safetensors',
            'test_cases': [{
                'resolutions': ['768x768'],
                'steps': [50]
            }]
        }

        with patch.object(sys, 'argv', [
            'benchmark_detailed.py',
            '--model', 'new_model.safetensors',
            '--resolution', '512x512',
            '--steps', '20'
        ]):
            args = parse_args()
            merged = merge_config_with_args(config, args)

            # CLI values should override config
            assert merged['model'] == 'new_model.safetensors'
            assert merged['test_cases'][0]['resolutions'] == ['512x512']
            assert merged['test_cases'][0]['steps'] == [20]

    def test_merge_gpu_config(self):
        """Test merging GPU configuration"""
        import sys
        from unittest.mock import patch

        with patch.object(sys, 'argv', [
            'benchmark_detailed.py',
            '--num-gpus', '4',
            '--gpu-ids', '0,1,2,3'
        ]):
            args = parse_args()
            merged = merge_config_with_args({}, args)

            assert merged['gpu_config']['configurations'] == [4]
            assert merged['gpu_config']['prefer_gpus'] == [0, 1, 2, 3]

    def test_merge_output_formats(self):
        """Test merging output format options"""
        import sys
        from unittest.mock import patch

        with patch.object(sys, 'argv', [
            'benchmark_detailed.py',
            '--json',
            '--csv'
        ]):
            args = parse_args()
            merged = merge_config_with_args({}, args)

            assert 'json' in merged['output']['format']
            assert 'csv' in merged['output']['format']

    def test_merge_execution_config(self):
        """Test merging execution configuration"""
        import sys
        from unittest.mock import patch

        with patch.object(sys, 'argv', [
            'benchmark_detailed.py',
            '--timeout', '300',
            '--retry', '3'
        ]):
            args = parse_args()
            merged = merge_config_with_args({}, args)

            assert merged['execution']['timeout_per_test'] == 300
            assert merged['execution']['retry_on_failure'] == 3


class TestArgumentParsing:
    """Tests for command-line argument parsing"""

    def test_parse_basic_args(self):
        """Test parsing basic arguments"""
        import sys
        from unittest.mock import patch

        with patch.object(sys, 'argv', [
            'benchmark_detailed.py',
            '--model', 'model.safetensors',
            '--steps', '20'
        ]):
            args = parse_args()
            assert args.model == 'model.safetensors'
            assert args.steps == 20

    def test_parse_with_config(self):
        """Test parsing with config file argument"""
        import sys
        from unittest.mock import patch

        with patch.object(sys, 'argv', [
            'benchmark_detailed.py',
            '--config', 'configs/benchmark_default.yaml'
        ]):
            args = parse_args()
            assert args.config == 'configs/benchmark_default.yaml'

    def test_parse_all_arguments(self):
        """Test parsing all available arguments"""
        import sys
        from unittest.mock import patch

        with patch.object(sys, 'argv', [
            'benchmark_detailed.py',
            '--config', 'config.yaml',
            '--model', 'model.safetensors',
            '--prompt', 'A beautiful sunset',
            '--resolution', '1024x576',
            '--steps', '50',
            '--num-frames', '32',
            '--num-gpus', '2',
            '--gpu-ids', '0,1',
            '--timeout', '300',
            '--retry', '2',
            '--output', 'results.json',
            '--json',
            '--csv',
            '--verbose'
        ]):
            args = parse_args()
            assert args.config == 'config.yaml'
            assert args.model == 'model.safetensors'
            assert args.prompt == 'A beautiful sunset'
            assert args.resolution == '1024x576'
            assert args.steps == 50
            assert args.num_frames == 32
            assert args.num_gpus == 2
            assert args.gpu_ids == '0,1'
            assert args.timeout == 300
            assert args.retry == 2
            assert args.output == 'results.json'
            assert args.json is True
            assert args.csv is True
            assert args.verbose is True
