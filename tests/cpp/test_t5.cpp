#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "test_io_utils.hpp"
#include "model_registry.hpp"
#include "t5.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

#include <filesystem>
namespace fs = std::filesystem;

// Force-link model_factory.cpp to ensure static registrations are not DCE'd by linker.
extern "C" void wan_force_model_registrations();

// ---------------------------------------------------------------------------
// Test: initialization for both T5 versions
// ---------------------------------------------------------------------------

void test_t5_init_all_versions(TestSuite& suite) {
    wan_force_model_registrations();
    String2TensorStorage empty_map{};

    suite.run("init_standard_t5_via_registry", [&empty_map]() {
        BackendRAII guard(ggml_backend_cpu_init());
        auto runner = ModelRegistry::instance()->create<T5Runner>("t5-standard", guard.backend, false, empty_map, "");
        runner->alloc_params_buffer();
        WAN_ASSERT_TRUE(runner != nullptr);
        WAN_ASSERT_EQ(runner->get_desc(), std::string("t5"));
    });

    suite.run("init_umt5_via_registry", [&empty_map]() {
        BackendRAII guard(ggml_backend_cpu_init());
        auto runner = ModelRegistry::instance()->create<T5Runner>("t5-umt5", guard.backend, false, empty_map, "");
        runner->alloc_params_buffer();
        WAN_ASSERT_TRUE(runner != nullptr);
        WAN_ASSERT_EQ(runner->get_desc(), std::string("t5"));
    });
}

// ---------------------------------------------------------------------------
// Test: registry registration mechanics
// ---------------------------------------------------------------------------

void test_t5_registry_registration(TestSuite& suite) {
    suite.run("both_versions_registered", []() {
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<T5Runner>("t5-standard"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<T5Runner>("t5-umt5"));
    });
}

// ---------------------------------------------------------------------------
// Test: .npy I/O roundtrip for T5-shaped tensors (numerical consistency)
// ---------------------------------------------------------------------------
// T5 text encoder outputs a sequence of hidden states: shape [seq_len, hidden_dim].
// Use save_npy to dump C++ tensors then compare with Python T5 reference outputs.

void test_t5_npy_io(TestSuite& suite) {
    suite.run("t5_hidden_state_f32_roundtrip", []() {
        // Small proxy for T5 hidden state: [seq=4, hidden=16]
        constexpr int seq  = 4;
        constexpr int hdim = 16;
        const size_t buf_size = ggml_tensor_overhead() * 8 + seq * hdim * sizeof(float) + 64;
        std::vector<uint8_t> buf(buf_size);
        ggml_init_params params{buf_size, buf.data(), false};
        ggml_context* ctx = ggml_init(params);
        WAN_ASSERT_TRUE(ctx != nullptr);

        ggml_tensor* t = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hdim, seq);
        float* data = (float*)t->data;
        for (int i = 0; i < seq * hdim; ++i) data[i] = (float)i * 0.01f - 0.5f;

        fs::path tmp = fs::temp_directory_path() / "test_t5_hidden.npy";
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
// Test: T5 inference with random data
// ---------------------------------------------------------------------------

void test_t5_inference_with_random_data(TestSuite& suite) {
    suite.run("t5_forward_with_random_input", []() {
        wan_force_model_registrations();
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};

        auto runner = ModelRegistry::instance()->create<T5Runner>(
            "t5-standard", guard.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();

        // Create random input: [1, 16] token IDs
        ggml_init_params params{20*1024*1024, nullptr, false};
        ggml_context* ctx = ggml_init(params);
        ggml_tensor* input_ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 16, 1);
        int32_t* ids = (int32_t*)input_ids->data;
        for (int i = 0; i < 16; ++i) ids[i] = (i % 32128);  // random token IDs

        // Create relative position bucket: [16, 16]
        ggml_tensor* pos_bucket = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 16, 16);
        int32_t* pos_data = (int32_t*)pos_bucket->data;
        for (int i = 0; i < 256; ++i) pos_data[i] = (i % 32);

        // Create GGMLRunnerContext
        GGMLRunnerContext runner_ctx;
        runner_ctx.ggml_ctx = ctx;
        runner_ctx.backend = guard.backend;

        // Run inference
        struct ggml_tensor* output = runner->forward(&runner_ctx, input_ids, pos_bucket, nullptr);
        WAN_ASSERT_TRUE(output != nullptr);
        WAN_ASSERT_EQ(output->ne[0], 4096);  // model_dim for T5
        WAN_ASSERT_EQ(output->ne[1], 16);    // seq_len

        // Save output to .npy for comparison
        fs::path tmp = fs::temp_directory_path() / "test_t5_output.npy";
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
    TestSuite suite{"T5 Model Tests"};
    test_t5_init_all_versions(suite);
    test_t5_registry_registration(suite);
    test_t5_npy_io(suite);
    test_t5_inference_with_random_data(suite);
    return suite.report();
}
