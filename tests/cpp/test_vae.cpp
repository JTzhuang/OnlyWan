#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_registry.hpp"

// WAN VAE uses WAN::WanVAERunner (from wan.hpp), NOT AutoEncoderKL (from vae.hpp).
// WAN::WanVAERunner::get_desc() returns "wan_vae".
#include "wan.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

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
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"WAN VAE Tests"};
    test_vae_init_all_versions(suite);
    test_vae_registry_registration(suite);
    return suite.report();
}
