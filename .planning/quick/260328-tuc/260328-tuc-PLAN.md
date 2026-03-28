---
phase: quick
plan: 260328-tuc
type: execute
wave: 1
depends_on: []
files_modified:
  - tests/cpp/benchmark_inference.cpp
  - tests/cpp/CMakeLists.txt
autonomous: true
requirements: []
user_setup: []

must_haves:
  truths:
    - "Developers can measure inference latency for all 4 model types"
    - "Benchmark results show per-layer timing breakdown"
    - "Memory usage (peak, average) is tracked during inference"
    - "Results are exportable to CSV for analysis"
  artifacts:
    - path: "tests/cpp/benchmark_inference.cpp"
      provides: "Comprehensive inference benchmark tool for CLIP, T5, VAE, Transformer"
      min_lines: 200
    - path: "tests/cpp/CMakeLists.txt"
      provides: "Build target for benchmark_inference executable"
  key_links:
    - from: "tests/cpp/benchmark_inference.cpp"
      to: "src/model_registry.hpp"
      via: "model creation"
      pattern: "ModelRegistry::instance()->create"
    - from: "tests/cpp/benchmark_inference.cpp"
      to: "tests/cpp/test_framework.hpp"
      via: "timing utilities"
      pattern: "std::chrono"
---

<objective>
Create a unified inference efficiency benchmark tool that measures latency, throughput, and memory usage across all 4 model types (CLIP, T5, VAE, Transformer).

Purpose: Enable performance profiling and optimization tracking for each model variant.
Output: benchmark_inference executable with CSV export capability.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@tests/cpp/test_framework.hpp
@tests/cpp/test_helpers.hpp
@tests/cpp/test_clip.cpp
@tests/cpp/test_t5.cpp
@tests/cpp/test_vae.cpp
@tests/cpp/test_transformer.cpp
</context>

<tasks>

<task type="auto">
  <name>Task 1: Create benchmark_inference.cpp with timing infrastructure</name>
  <files>tests/cpp/benchmark_inference.cpp</files>
  <action>
Create tests/cpp/benchmark_inference.cpp with:

1. **Timing utilities:**
   - BenchmarkResult struct: model_name, version, input_shape, latency_ms, throughput_tokens_per_sec, peak_memory_mb, avg_memory_mb
   - TimerRAII class: auto-timing scope with chrono::high_resolution_clock
   - MemoryTracker class: track ggml_context memory usage before/after forward pass

2. **Benchmark function for each model type:**
   - benchmark_clip(version, num_runs, csv_output) — text input "a photo of a cat", measure latency
   - benchmark_t5(version, num_runs, csv_output) — text input "translate English to French: hello", measure latency
   - benchmark_vae(version, num_runs, csv_output) — random tensor input matching expected shape, measure latency
   - benchmark_transformer(version, num_runs, csv_output) — random latent input, measure latency

3. **Main function:**
   - Accept command-line args: --model {clip|t5|vae|transformer}, --version {version_str}, --runs {N}, --output {csv_file}
   - Default: run all models with 5 runs each, output to benchmark_results.csv
   - Print per-run latency to stdout, final summary to stderr

4. **CSV export:**
   - Header: model,version,run,latency_ms,throughput,peak_memory_mb,avg_memory_mb
   - One row per run per model
   - Append mode (don't overwrite existing results)

Use BackendRAII for backend lifecycle, model_registry for creation, test_framework.hpp assertions.
  </action>
  <verify>
    <automated>cd /data/zhongwang2/jtzhuang/projects/OnlyWan && cmake --build build && ./build/bin/benchmark_inference --help 2>&1 | grep -q "model\|version\|runs"</automated>
  </verify>
  <done>benchmark_inference.cpp compiles, accepts CLI args, runs without crashing on all 4 model types</done>
</task>

<task type="auto">
  <name>Task 2: Add benchmark_inference build target to CMakeLists.txt</name>
  <files>tests/cpp/CMakeLists.txt</files>
  <action>
Add to tests/cpp/CMakeLists.txt:

```cmake
# Benchmark tool: measure inference latency, throughput, memory for all models
add_executable(benchmark_inference benchmark_inference.cpp)
target_link_libraries(benchmark_inference PRIVATE wan-cpp)
target_include_directories(benchmark_inference PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(benchmark_inference PRIVATE cxx_std_17)
```

Place after the test_transformer target. Do NOT add to add_test() — benchmark is a utility, not a unit test.
  </action>
  <verify>
    <automated>cd /data/zhongwang2/jtzhuang/projects/OnlyWan && cmake --build build 2>&1 | grep -q "benchmark_inference" && test -f build/bin/benchmark_inference</automated>
  </verify>
  <done>benchmark_inference executable exists in build/bin/, CMakeLists.txt updated</done>
</task>

</tasks>

<verification>
1. Build succeeds: `cmake --build build` completes without errors
2. Executable exists: `ls -la build/bin/benchmark_inference`
3. Help works: `./build/bin/benchmark_inference --help` shows usage
4. Runs all models: `./build/bin/benchmark_inference --runs 2` completes for all 4 types
5. CSV output: `benchmark_results.csv` contains valid data with headers
</verification>

<success_criteria>
- benchmark_inference.cpp compiles and links successfully
- Tool accepts --model, --version, --runs, --output CLI arguments
- Runs inference on CLIP, T5, VAE, Transformer without crashing
- Outputs CSV with latency, throughput, memory metrics
- Results are reproducible across multiple runs
</success_criteria>

<output>
After completion, create `.planning/quick/260328-tuc/260328-tuc-SUMMARY.md`
</output>
