---
phase: 15
slug: multi-gpu-inference
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-18
---

# Phase 15 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | pytest 7.x + CUDA test harness |
| **Config file** | tests/conftest.py (Wave 0 installs) |
| **Quick run command** | `pytest tests/test_multi_gpu_basic.py -v` |
| **Full suite command** | `pytest tests/test_multi_gpu_*.py -v --tb=short` |
| **Estimated runtime** | ~120 seconds |

---

## Sampling Rate

- **After every task commit:** Run `pytest tests/test_multi_gpu_basic.py -v`
- **After every plan wave:** Run `pytest tests/test_multi_gpu_*.py -v --tb=short`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 15-01-01 | 01 | 1 | Multi-GPU API design | unit | `pytest tests/test_multi_gpu_api.py::test_wan_params_multi_gpu -v` | ❌ W0 | ⬜ pending |
| 15-01-02 | 01 | 1 | Data parallelism implementation | integration | `pytest tests/test_multi_gpu_data_parallel.py -v` | ❌ W0 | ⬜ pending |
| 15-02-01 | 02 | 1 | Tensor parallelism support | integration | `pytest tests/test_multi_gpu_tensor_parallel.py -v` | ❌ W0 | ⬜ pending |
| 15-03-01 | 03 | 2 | Performance benchmarking | benchmark | `pytest tests/test_multi_gpu_benchmark.py -v` | ❌ W0 | ⬜ pending |
| 15-03-02 | 03 | 2 | Numerical precision validation | unit | `pytest tests/test_multi_gpu_precision.py -v` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/test_multi_gpu_api.py` — stubs for API parameter validation
- [ ] `tests/test_multi_gpu_data_parallel.py` — data parallelism test cases
- [ ] `tests/test_multi_gpu_tensor_parallel.py` — tensor parallelism test cases
- [ ] `tests/test_multi_gpu_benchmark.py` — performance benchmark harness
- [ ] `tests/test_multi_gpu_precision.py` — numerical precision verification
- [ ] `tests/conftest.py` — CUDA device detection, multi-GPU fixtures
- [ ] pytest 7.x installed with CUDA support

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Multi-GPU inference produces semantically identical results | Numerical precision < 0.01% deviation | Requires actual multi-GPU hardware and visual inspection | Run T2V generation on 1 GPU vs 2 GPUs, compare output frames pixel-by-pixel |
| GPU communication bandwidth utilization | Communication efficiency > 80% | Requires NVIDIA Nsight Systems profiling | Profile with `nsys profile --gpu-metrics-device=all` and analyze AllReduce operations |
| Graceful GPU failure handling | Error recovery on GPU failure | Requires simulated GPU failure injection | Use CUDA_VISIBLE_DEVICES to simulate GPU removal mid-inference |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
