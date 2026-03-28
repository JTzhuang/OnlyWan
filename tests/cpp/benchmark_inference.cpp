#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_registry.hpp"
#include "clip.hpp"
#include "t5.hpp"
#include "wan.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

#ifdef WAN_USE_CUDA
#include "ggml-cuda.h"
#endif

#ifdef WAN_USE_METAL
#include "ggml-metal.h"
#endif

#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <cmath>
#include <sstream>

// Force-link model_factory.cpp to ensure static registrations are not DCE'd by linker.
extern "C" void wan_force_model_registrations();

// ---------------------------------------------------------------------------
// Benchmark Result Structure
// ---------------------------------------------------------------------------

struct BenchmarkResult {
    std::string model_name;
    std::string version;
    std::string input_shape;
    double latency_ms;
    double throughput_tokens_per_sec;
    double peak_memory_mb;
    double avg_memory_mb;

    std::string to_csv_row() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << model_name << ","
            << version << ","
            << input_shape << ","
            << latency_ms << ","
            << throughput_tokens_per_sec << ","
            << peak_memory_mb << ","
            << avg_memory_mb;
        return oss.str();
    }
};

// ---------------------------------------------------------------------------
// Timing Utilities
// ---------------------------------------------------------------------------

class TimerRAII {
public:
    explicit TimerRAII(double& elapsed_ms)
        : elapsed_ms_(elapsed_ms),
          start_(std::chrono::high_resolution_clock::now()) {}

    ~TimerRAII() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        elapsed_ms_ = duration.count() / 1000.0;
    }

private:
    double& elapsed_ms_;
    std::chrono::high_resolution_clock::time_point start_;
};

// ---------------------------------------------------------------------------
// Memory Tracking
// ---------------------------------------------------------------------------

class MemoryTracker {
public:
    MemoryTracker() : peak_mb_(0.0), avg_mb_(0.0) {}

    void record_peak(const GGMLRunnerContext& ctx) {
        if (!ctx.ggml_ctx) return;
        size_t used = ggml_used_mem(ctx.ggml_ctx);
        double used_mb = used / (1024.0 * 1024.0);
        if (used_mb > peak_mb_) {
            peak_mb_ = used_mb;
        }
    }

    void finalize(int num_samples) {
        if (num_samples > 0) {
            avg_mb_ = peak_mb_ / num_samples;
        }
    }

    double peak_mb() const { return peak_mb_; }
    double avg_mb() const { return avg_mb_; }

private:
    double peak_mb_;
    double avg_mb_;
};

// ---------------------------------------------------------------------------
// Backend Selection Helper
// ---------------------------------------------------------------------------

ggml_backend_t backend_from_string(const std::string& backend_name, int device_id = 0) {
    if (backend_name == "cpu") {
        std::cerr << "[Backend] Selected: CPU\n";
        return ggml_backend_cpu_init();
    }
#ifdef WAN_USE_CUDA
    else if (backend_name == "cuda") {
        std::cerr << "[Backend] Selected: CUDA (device " << device_id << ")\n";
        return ggml_backend_cuda_init(device_id);
    }
#endif
#ifdef WAN_USE_METAL
    else if (backend_name == "metal") {
        std::cerr << "[Backend] Selected: Metal\n";
        return ggml_backend_metal_init();
    }
#endif
    else {
        std::cerr << "[Backend] Unknown backend '" << backend_name << "', falling back to CPU\n";
        return ggml_backend_cpu_init();
    }
}

// ---------------------------------------------------------------------------
// CLIP Vision Benchmark
// ---------------------------------------------------------------------------

BenchmarkResult benchmark_clip(const std::string& version, int num_runs, MemoryTracker& mem_tracker, const std::string& backend_name, int device_id = 0) {
    wan_force_model_registrations();
    BackendRAII guard(backend_from_string(backend_name, device_id));
    String2TensorStorage empty_map{};

    auto runner = ModelRegistry::instance()->create<CLIPVisionModelProjectionRunner>(
        version, guard.backend, false, empty_map, "");
    if (!runner) {
        throw std::runtime_error("Failed to create CLIP Vision runner for version: " + version);
    }

    runner->alloc_params_buffer();

    std::vector<double> latencies;
    double total_latency = 0.0;

    for (int i = 0; i < num_runs; ++i) {
        // Create fresh context for each run
        ggml_init_params params{512*1024*1024, nullptr, false};
        ggml_context* ctx = ggml_init(params);

        // Create input: pixel_values [3, 224, 224] (RGB image)
        ggml_tensor* pixel_values = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, 224, 224, 3);
        float* pv_data = (float*)pixel_values->data;
        for (int j = 0; j < 224*224*3; ++j) pv_data[j] = 0.5f;

        // Create GGMLRunnerContext
        GGMLRunnerContext runner_ctx;
        runner_ctx.ggml_ctx = ctx;
        runner_ctx.backend = guard.backend;

        double elapsed_ms = 0.0;
        {
            TimerRAII timer(elapsed_ms);
            auto output = runner->forward(&runner_ctx, pixel_values, false, -1);
            mem_tracker.record_peak(runner_ctx);
        }

        latencies.push_back(elapsed_ms);
        total_latency += elapsed_ms;
        std::cerr << "  Run " << (i + 1) << ": " << std::fixed << std::setprecision(2)
                  << elapsed_ms << " ms\n";

        ggml_free(ctx);
    }

    double avg_latency = total_latency / num_runs;
    double throughput = (avg_latency > 0.0) ? (1.0 / avg_latency) * 1000.0 : 0.0;  // images per second

    mem_tracker.finalize(num_runs);

    BenchmarkResult result;
    result.model_name = "CLIP Vision";
    result.version = version;
    result.input_shape = "224,224,3";
    result.latency_ms = avg_latency;
    result.throughput_tokens_per_sec = throughput;
    result.peak_memory_mb = mem_tracker.peak_mb();
    result.avg_memory_mb = mem_tracker.avg_mb();

    return result;
}

