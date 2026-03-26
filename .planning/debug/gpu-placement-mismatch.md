---
status: resolved
trigger: "GPU computation on wrong device - gpu_ids=1 but GPU 0 has memory usage"
created: 2026-03-26T05:51:44Z
updated: 2026-03-26T06:02:40Z
symptoms_prefilled: true
goal: find_and_fix
---

## Current Focus

hypothesis: RESOLVED
test: rebuild and run CLI with config specifying gpu_ids=[1]
expecting: GPU 1 should have model weights and computation
result: SUCCESS - GPU 1 now has 23073 MiB, computation runs at 93% GPU-Util on GPU 1

Verified evidence:
- nvidia-smi shows GPU 1 with wan-cli process using 23064 MiB
- Log shows "Using GPU device 1 from config"
- Log shows "Backend initialized: CUDA1"
- GPU 1 utilization at 93% during inference step

## Symptoms

expected: Computation on GPU 1 (gpu_ids=1), memory usage on GPU 1
actual: Computation on CPU, memory usage on GPU 0
errors: None explicit
reproduction: ./build_cuda/bin/wan-cli ./models/config.json -p "a cat is playing basketball" --vocab-dir ./models/google/umt5-xxl/ && nvidia-smi
started: After CUDA compilation enabled

## Eliminated

## Evidence

- timestamp: 2026-03-26T05:52:00Z
  checked: config.json structure
  found: "gpu_ids": [1] present in config, correctly read by ConfigLoader
  implication: config is valid; issue is downstream

- timestamp: 2026-03-26T05:52:15Z
  checked: wan_load_model() at line 428 in wan-api.cpp
  found: |
    ctx->backend = WanBackendPtr(Wan::WanBackend::create(ctx->backend_type, n_threads, 0));
  implication: CRITICAL BUG - hardcoded device_id=0 passed, ignoring ctx->params.gpu_ids

- timestamp: 2026-03-26T05:52:30Z
  checked: WanBackend::create() signature at wan_loader.cpp:101
  found: WanBackend::create(const std::string& type, int n_threads, int device_id = 0)
  implication: device_id parameter IS supported, but wan_load_model() never uses it

- timestamp: 2026-03-26T05:57:30Z
  checked: GPU memory usage after fix
  found: GPU 0 has 95062 MiB (still most), GPU 1 has 583 MiB (newly allocated)
  implication: Fix partially works - GPU 1 is now being used, but main weights still on GPU 0

PROBLEM IDENTIFIED: Runners created with CPU backend during load, weights loaded to CPU/GPU 0. Then GPU 1 backend created but runners still use old backend. Need to pass correct device_id to WanModel::load() or recreate runners with GPU 1 backend.

## Resolution

root_cause: gpu_ids from config.json were not being used to select GPU device. The problem occurred in two places:
  1. wan_load_model() at line 428 was calling WanBackend::create() with hardcoded device_id=0
  2. WanModel::load() at line 191 was always using CPU backend for loading, regardless of target device
  Result: All model weights loaded to GPU 0, even when config specified gpu_ids=[1]

fix: Three-part fix applied:
  1. Modified wan-internal.hpp: WanModel::load() signature to accept device_id parameter (line 94)
  2. Modified wan-api.cpp: Updated WanModel::load() implementation (lines 128-130, 189-207)
     - Accept device_id parameter
     - Use CUDA backend for specified device_id instead of always CPU
     - Fallback to CPU only if CUDA not available or device_id < 0
  3. Modified wan-api.cpp: Extract gpu_ids early in wan_load_model() (lines 415-420)
     - Get first gpu_id from config before loading model
     - Pass device_id through to WanModel::load()

verification: VERIFIED
  - Binary rebuilt successfully
  - Ran CLI command with config specifying gpu_ids=[1]
  - nvidia-smi confirms: GPU 1 has 23073 MiB with wan-cli process
  - Logs show: "Using GPU device 1 from config", "Backend initialized: CUDA1"
  - GPU 1 utilization at 93% during inference

files_changed: [include/wan-cpp/wan-internal.hpp, src/api/wan-api.cpp]

## Commit

Hash: c5ce988
Message: "fix: use gpu_ids from config.json for device placement"

The fix has been successfully applied, tested, and committed.
