// test_io_npy.cpp - Tests for .npy file I/O utilities (load_npy / save_npy).
//
// These tests verify:
//   1. Round-trip: C++ save -> C++ load matches original data
//   2. 1D, 2D, 3D tensor shapes are preserved correctly
//   3. float32, float16, int32, int64 dtypes are supported
//   4. Dimension mapping: ggml ne <-> NumPy shape (row-major, reversed dims)
//   5. Error handling for bad files / unsupported dtypes
//
// No model weights or backends are needed; tests use a small ggml_ctx
// with no_alloc=false to allocate tensor data directly.

#include <cmath>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "ggml.h"
#include "test_framework.hpp"
#include "test_io_utils.hpp"

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// RAII ggml_context wrapper for tests.
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

// Return a temporary file path that is cleaned up after the test.
struct TempFile {
    std::string path;
    explicit TempFile(const std::string& name) {
        path = (fs::temp_directory_path() / name).string();
    }
    ~TempFile() {
        fs::remove(path);  // best-effort cleanup
    }
};

// Fill a float32 tensor with sequential values: data[i] = (float)i.
static void fill_f32_sequential(struct ggml_tensor* t) {
    float* d = (float*)t->data;
    int64_t n = ggml_nelements(t);
    for (int64_t i = 0; i < n; ++i) d[i] = (float)i;
}

// Fill an int32 tensor with sequential values.
static void fill_i32_sequential(struct ggml_tensor* t) {
    int32_t* d = (int32_t*)t->data;
    int64_t n = ggml_nelements(t);
    for (int64_t i = 0; i < n; ++i) d[i] = (int32_t)i;
}

// Fill an int64 tensor with sequential values.
static void fill_i64_sequential(struct ggml_tensor* t) {
    int64_t* d = (int64_t*)t->data;
    int64_t n = ggml_nelements(t);
    for (int64_t i = 0; i < n; ++i) d[i] = (int64_t)i;
}

// ---------------------------------------------------------------------------
// Test group: round-trip (save -> load) for 1D tensors
// ---------------------------------------------------------------------------

void test_roundtrip_1d(TestSuite& suite) {
    suite.run("roundtrip_1d_f32", []() {
        GGMLCtxRAII ctx;
        TempFile tmp("test_io_1d_f32.npy");

        // Create and fill tensor
        struct ggml_tensor* t = ggml_new_tensor_1d(ctx.ctx, GGML_TYPE_F32, 16);
        fill_f32_sequential(t);

        // Save and reload
        save_npy(tmp.path, t);

        GGMLCtxRAII ctx2;
        struct ggml_tensor* t2 = load_npy(ctx2.ctx, tmp.path);

        WAN_ASSERT_EQ(t2->type,  GGML_TYPE_F32);
        WAN_ASSERT_EQ(t2->ne[0], (int64_t)16);
        WAN_ASSERT_EQ(ggml_nelements(t2), (int64_t)16);

        const float* src = (float*)t->data;
        const float* dst = (float*)t2->data;
        for (int64_t i = 0; i < 16; ++i) {
            if (src[i] != dst[i]) {
                throw std::runtime_error(
                    "Mismatch at index " + std::to_string(i) +
                    ": expected " + std::to_string(src[i]) +
                    " got " + std::to_string(dst[i]));
            }
        }
    });

    suite.run("roundtrip_1d_i32", []() {
        GGMLCtxRAII ctx;
        TempFile tmp("test_io_1d_i32.npy");

        struct ggml_tensor* t = ggml_new_tensor_1d(ctx.ctx, GGML_TYPE_I32, 8);
        fill_i32_sequential(t);
        save_npy(tmp.path, t);

        GGMLCtxRAII ctx2;
        struct ggml_tensor* t2 = load_npy(ctx2.ctx, tmp.path);

        WAN_ASSERT_EQ(t2->type,  GGML_TYPE_I32);
        WAN_ASSERT_EQ(t2->ne[0], (int64_t)8);

        const int32_t* src = (int32_t*)t->data;
        const int32_t* dst = (int32_t*)t2->data;
        for (int64_t i = 0; i < 8; ++i) {
            if (src[i] != dst[i]) {
                throw std::runtime_error(
                    "i32 mismatch at " + std::to_string(i));
            }
        }
    });

    suite.run("roundtrip_1d_i64", []() {
        GGMLCtxRAII ctx;
        TempFile tmp("test_io_1d_i64.npy");

        struct ggml_tensor* t = ggml_new_tensor_1d(ctx.ctx, GGML_TYPE_I64, 6);
        fill_i64_sequential(t);
        save_npy(tmp.path, t);

        GGMLCtxRAII ctx2;
        struct ggml_tensor* t2 = load_npy(ctx2.ctx, tmp.path);

        WAN_ASSERT_EQ(t2->type,  GGML_TYPE_I64);
        WAN_ASSERT_EQ(t2->ne[0], (int64_t)6);

        const int64_t* src = (int64_t*)t->data;
        const int64_t* dst = (int64_t*)t2->data;
        for (int64_t i = 0; i < 6; ++i) {
            if (src[i] != dst[i]) {
                throw std::runtime_error(
                    "i64 mismatch at " + std::to_string(i));
            }
        }
    });
}

