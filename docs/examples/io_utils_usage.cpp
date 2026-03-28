// docs/examples/io_utils_usage.cpp
//
// Runnable examples demonstrating how to use the I/O utilities API
// for loading and saving .npy tensor files with different data types.
//
// Compile with: g++ -std=c++17 -I/path/to/src -I/path/to/ggml/include \
//               io_utils_usage.cpp -o io_utils_usage
//
// Note: This file is for documentation purposes. Requires test_io_utils.hpp
// and libnpy/npy.hpp to be available in the include path.

#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ggml.h"
#include "test_io_utils.hpp"

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// RAII Helpers
// ---------------------------------------------------------------------------

// RAII wrapper for ggml_context lifecycle
struct GGMLCtxRAII {
    struct ggml_context* ctx = nullptr;

    explicit GGMLCtxRAII(size_t mem_bytes = 64 * 1024 * 1024) {
        struct ggml_init_params params = {};
        params.mem_size   = mem_bytes;
        params.mem_buffer = nullptr;
        params.no_alloc   = false;  // allocate tensor data in-context
        ctx = ggml_init(params);
        if (!ctx) throw std::runtime_error("ggml_init failed");
    }

    ~GGMLCtxRAII() {
        if (ctx) ggml_free(ctx);
    }

    // Non-copyable
    GGMLCtxRAII(const GGMLCtxRAII&) = delete;
    GGMLCtxRAII& operator=(const GGMLCtxRAII&) = delete;
};

// RAII wrapper for temporary files
struct TempFile {
    std::string path;

    explicit TempFile(const std::string& name) {
        path = (fs::temp_directory_path() / name).string();
    }

    ~TempFile() {
        fs::remove(path);  // best-effort cleanup
    }
};

// ---------------------------------------------------------------------------
// Example 1: Round-trip Save and Load
// ---------------------------------------------------------------------------

