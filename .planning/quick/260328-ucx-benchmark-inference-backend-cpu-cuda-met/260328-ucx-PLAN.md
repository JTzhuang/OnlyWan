---
quick_task: 260328-ucx
title: Add backend selection to benchmark_inference tool
description: Add --backend parameter supporting cpu/cuda/metal to benchmark_inference
created: 2026-03-28
target_completion: 2026-03-28
files_modified:
  - tests/cpp/benchmark_inference.cpp
autonomous: true
---

<objective>
Add --backend parameter to benchmark_inference tool to support CPU, CUDA, and Metal backends for performance comparison across different compute devices.

Purpose: Enable benchmarking on different hardware backends to identify performance characteristics per device.
Output: benchmark_inference binary with --backend {cpu|cuda|metal} support.
</objective>

<execution_context>
@/data/zhongwang2/jtzhuang/projects/OnlyWan/src/ggml_extend.hpp (backend initialization patterns)
@/data/zhongwang2/jtzhuang/projects/OnlyWan/tests/cpp/benchmark_inference.cpp (current implementation)
</execution_context>

<context>
Current state:
- benchmark_inference.cpp hardcodes CPU backend via ggml_backend_cpu_init()
- All benchmark functions (CLIP, T5, VAE, Transformer) use BackendRAII guard with CPU backend
- ggml_extend.hpp shows backend support: SD_USE_CUDA, SD_USE_METAL, SD_USE_VULKAN, SD_USE_OPENCL, SD_USE_SYCL

Backend initialization patterns from codebase:
- CPU: ggml_backend_cpu_init()
- CUDA: ggml_backend_cuda_init() (when SD_USE_CUDA defined)
- Metal: ggml_backend_metal_init() (when SD_USE_METAL defined)
- Fallback: Always available CPU backend
</context>

<task type="auto">
  <name>Task 1: Add backend parameter parsing and initialization</name>
  <files>tests/cpp/benchmark_inference.cpp</files>
  <action>
1. Add --backend parameter to command-line parsing (after --output parameter)
   - Accept values: cpu, cuda, metal
   - Default to "cpu"
   - Validate backend string against available options

2. Create backend_from_string() helper function that:
   - Takes backend name string
   - Returns ggml_backend_t
   - Checks compile-time flags (SD_USE_CUDA, SD_USE_METAL)
   - Falls back to CPU if requested backend unavailable
   - Logs which backend was selected

3. Modify all benchmark functions (benchmark_clip, benchmark_t5, benchmark_vae, benchmark_transformer):
   - Replace hardcoded ggml_backend_cpu_init() with backend_from_string(backend_name)
   - Pass backend_name as parameter to each benchmark function
   - Update function signatures to accept backend_name parameter

4. Update help text to document --backend option with available choices
  </action>
  <verify>
    <automated>
cd /data/zhongwang2/jtzhuang/projects/OnlyWan && \
./build/bin/benchmark_inference --help | grep -A 1 "backend"
    </automated>
  </verify>
  <done>
- benchmark_inference accepts --backend {cpu|cuda|metal} parameter
- Help text documents backend option
- Backend selection logic respects compile-time flags
- All benchmark functions use selected backend
- Default backend is CPU
  </done>
</task>

<task type="auto">
  <name>Task 2: Build and verify backend selection works</name>
  <files>tests/cpp/benchmark_inference.cpp</files>
  <action>
1. Build the project:
   cd /data/zhongwang2/jtzhuang/projects/OnlyWan && cmake --build build -j4

2. Test backend parameter parsing:
   - Run with --backend cpu (should work)
   - Run with --backend cuda (should work if CUDA enabled, fallback to CPU otherwise)
   - Run with --backend metal (should work if Metal enabled, fallback to CPU otherwise)
   - Run with invalid backend (should error or fallback)

3. Verify output shows which backend was selected in stderr messages
  </action>
  <verify>
    <automated>
cd /data/zhongwang2/jtzhuang/projects/OnlyWan/build && \
./bin/benchmark_inference --backend cpu --model clip --runs 1 2>&1 | head -20
    </automated>
  </verify>
  <done>
- Binary builds without errors
- --backend parameter is recognized
- Backend selection is logged to stderr
- Benchmarks run on selected backend
  </done>
</task>

<success_criteria>
- benchmark_inference accepts --backend parameter with cpu/cuda/metal values
- Help text documents the new parameter
- Backend selection respects compile-time availability flags
- All benchmark functions use the selected backend
- Tool logs which backend was selected during execution
- Default behavior (no --backend flag) uses CPU
</success_criteria>

<output>
After completion, create `.planning/quick/260328-ucx-benchmark-inference-backend-cpu-cuda-met/260328-ucx-SUMMARY.md`
</output>
