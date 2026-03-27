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
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"T5 Model Tests"};
    test_t5_init_all_versions(suite);
    test_t5_registry_registration(suite);
    return suite.report();
}