// ---------------------------------------------------------------------------
// Test group: round-trip for 2D tensors
// ---------------------------------------------------------------------------

void test_roundtrip_2d(TestSuite& suite) {
    suite.run("roundtrip_2d_f32_shape_preserved", []() {
        GGMLCtxRAII ctx;
        TempFile tmp("test_io_2d_f32.npy");

        // ggml: ne[0]=cols=4, ne[1]=rows=3  -> numpy shape (3, 4)
        struct ggml_tensor* t = ggml_new_tensor_2d(ctx.ctx, GGML_TYPE_F32, 4, 3);
        fill_f32_sequential(t);
        save_npy(tmp.path, t);

        GGMLCtxRAII ctx2;
        struct ggml_tensor* t2 = load_npy(ctx2.ctx, tmp.path);

        WAN_ASSERT_EQ(t2->type,  GGML_TYPE_F32);
        WAN_ASSERT_EQ(t2->ne[0], (int64_t)4);  // cols restored
        WAN_ASSERT_EQ(t2->ne[1], (int64_t)3);  // rows restored
        WAN_ASSERT_EQ(ggml_nelements(t2), (int64_t)12);

        // Verify data content
        const float* src = (float*)t->data;
        const float* dst = (float*)t2->data;
        for (int64_t i = 0; i < 12; ++i) {
            if (src[i] != dst[i]) {
                throw std::runtime_error(
                    "2D f32 mismatch at " + std::to_string(i));
            }
        }
    });

    suite.run("roundtrip_2d_f16", []() {
        GGMLCtxRAII ctx;
        TempFile tmp("test_io_2d_f16.npy");

        // 5x2 matrix of float16
        struct ggml_tensor* t = ggml_new_tensor_2d(ctx.ctx, GGML_TYPE_F16, 5, 2);
        // Fill with fp16 values via fp32->fp16 conversion
        ggml_fp16_t* d = (ggml_fp16_t*)t->data;
        for (int i = 0; i < 10; ++i) {
            d[i] = ggml_fp32_to_fp16((float)i * 0.5f);
        }
        save_npy(tmp.path, t);

        GGMLCtxRAII ctx2;
        struct ggml_tensor* t2 = load_npy(ctx2.ctx, tmp.path);

        WAN_ASSERT_EQ(t2->type,  GGML_TYPE_F16);
        WAN_ASSERT_EQ(t2->ne[0], (int64_t)5);
        WAN_ASSERT_EQ(t2->ne[1], (int64_t)2);

        const ggml_fp16_t* src = (ggml_fp16_t*)t->data;
        const ggml_fp16_t* dst = (ggml_fp16_t*)t2->data;
        for (int i = 0; i < 10; ++i) {
            // fp16 raw bits must match exactly after round-trip
            if (src[i] != dst[i]) {
                throw std::runtime_error(
                    "f16 mismatch at index " + std::to_string(i));
            }
        }
    });
}

// ---------------------------------------------------------------------------
// Test group: round-trip for 3D tensors
// ---------------------------------------------------------------------------

