import pytest
import subprocess
import os


@pytest.fixture(scope="session")
def gpu_count():
    """Detect available CUDA GPUs using nvidia-smi."""
    try:
        result = subprocess.run(
            ["nvidia-smi", "-L"],
            capture_output=True,
            text=True,
            timeout=5
        )
        if result.returncode == 0:
            lines = [line for line in result.stdout.strip().split('\n') if line]
            return len(lines)
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    # Fallback: check /proc/driver/nvidia/gpus/
    try:
        gpu_dir = "/proc/driver/nvidia/gpus"
        if os.path.isdir(gpu_dir):
            return len([d for d in os.listdir(gpu_dir) if os.path.isdir(os.path.join(gpu_dir, d))])
    except (FileNotFoundError, PermissionError):
        pass

    return 0


@pytest.fixture(scope="session")
def skip_if_no_cuda(gpu_count):
    """Skip test if no CUDA GPUs available."""
    if gpu_count == 0:
        pytest.skip("No CUDA GPUs available")


@pytest.fixture(scope="session")
def skip_if_single_gpu(gpu_count):
    """Skip test if fewer than 2 GPUs available."""
    if gpu_count < 2:
        pytest.skip(f"Multi-GPU test requires 2+ GPUs, found {gpu_count}")


@pytest.fixture(scope="session")
def wan_binary():
    """Path to the wan-cli binary."""
    # Assume build output is in build/ or build_cuda/
    candidates = [
        "/home/jtzhuang/projects/stable-diffusion.cpp/wan/build/bin/wan-cli",
        "/home/jtzhuang/projects/stable-diffusion.cpp/wan/build_cuda/bin/wan-cli",
        "/home/jtzhuang/projects/stable-diffusion.cpp/wan/build/wan-cli",
        "/home/jtzhuang/projects/stable-diffusion.cpp/wan/build_cuda/wan-cli",
    ]

    for path in candidates:
        if os.path.isfile(path) and os.access(path, os.X_OK):
            return path

    pytest.skip("wan-cli binary not found in build directories")


@pytest.fixture(scope="session")
def model_path():
    """Path to test model files."""
    # Placeholder - actual path will be configured per environment
    model_dir = os.environ.get("WAN_MODEL_PATH", "/home/jtzhuang/projects/stable-diffusion.cpp/wan/models")
    if not os.path.isdir(model_dir):
        pytest.skip(f"Model directory not found: {model_dir}")
    return model_dir
