#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "test_io_utils.hpp"
#include "model_registry.hpp"
#include "clip.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

#include <filesystem>
namespace fs = std::filesystem;

// Force-link model_factory.cpp to ensure static registrations are not DCE'd by linker.
extern "C" void wan_force_model_registrations();

// ---------------------------------------------------------------------------
// Test: initialization for all 3 CLIP versions
// ---------------------------------------------------------------------------

void test_clip_init_all_versions(TestSuite& suite) {
    wan_force_model_registrations();
    String2TensorStorage empty_map{};

    const struct { const char* version_str; const char* test_name; } versions[] = {
        {"clip-vit-l-14",    "init_clip_vit_l_14_via_registry"},
        {"clip-vit-h-14",    "init_clip_vit_h_14_via_registry"},
        {"clip-vit-bigg-14", "init_clip_vit_bigg_14_via_registry"},
    };
    for (const auto& entry : versions) {
        const char* version_str = entry.version_str;
        suite.run(entry.test_name, [version_str, &empty_map]() {
            BackendRAII guard(ggml_backend_cpu_init());
            auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>(version_str, guard.backend, false, empty_map, "");
            WAN_ASSERT_TRUE(runner != nullptr);
            runner->alloc_params_buffer();
            WAN_ASSERT_EQ(runner->get_desc(), std::string("clip"));
        });
    }
}

// ---------------------------------------------------------------------------
// Test: registry mechanics round-trip
// ---------------------------------------------------------------------------

void test_clip_registry_roundtrip(TestSuite& suite) {
    suite.run("all_3_versions_registered", []() {
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<CLIPTextModelRunner>("clip-vit-l-14"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<CLIPTextModelRunner>("clip-vit-h-14"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<CLIPTextModelRunner>("clip-vit-bigg-14"));
    });

    suite.run("create_each_version", []() {
        String2TensorStorage empty_map{};
        const std::string versions[] = {
            "clip-vit-l-14",
            "clip-vit-h-14",
            "clip-vit-bigg-14",
        };
        for (const std::string& version : versions) {
            BackendRAII guard(ggml_backend_cpu_init());
            auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>(version, guard.backend, false, empty_map, "");
            WAN_ASSERT_TRUE(runner != nullptr);
            WAN_ASSERT_EQ(runner->get_desc(), std::string("clip"));
        }
    });
}

// ---------------------------------------------------------------------------
// Test: .npy I/O roundtrip for CLIP-shaped tensors (numerical consistency)
// ---------------------------------------------------------------------------
// These tests verify save_npy/load_npy work correctly for the tensor shapes
// typical in CLIP (e.g. token embeddings [seq_len, hidden_dim]).
// Use save_npy(path, tensor) to dump C++ outputs, then compare with Python.

void test_clip_npy_io(TestSuite& suite) {
    suite.run("clip_embedding_f32_roundtrip", []() {
        // Simulate a CLIP text embedding: shape [77, 768] (seq_len=77, hidden=768)
        // Use small dims to keep the test fast.
        constexpr int seq  = 4;
        constexpr int hdim = 8;
        const size_t buf_size = ggml_tensor_overhead() * 8 + seq * hdim * sizeof(float) + 64;
        std::vector<uint8_t> buf(buf_size);
        ggml_init_params params{buf_size, buf.data(), false};
        ggml_context* ctx = ggml_init(params);
        WAN_ASSERT_TRUE(ctx != nullptr);

        ggml_tensor* t = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hdim, seq);
        float* data = (float*)t->data;
        for (int i = 0; i < seq * hdim; ++i) data[i] = (float)i * 0.1f;

        fs::path tmp = fs::temp_directory_path() / "test_clip_emb.npy";
        save_npy(tmp.string(), t);

        ggml_tensor* loaded = load_npy(ctx, tmp.string());
        WAN_ASSERT_TRUE(loaded != nullptr);
        WAN_ASSERT_EQ(loaded->type,  t->type);
        WAN_ASSERT_EQ(loaded->ne[0], t->ne[0]);
        WAN_ASSERT_EQ(loaded->ne[1], t->ne[1]);
        float* ld = (float*)loaded->data;
        for (int i = 0; i < seq * hdim; ++i)
            WAN_ASSERT_NEAR(ld[i], data[i], 1e-6f);

        fs::remove(tmp);
        ggml_free(ctx);
    });
}

// ---------------------------------------------------------------------------
// Test: CLIP inference with random data
// ---------------------------------------------------------------------------

void test_clip_inference_with_random_data(TestSuite& suite) {
    suite.run("clip_forward_with_random_input", []() {
        wan_force_model_registrations();
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};

        auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>(
            "clip-vit-l-14", guard.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();

        // Create random input: [1, 77] token IDs
        ggml_init_params params{100*1024*1024, nullptr, false};
        ggml_context* ctx = ggml_init(params);
        ggml_tensor* input_ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 77, 1);
        int32_t* ids = (int32_t*)input_ids->data;
        for (int i = 0; i < 77; ++i) ids[i] = (i % 49152);  // random token IDs

        // Create attention mask: [1, 77]
        ggml_tensor* mask = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 77, 1);
        float* mask_data = (float*)mask->data;
        for (int i = 0; i < 77; ++i) mask_data[i] = 1.0f;

        // Create GGMLRunnerContext
        GGMLRunnerContext runner_ctx;
        runner_ctx.ggml_ctx = ctx;
        runner_ctx.backend = guard.backend;

        // Run inference
        struct ggml_tensor* output = runner->forward(&runner_ctx, input_ids, nullptr, mask, 0, false, -1);
        WAN_ASSERT_TRUE(output != nullptr);
        WAN_ASSERT_EQ(output->ne[0], 768);  // hidden_size for vit-l-14
        WAN_ASSERT_EQ(output->ne[1], 77);   // seq_len

        // Save output to .npy for comparison
        fs::path tmp = fs::temp_directory_path() / "test_clip_output.npy";
        save_npy(tmp.string(), output);
        WAN_ASSERT_TRUE(fs::exists(tmp));

        ggml_free(ctx);
        fs::remove(tmp);
    });
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"CLIP Model Tests"};
    test_clip_init_all_versions(suite);
    test_clip_registry_roundtrip(suite);
    test_clip_npy_io(suite);
    test_clip_inference_with_random_data(suite);
    return suite.report();
}