// ---------------------------------------------------------------------------
// T5 Benchmark
// ---------------------------------------------------------------------------

BenchmarkResult benchmark_t5(const std::string& version, int num_runs, MemoryTracker& mem_tracker, const std::string& backend_name, int device_id = 0) {
    wan_force_model_registrations();
    BackendRAII guard(backend_from_string(backend_name, device_id));
    String2TensorStorage empty_map{};

    auto runner = ModelRegistry::instance()->create<T5Runner>(
        version, guard.backend, false, empty_map, "");
    if (!runner) {
        throw std::runtime_error("Failed to create T5 runner for version: " + version);
    }

    runner->alloc_params_buffer();

    std::vector<double> latencies;
    double total_latency = 0.0;

    for (int i = 0; i < num_runs; ++i) {
        // Create fresh context for each run
        ggml_init_params params{256*1024*1024, nullptr, false};
        ggml_context* ctx = ggml_init(params);

        // Create input: [1, 512] token IDs
        ggml_tensor* input_ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 512, 1);
        int32_t* ids = (int32_t*)input_ids->data;
        for (int j = 0; j < 512; ++j) ids[j] = (j % 32128);

        // Create pos_bucket: [512, 512]
        ggml_tensor* pos_bucket = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 512, 512);
        int32_t* pb_data = (int32_t*)pos_bucket->data;
        for (int j = 0; j < 512 * 512; ++j) pb_data[j] = 0;

        // Create GGMLRunnerContext
        GGMLRunnerContext runner_ctx;
        runner_ctx.ggml_ctx = ctx;
        runner_ctx.backend = guard.backend;

        double elapsed_ms = 0.0;
        {
            TimerRAII timer(elapsed_ms);
            auto output = runner->forward(&runner_ctx, input_ids, pos_bucket, nullptr);
            mem_tracker.record_peak(runner_ctx);
        }

        latencies.push_back(elapsed_ms);
        total_latency += elapsed_ms;
        std::cerr << "  Run " << (i + 1) << ": " << std::fixed << std::setprecision(2)
                  << elapsed_ms << " ms\n";

        ggml_free(ctx);
    }

    double avg_latency = total_latency / num_runs;
    double throughput = (avg_latency > 0.0) ? (512.0 / avg_latency) * 1000.0 : 0.0;  // tokens per second

    mem_tracker.finalize(num_runs);

    BenchmarkResult result;
    result.model_name = "T5";
    result.version = version;
    result.input_shape = "512";
    result.latency_ms = avg_latency;
    result.throughput_tokens_per_sec = throughput;
    result.peak_memory_mb = mem_tracker.peak_mb();
    result.avg_memory_mb = mem_tracker.avg_mb();

    return result;
}

// ---------------------------------------------------------------------------
// VAE Benchmark
// ---------------------------------------------------------------------------

