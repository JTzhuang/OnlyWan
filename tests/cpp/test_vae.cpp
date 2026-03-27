#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "test_io_utils.hpp"
#include "model_registry.hpp"

// WAN VAE uses WAN::WanVAERunner (from wan.hpp), NOT AutoEncoderKL (from vae.hpp).
// WAN::WanVAERunner::get_desc() returns "wan_vae".
#include "wan.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

#include <filesystem>
namespace fs = std::filesystem;

// Force-link model_factory.cpp to ensure static registrations are not DCE'd by linker.
extern "C" void wan_force_model_registrations();

// ---------------------------------------------------------------------------
// Test: initialization for all 4 WAN VAE versions
// ---------------------------------------------------------------------------

void test_vae_init_all_versions(TestSuite& suite) {
    wan_force_model_registrations();
    String2TensorStorage empty_map{};

    const struct { const char* version_str; const char* test_name; } versions[] = {
        {"wan-vae-t2v",        "init_wan_vae_t2v"},
        {"wan-vae-t2v-decode", "init_wan_vae_t2v_decode"},
        {"wan-vae-i2v",        "init_wan_vae_i2v"},
        {"wan-vae-ti2v",       "init_wan_vae_ti2v"},
    };
    for (const auto& entry : versions) {
        const char* version_str = entry.version_str;
        suite.run(entry.test_name, [version_str, &empty_map]() {
            BackendRAII guard(ggml_backend_cpu_init());
            auto runner = ModelRegistry::instance()->create<WAN::WanVAERunner>(version_str, guard.backend, false, empty_map, "");
            WAN_ASSERT_TRUE(runner != nullptr);
            runner->alloc_params_buffer();
            // WAN::WanVAERunner::get_desc() returns "wan_vae"
            WAN_ASSERT_EQ(runner->get_desc(), std::string("wan_vae"));
        });
    }
}

// ---------------------------------------------------------------------------
// Test: registry registration mechanics
// ---------------------------------------------------------------------------

void test_vae_registry_registration(TestSuite& suite) {
    suite.run("all_4_wan_vae_versions_in_registry", []() {
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanVAERunner>("wan-vae-t2v"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanVAERunner>("wan-vae-t2v-decode"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanVAERunner>("wan-vae-i2v"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanVAERunner>("wan-vae-ti2v"));
    });
}

// ---------------------------------------------------------------------------
// Test: .npy I/O roundtrip for VAE-shaped tensors (numerical consistency)
// ---------------------------------------------------------------------------
// WAN VAE encodes/decodes latent tensors: typical shape [C, T, H, W] = [16, 1, 8, 8].
// Use save_npy to dump C++ latents then compare with Python WAN VAE reference.

void test_vae_npy_io(TestSuite& suite) {
    suite.run("vae_latent_f32_roundtrip", []() {
        // Small proxy for a VAE latent: [C=4, H=4, W=4]
        constexpr int C = 4, H = 4, W = 4;
        const size_t buf_size = ggml_tensor_overhead() * 8 + C * H * W * sizeof(float) + 64;
        std::vector<uint8_t> buf(buf_size);
        ggml_init_params params{buf_size, buf.data(), false};
        ggml_context* ctx = ggml_init(params);
        WAN_ASSERT_TRUE(ctx != nullptr);

        ggml_tensor* t = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, W, H, C);
        float* data = (float*)t->data;
        for (int i = 0; i < C * H * W; ++i) data[i] = (float)i * 0.05f - 1.0f;

        fs::path tmp = fs::temp_directory_path() / "test_vae_latent.npy";
        save_npy(tmp.string(), t);

        ggml_tensor* loaded = load_npy(ctx, tmp.string());
        WAN_ASSERT_TRUE(loaded != nullptr);
        WAN_ASSERT_EQ(loaded->type,  t->type);
        WAN_ASSERT_EQ(loaded->ne[0], t->ne[0]);
        WAN_ASSERT_EQ(loaded->ne[1], t->ne[1]);
        WAN_ASSERT_EQ(loaded->ne[2], t->ne[2]);
        float* ld = (float*)loaded->data;
        for (int i = 0; i < C * H * W; ++i)
            WAN_ASSERT_NEAR(ld[i], data[i], 1e-6f);

        fs::remove(tmp);
        ggml_free(ctx);
    });
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"WAN VAE Tests"};
    test_vae_init_all_versions(suite);
    test_vae_registry_registration(suite);
    return suite.report();
}
