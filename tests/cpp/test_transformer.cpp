#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "test_io_utils.hpp"
#include "model_registry.hpp"

// WAN Transformer (DiT) header
// NOTE: flux.hpp has been REMOVED from src/. This file no longer tests FluxRunner.
// WAN::WanRunner is the WAN DiT Transformer (replaces Flux for video generation).
#include "wan.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

#include <filesystem>
namespace fs = std::filesystem;

// Force-link model_factory.cpp to ensure static registrations are not DCE'd by linker.
extern "C" void wan_force_model_registrations();

// ---------------------------------------------------------------------------
// WAN Transformer (WanRunner) registry test
// ---------------------------------------------------------------------------
// WAN::WanRunner supports T2V, I2V, and TI2V variants via SDVersion.

void test_wan_transformer_init(TestSuite& suite) {
    wan_force_model_registrations();

    suite.run("wan_runner_3_versions_in_registry", []() {
        // Verify all 3 WAN::WanRunner variants are registered
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanRunner>("wan-runner-t2v"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanRunner>("wan-runner-i2v"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanRunner>("wan-runner-ti2v"));
    });

    suite.run("create_wan_runner_t2v_via_registry", []() {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage tensor_map{};
        // Add a dummy block tensor to ensure num_layers is detected as 40
        tensor_map["blocks.39.weight"] = TensorStorage();

        // Create via registry using string version
        auto runner = ModelRegistry::instance()->create<WAN::WanRunner>(
            "wan-runner-t2v", guard.backend, false, tensor_map, "");
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();
        // WAN::WanRunner::get_desc() returns "Wan2.x-T2V-14B" for 40 layers and T2V
        WAN_ASSERT_EQ(runner->get_desc(), std::string("Wan2.x-T2V-14B"));
    });

    suite.run("create_wan_runner_i2v_via_registry", []() {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage tensor_map{};
        tensor_map["blocks.39.weight"] = TensorStorage();
        // Add img_emb tensor to trigger i2v detection
        tensor_map["img_emb"] = TensorStorage();

        // Create via registry using string version
        auto runner = ModelRegistry::instance()->create<WAN::WanRunner>(
            "wan-runner-i2v", guard.backend, false, tensor_map, "");
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();
        // For 40 layers and model_type == "i2v", get_desc() returns "Wan2.1-I2V-14B"
        WAN_ASSERT_EQ(runner->get_desc(), std::string("Wan2.1-I2V-14B"));
    });

    suite.run("create_wan_runner_ti2v_via_registry", []() {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage tensor_map{};
        // For Wan2.2-TI2V-5B, num_layers must be 30
        tensor_map["blocks.29.weight"] = TensorStorage();

        // Create via registry using string version
        auto runner = ModelRegistry::instance()->create<WAN::WanRunner>(
            "wan-runner-ti2v", guard.backend, false, tensor_map, "");
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();
        WAN_ASSERT_EQ(runner->get_desc(), std::string("Wan2.2-TI2V-5B"));
    });
}

// ---------------------------------------------------------------------------
// NpyIO: WAN Transformer latent numerical comparison
// ---------------------------------------------------------------------------
// WAN DiT operates on video latent tensors: [frames, height, width, channels].
// Use save_npy to dump C++ tensors then compare with Python WAN reference outputs.

void test_wan_transformer_npy_io(TestSuite& suite) {
    suite.run("wan_latent_f32_roundtrip", []() {
        // Small proxy for WAN latent: [frames=2, h=4, w=4, ch=16]
        constexpr int frames = 2, h = 4, w = 4, ch = 16;
        const size_t buf_size = ggml_tensor_overhead() * 8 + frames * h * w * ch * sizeof(float) + 64;
        std::vector<uint8_t> buf(buf_size);
        ggml_init_params params{buf_size, buf.data(), false};
        ggml_context* ctx = ggml_init(params);
        WAN_ASSERT_TRUE(ctx != nullptr);

        // ggml dim order: ne[0]=ch, ne[1]=w, ne[2]=h, ne[3]=frames
        ggml_tensor* t = ggml_new_tensor_4d(ctx, GGML_TYPE_F32, ch, w, h, frames);
        float* data = (float*)t->data;
        for (int i = 0; i < frames * h * w * ch; ++i) data[i] = (float)i * 0.001f;

        fs::path tmp = fs::temp_directory_path() / "test_wan_latent.npy";
        save_npy(tmp.string(), t);

        ggml_tensor* loaded = load_npy(ctx, tmp.string());
        WAN_ASSERT_TRUE(loaded != nullptr);
        WAN_ASSERT_EQ(loaded->ne[0], t->ne[0]);
        WAN_ASSERT_EQ(loaded->ne[1], t->ne[1]);
        WAN_ASSERT_EQ(loaded->ne[2], t->ne[2]);
        WAN_ASSERT_EQ(loaded->ne[3], t->ne[3]);

        float* ldata = (float*)loaded->data;
        for (int i = 0; i < frames * h * w * ch; ++i)
            WAN_ASSERT_TRUE(std::abs(ldata[i] - data[i]) < 1e-6f);

        ggml_free(ctx);
        fs::remove(tmp);
    });
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"WAN Transformer Tests"};
    test_wan_transformer_init(suite);
    test_wan_transformer_npy_io(suite);
    return suite.report();
}