BenchmarkResult benchmark_vae(const std::string& version, int num_runs, MemoryTracker& mem_tracker, const std::string& backend_name, int device_id = 0) {
    wan_force_model_registrations();
    BackendRAII guard(backend_from_string(backend_name, device_id));
    String2TensorStorage empty_map{};

    auto runner = ModelRegistry::instance()->create<WAN::WanVAERunner>(
        version, guard.backend, false, empty_map, "");
    if (!runner) {
        throw std::runtime_error("Failed to create VAE runner for version: " + version);
    }

    runner->alloc_params_buffer();

    std::vector<double> latencies;
    double total_latency = 0.0;

    for (int i = 0; i < num_runs; ++i) {
        // Create fresh context for each run
        ggml_init_params params{100*1024*1024, nullptr, false};
        ggml_context* ctx = ggml_init(params);

        // Create random latent: [8, 8, 1, 16] (w,h,t,c) in ggml layout
        ggml_tensor* z = ggml_new_tensor_4d(ctx, GGML_TYPE_F32, 8, 8, 1, 16);
        float* z_data = (float*)z->data;
        for (int j = 0; j < 16*1*8*8; ++j) z_data[j] = (float)(j % 100) * 0.01f - 0.5f;

        double elapsed_ms = 0.0;
        {
            TimerRAII timer(elapsed_ms);
            ggml_tensor* decoded = nullptr;
            bool ok = runner->compute(4, z, true, &decoded, ctx);
            if (!ok) {
                throw std::runtime_error("VAE compute failed");
            }
            GGMLRunnerContext runner_ctx;
            runner_ctx.ggml_ctx = ctx;
            runner_ctx.backend = guard.backend;
            mem_tracker.record_peak(runner_ctx);
        }

        latencies.push_back(elapsed_ms);
        total_latency += elapsed_ms;
        std::cerr << "  Run " << (i + 1) << ": " << std::fixed << std::setprecision(2)
                  << elapsed_ms << " ms\n";

        ggml_free(ctx);
    }

    double avg_latency = total_latency / num_runs;
    double throughput = (avg_latency > 0.0) ? (1.0 / avg_latency) * 1000.0 : 0.0;  // frames per second

    mem_tracker.finalize(num_runs);

    BenchmarkResult result;
    result.model_name = "VAE";
    result.version = version;
    result.input_shape = "8,8,1,16";
    result.latency_ms = avg_latency;
    result.throughput_tokens_per_sec = throughput;
    result.peak_memory_mb = mem_tracker.peak_mb();
    result.avg_memory_mb = mem_tracker.avg_mb();

    return result;
}

// ---------------------------------------------------------------------------
// Transformer (WAN) Benchmark
// ---------------------------------------------------------------------------

BenchmarkResult benchmark_transformer(const std::string& version, int num_runs, MemoryTracker& mem_tracker, const std::string& backend_name, int device_id = 0) {
    wan_force_model_registrations();
    BackendRAII guard(backend_from_string(backend_name, device_id));
    String2TensorStorage empty_map{};

    // Add dummy block tensor to ensure num_layers is detected
    empty_map["blocks.39.weight"] = TensorStorage();

    auto runner = ModelRegistry::instance()->create<WAN::WanRunner>(
        version, guard.backend, false, empty_map, "");
    if (!runner) {
        throw std::runtime_error("Failed to create Transformer runner for version: " + version);
    }

    runner->alloc_params_buffer();

    std::vector<double> latencies;
    double total_latency = 0.0;

    for (int i = 0; i < num_runs; ++i) {
        // Create fresh context for each run
        ggml_init_params params{512*1024*1024, nullptr, false};
        ggml_context* ctx = ggml_init(params);

        // Create latent x: [ch=8, w=8, h=2, frames=16] matching test_transformer.cpp
        ggml_tensor* x = ggml_new_tensor_4d(ctx, GGML_TYPE_F32, 80, 80, 2, 81);
        float* x_data = (float*)x->data;
        for (size_t j = 0; j < 8*8*2*16; ++j) x_data[j] = 0.1f;

        // Create timestep: [1]
        ggml_tensor* timesteps = ggml_new_tensor_1d(ctx, GGML_TYPE_I32, 1);
        int32_t* ts_data = (int32_t*)timesteps->data;
        ts_data[0] = 500;

        // Create context (text encoder output): [4096, 77, 1]
        ggml_tensor* context = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, 4096, 77, 1);
        float* ctx_data = (float*)context->data;
        for (size_t j = 0; j < 4096*77; ++j) ctx_data[j] = 0.1f;

        double elapsed_ms = 0.0;
        {
            TimerRAII timer(elapsed_ms);
            ggml_tensor* output = nullptr;
            bool ok = runner->compute(4, x, timesteps, context, nullptr, nullptr, nullptr, nullptr, 1.0f, &output, ctx);
            if (!ok) {
                throw std::runtime_error("Transformer compute failed");
            }
            GGMLRunnerContext runner_ctx;
            runner_ctx.ggml_ctx = ctx;
            runner_ctx.backend = guard.backend;
            mem_tracker.record_peak(runner_ctx);
        }

        latencies.push_back(elapsed_ms);
        total_latency += elapsed_ms;
        std::cerr << "  Run " << (i + 1) << ": " << std::fixed << std::setprecision(2)
                  << elapsed_ms << " ms\n";

        ggml_free(ctx);
    }

    double avg_latency = total_latency / num_runs;
    double throughput = (avg_latency > 0.0) ? (1.0 / avg_latency) * 1000.0 : 0.0;  // iterations per second

    mem_tracker.finalize(num_runs);

    BenchmarkResult result;
    result.model_name = "Transformer";
    result.version = version;
    result.input_shape = "8,8,2,16";
    result.latency_ms = avg_latency;
    result.throughput_tokens_per_sec = throughput;
    result.peak_memory_mb = mem_tracker.peak_mb();
    result.avg_memory_mb = mem_tracker.avg_mb();

    return result;
}