void example_roundtrip_save_load() {
    std::cout << "\n=== Example 1: Round-trip Save and Load ===" << std::endl;

    try {
        // Create context
        GGMLCtxRAII ctx(64 * 1024 * 1024);

        // Create a 1D tensor with float32 data
        struct ggml_tensor* t1 = ggml_new_tensor_1d(ctx.ctx, GGML_TYPE_F32, 10);
        float* data = (float*)t1->data;
        for (int i = 0; i < 10; ++i) {
            data[i] = (float)i * 1.5f;
        }

        std::cout << "Created 1D tensor with 10 float32 values" << std::endl;

        // Save to .npy file
        TempFile tmp("example_roundtrip.npy");
        save_npy(tmp.path, t1);
        std::cout << "Saved to: " << tmp.path << std::endl;

        // Load back into a new context
        GGMLCtxRAII ctx2(64 * 1024 * 1024);
        struct ggml_tensor* t2 = load_npy(ctx2.ctx, tmp.path);

        std::cout << "Loaded tensor: type=" << t2->type << ", ne[0]=" << t2->ne[0] << std::endl;

        // Verify data matches
        const float* loaded_data = (float*)t2->data;
        bool match = true;
        for (int i = 0; i < 10; ++i) {
            if (data[i] != loaded_data[i]) {
                match = false;
                std::cout << "Mismatch at index " << i << ": " << data[i] << " != " << loaded_data[i] << std::endl;
            }
        }

        if (match) {
            std::cout << "Data matches perfectly after round-trip!" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------------
// Example 2: Multi-dimensional Tensors
// ---------------------------------------------------------------------------

void example_multidimensional_tensors() {
    std::cout << "\n=== Example 2: Multi-dimensional Tensors ===" << std::endl;

    try {
        GGMLCtxRAII ctx(64 * 1024 * 1024);

        // 2D Tensor (Matrix): 3 rows x 4 columns
        // In ggml: ne[0]=4 (cols), ne[1]=3 (rows)
        // In NumPy: shape (3, 4)
        std::cout << "\n2D Tensor (3x4 matrix):" << std::endl;
        struct ggml_tensor* t2d = ggml_new_tensor_2d(ctx.ctx, GGML_TYPE_F32, 4, 3);
        float* data2d = (float*)t2d->data;
        for (int i = 0; i < 12; ++i) {
            data2d[i] = (float)i;
        }
        std::cout << "  ggml: ne[0]=" << t2d->ne[0] << ", ne[1]=" << t2d->ne[1] << std::endl;
        std::cout << "  NumPy shape: (3, 4)" << std::endl;

        TempFile tmp2d("example_2d.npy");
        save_npy(tmp2d.path, t2d);

        GGMLCtxRAII ctx_load2d(64 * 1024 * 1024);
        struct ggml_tensor* t2d_loaded = load_npy(ctx_load2d.ctx, tmp2d.path);
        std::cout << "  Loaded: ne[0]=" << t2d_loaded->ne[0] << ", ne[1]=" << t2d_loaded->ne[1] << std::endl;

        // 3D Tensor
        // In ggml: ne[0]=2, ne[1]=3, ne[2]=4
        // In NumPy: shape (4, 3, 2)
        std::cout << "\n3D Tensor (4x3x2):" << std::endl;
        struct ggml_tensor* t3d = ggml_new_tensor_3d(ctx.ctx, GGML_TYPE_F32, 2, 3, 4);
        float* data3d = (float*)t3d->data;
        for (int i = 0; i < 24; ++i) {
            data3d[i] = (float)i * 0.1f;
        }
        std::cout << "  ggml: ne[0]=" << t3d->ne[0] << ", ne[1]=" << t3d->ne[1] << ", ne[2]=" << t3d->ne[2] << std::endl;
        std::cout << "  NumPy shape: (4, 3, 2)" << std::endl;

        TempFile tmp3d("example_3d.npy");
        save_npy(tmp3d.path, t3d);

        GGMLCtxRAII ctx_load3d(64 * 1024 * 1024);
        struct ggml_tensor* t3d_loaded = load_npy(ctx_load3d.ctx, tmp3d.path);
        std::cout << "  Loaded: ne[0]=" << t3d_loaded->ne[0] << ", ne[1]=" << t3d_loaded->ne[1] << ", ne[2]=" << t3d_loaded->ne[2] << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------------
// Example 3: Different Data Types
// ---------------------------------------------------------------------------

void example_different_dtypes() {
    std::cout << "\n=== Example 3: Different Data Types ===" << std::endl;

    try {
        GGMLCtxRAII ctx(64 * 1024 * 1024);

        // Float32 (F32)
        std::cout << "\nFloat32 (F32):" << std::endl;
        struct ggml_tensor* t_f32 = ggml_new_tensor_1d(ctx.ctx, GGML_TYPE_F32, 5);
        float* data_f32 = (float*)t_f32->data;
        for (int i = 0; i < 5; ++i) {
            data_f32[i] = (float)i * 1.5f;
        }
        TempFile tmp_f32("example_f32.npy");
        save_npy(tmp_f32.path, t_f32);
        std::cout << "  Saved F32 tensor (5 elements)" << std::endl;

        // Float16 (F16)
        std::cout << "\nFloat16 (F16):" << std::endl;
        struct ggml_tensor* t_f16 = ggml_new_tensor_1d(ctx.ctx, GGML_TYPE_F16, 5);
        ggml_fp16_t* data_f16 = (ggml_fp16_t*)t_f16->data;
        for (int i = 0; i < 5; ++i) {
            data_f16[i] = ggml_fp32_to_fp16((float)i * 0.5f);
        }
        TempFile tmp_f16("example_f16.npy");
        save_npy(tmp_f16.path, t_f16);
        std::cout << "  Saved F16 tensor (5 elements)" << std::endl;

        // Int32 (I32)
        std::cout << "\nInt32 (I32):" << std::endl;
        struct ggml_tensor* t_i32 = ggml_new_tensor_1d(ctx.ctx, GGML_TYPE_I32, 5);
        int32_t* data_i32 = (int32_t*)t_i32->data;
        for (int i = 0; i < 5; ++i) {
            data_i32[i] = (int32_t)(i * 100);
        }
        TempFile tmp_i32("example_i32.npy");
        save_npy(tmp_i32.path, t_i32);
        std::cout << "  Saved I32 tensor (5 elements)" << std::endl;

        // Int64 (I64)
        std::cout << "\nInt64 (I64):" << std::endl;
        struct ggml_tensor* t_i64 = ggml_new_tensor_1d(ctx.ctx, GGML_TYPE_I64, 5);
        int64_t* data_i64 = (int64_t*)t_i64->data;
        for (int i = 0; i < 5; ++i) {
            data_i64[i] = (int64_t)(i * 1000000);
        }
        TempFile tmp_i64("example_i64.npy");
        save_npy(tmp_i64.path, t_i64);
        std::cout << "  Saved I64 tensor (5 elements)" << std::endl;

        // Load all back and verify types
        std::cout << "\nLoading all tensors back:" << std::endl;
        GGMLCtxRAII ctx_load(64 * 1024 * 1024);

        auto t_f32_loaded = load_npy(ctx_load.ctx, tmp_f32.path);
        std::cout << "  F32 loaded: type=" << t_f32_loaded->type << std::endl;

        auto t_f16_loaded = load_npy(ctx_load.ctx, tmp_f16.path);
        std::cout << "  F16 loaded: type=" << t_f16_loaded->type << std::endl;

        auto t_i32_loaded = load_npy(ctx_load.ctx, tmp_i32.path);
        std::cout << "  I32 loaded: type=" << t_i32_loaded->type << std::endl;

        auto t_i64_loaded = load_npy(ctx_load.ctx, tmp_i64.path);
        std::cout << "  I64 loaded: type=" << t_i64_loaded->type << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------------
// Example 4: Error Handling
// ---------------------------------------------------------------------------

void example_error_handling() {
    std::cout << "\n=== Example 4: Error Handling ===" << std::endl;

    // Try loading non-existent file
    std::cout << "\nAttempting to load non-existent file:" << std::endl;
    try {
        GGMLCtxRAII ctx(64 * 1024 * 1024);
        auto t = load_npy(ctx.ctx, "/tmp/this_file_does_not_exist_wan_example.npy");
    } catch (const std::runtime_error& e) {
        std::cout << "  Caught expected error: " << e.what() << std::endl;
    }

    // Try saving null tensor
    std::cout << "\nAttempting to save null tensor:" << std::endl;
    try {
        save_npy("/tmp/should_not_exist.npy", nullptr);
    } catch (const std::runtime_error& e) {
        std::cout << "  Caught expected error: " << e.what() << std::endl;
    }

    // Try saving to invalid path
    std::cout << "\nAttempting to save to invalid path:" << std::endl;
    try {
        GGMLCtxRAII ctx(64 * 1024 * 1024);
        struct ggml_tensor* t = ggml_new_tensor_1d(ctx.ctx, GGML_TYPE_F32, 5);
        save_npy("/invalid/path/that/does/not/exist.npy", t);
    } catch (const std::runtime_error& e) {
        std::cout << "  Caught expected error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::cout << "WAN I/O Utilities Usage Examples" << std::endl;
    std::cout << "=================================" << std::endl;

    example_roundtrip_save_load();
    example_multidimensional_tensors();
    example_different_dtypes();
    example_error_handling();

    std::cout << "\n=== All examples completed ===" << std::endl;
    return 0;
}