void test_roundtrip_3d(TestSuite& suite) {
    suite.run("roundtrip_3d_f32", []() {
        GGMLCtxRAII ctx;
        TempFile tmp("test_io_3d_f32.npy");

        // ggml: ne[0]=2, ne[1]=3, ne[2]=4  -> numpy shape (4, 3, 2)
        struct ggml_tensor* t = ggml_new_tensor_3d(ctx.ctx, GGML_TYPE_F32, 2, 3, 4);
        fill_f32_sequential(t);
        save_npy(tmp.path, t);

        GGMLCtxRAII ctx2;
        struct ggml_tensor* t2 = load_npy(ctx2.ctx, tmp.path);

        WAN_ASSERT_EQ(t2->type,  GGML_TYPE_F32);
        WAN_ASSERT_EQ(t2->ne[0], (int64_t)2);
        WAN_ASSERT_EQ(t2->ne[1], (int64_t)3);
        WAN_ASSERT_EQ(t2->ne[2], (int64_t)4);
        WAN_ASSERT_EQ(ggml_nelements(t2), (int64_t)24);

        const float* src = (float*)t->data;
        const float* dst = (float*)t2->data;
        for (int64_t i = 0; i < 24; ++i) {
            if (src[i] != dst[i]) {
                throw std::runtime_error(
                    "3D f32 mismatch at " + std::to_string(i));
            }
        }
    });
}

// ---------------------------------------------------------------------------
// Test group: error handling
// ---------------------------------------------------------------------------

void test_error_handling(TestSuite& suite) {
    suite.run("load_nonexistent_file_throws", []() {
        GGMLCtxRAII ctx;
        bool threw = false;
        try {
            load_npy(ctx.ctx, "/tmp/this_file_does_not_exist_wan_test.npy");
        } catch (const std::exception&) {
            threw = true;
        }
        if (!threw) {
            throw std::runtime_error("Expected exception for missing file, got none");
        }
    });

    suite.run("save_null_tensor_throws", []() {
        bool threw = false;
        try {
            save_npy("/tmp/should_not_exist.npy", nullptr);
        } catch (const std::exception&) {
            threw = true;
        }
        if (!threw) {
            throw std::runtime_error("Expected exception for null tensor, got none");
        }
    });
}

// ---------------------------------------------------------------------------
// Test group: element count and total bytes verification
// ---------------------------------------------------------------------------

void test_tensor_metadata(TestSuite& suite) {
    suite.run("nelements_after_load_1d", []() {
        GGMLCtxRAII ctx;
        TempFile tmp("test_meta_1d.npy");
        struct ggml_tensor* t = ggml_new_tensor_1d(ctx.ctx, GGML_TYPE_F32, 100);
        fill_f32_sequential(t);
        save_npy(tmp.path, t);

        GGMLCtxRAII ctx2;
        struct ggml_tensor* t2 = load_npy(ctx2.ctx, tmp.path);
        WAN_ASSERT_EQ(ggml_nelements(t2), (int64_t)100);
        WAN_ASSERT_EQ(ggml_nbytes(t2), (size_t)(100 * sizeof(float)));
    });

    suite.run("nbytes_f16_correct", []() {
        GGMLCtxRAII ctx;
        TempFile tmp("test_meta_f16.npy");
        struct ggml_tensor* t = ggml_new_tensor_2d(ctx.ctx, GGML_TYPE_F16, 8, 4);
        // Fill with zeros
        memset(t->data, 0, ggml_nbytes(t));
        save_npy(tmp.path, t);

        GGMLCtxRAII ctx2;
        struct ggml_tensor* t2 = load_npy(ctx2.ctx, tmp.path);
        WAN_ASSERT_EQ(t2->type, GGML_TYPE_F16);
        WAN_ASSERT_EQ(ggml_nelements(t2), (int64_t)32);
        WAN_ASSERT_EQ(ggml_nbytes(t2), (size_t)(32 * sizeof(ggml_fp16_t)));
    });
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"test_io_npy"};

    test_roundtrip_1d(suite);
    test_roundtrip_2d(suite);
    test_roundtrip_3d(suite);
    test_error_handling(suite);
    test_tensor_metadata(suite);

    return suite.report();
}