// ---------------------------------------------------------------------------
// CSV Export
// ---------------------------------------------------------------------------

void export_to_csv(const std::vector<BenchmarkResult>& results, const std::string& output_file) {
    std::ofstream file(output_file, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open CSV file: " + output_file);
    }

    // Write header if file is empty
    file.seekp(0, std::ios::end);
    if (file.tellp() == 0) {
        file << "model,version,input_shape,latency_ms,throughput,peak_memory_mb,avg_memory_mb\n";
    }

    for (const auto& result : results) {
        file << result.to_csv_row() << "\n";
    }

    file.close();
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    std::string model_type = "all";
    std::string version = "";
    int num_runs = 5;
    std::string output_file = "benchmark_results.csv";
    std::string backend = "cpu";
    int device_id = 0;
    bool show_help = false;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            show_help = true;
        } else if (arg == "--model" && i + 1 < argc) {
            model_type = argv[++i];
        } else if (arg == "--version" && i + 1 < argc) {
            version = argv[++i];
        } else if (arg == "--runs" && i + 1 < argc) {
            num_runs = std::stoi(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--backend" && i + 1 < argc) {
            backend = argv[++i];
        } else if (arg == "--device" && i + 1 < argc) {
            device_id = std::stoi(argv[++i]);
        }
    }

    if (show_help) {
        std::cout << "Usage: benchmark_inference [options]\n"
                  << "Options:\n"
                  << "  --model {clip|t5|vae|transformer|all}  Model type to benchmark (default: all)\n"
                  << "  --version VERSION_STR                   Model version string\n"
                  << "  --runs N                                Number of runs per model (default: 5)\n"
                  << "  --output FILE                           CSV output file (default: benchmark_results.csv)\n"
                  << "  --backend {cpu|cuda|metal}              Compute backend (default: cpu)\n"
                  << "  --device ID                             GPU device ID for CUDA backend (default: 0)\n"
                  << "  --help                                  Show this help message\n";
        return 0;
    }

    std::vector<BenchmarkResult> all_results;

    try {
        // CLIP Vision benchmarks - skip for now due to large memory requirements
        // if (model_type == "all" || model_type == "clip") {
        //     std::cerr << "\n=== Benchmarking CLIP Vision ===\n";
        //     ...
        // }

        // T5 benchmarks
        if (model_type == "all" || model_type == "t5") {
            std::cerr << "\n=== Benchmarking T5 ===\n";
            const std::string t5_versions[] = {"t5-standard", "t5-umt5"};
            for (const auto& v : t5_versions) {
                std::cerr << "Version: " << v << "\n";
                MemoryTracker mem;
                auto result = benchmark_t5(v, num_runs, mem, backend, device_id);
                all_results.push_back(result);
                std::cout << "T5 " << v << ": " << std::fixed << std::setprecision(2)
                          << result.latency_ms << " ms avg\n";
            }
        }

        // VAE benchmarks
        if (model_type == "all" || model_type == "vae") {
            std::cerr << "\n=== Benchmarking VAE ===\n";
            const std::string vae_versions[] = {"wan-vae-t2v", "wan-vae-t2v-decode", "wan-vae-i2v", "wan-vae-ti2v"};
            for (const auto& v : vae_versions) {
                std::cerr << "Version: " << v << "\n";
                MemoryTracker mem;
                auto result = benchmark_vae(v, num_runs, mem, backend, device_id);
                all_results.push_back(result);
                std::cout << "VAE " << v << ": " << std::fixed << std::setprecision(2)
                          << result.latency_ms << " ms avg\n";
            }
        }

        // Transformer benchmarks
        if (model_type == "all" || model_type == "transformer") {
            std::cerr << "\n=== Benchmarking Transformer ===\n";
            const std::string transformer_versions[] = {"wan-runner-t2v", "wan-runner-i2v", "wan-runner-ti2v"};
            for (const auto& v : transformer_versions) {
                std::cerr << "Version: " << v << "\n";
                MemoryTracker mem;
                auto result = benchmark_transformer(v, num_runs, mem, backend, device_id);
                all_results.push_back(result);
                std::cout << "Transformer " << v << ": " << std::fixed << std::setprecision(2)
                          << result.latency_ms << " ms avg\n";
            }
        }

        // Export results to CSV
        export_to_csv(all_results, output_file);
        std::cout << "\nResults exported to: " << output_file << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
