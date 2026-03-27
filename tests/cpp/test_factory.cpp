#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_registry.hpp"

// WAN model headers (for registry aliases to resolve)
#include "clip.hpp"
#include "t5.hpp"
#include "wan.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

// NOTE: model_factory.hpp is NOT included here — we test the global registry directly.
// NOTE: flux.hpp is NOT included — it has been removed from src/.

// Force-link model_factory.cpp to ensure static registrations are not DCE'd by linker.
extern "C" void wan_force_model_registrations();

// ---------------------------------------------------------------------------
// CLIP registry test
// ---------------------------------------------------------------------------

void test_clip_registry(TestSuite& suite) {
    wan_force_model_registrations();  // ensure all 9 registrations are active

    suite.run("clip_3_versions_in_registry", []() {
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<CLIPTextModelRunner>("clip-vit-l-14"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<CLIPTextModelRunner>("clip-vit-h-14"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<CLIPTextModelRunner>("clip-vit-bigg-14"));
    });

    suite.run("clip_create_via_registry", []() {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};
        auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>("clip-vit-l-14", guard.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();
        WAN_ASSERT_EQ(runner->get_desc(), std::string("clip"));
    });

    suite.run("clip_unknown_version_throws", []() {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};
        WAN_ASSERT_THROWS(ModelRegistry::instance()->create<CLIPTextModelRunner>("clip-nonexistent", guard.backend, false, empty_map, ""));
    });
}

// ---------------------------------------------------------------------------
// T5 registry test
// ---------------------------------------------------------------------------

void test_t5_registry(TestSuite& suite) {
    suite.run("t5_2_versions_in_registry", []() {
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<T5Runner>("t5-standard"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<T5Runner>("t5-umt5"));
    });

    suite.run("t5_create_both_via_registry", []() {
        String2TensorStorage empty_map{};

        BackendRAII guard_std(ggml_backend_cpu_init());
        auto t5 = ModelRegistry::instance()->create<T5Runner>("t5-standard", guard_std.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(t5 != nullptr);
        t5->alloc_params_buffer();
        WAN_ASSERT_EQ(t5->get_desc(), std::string("t5"));
        t5.reset();

        BackendRAII guard_umt5(ggml_backend_cpu_init());
        auto umt5 = ModelRegistry::instance()->create<T5Runner>("t5-umt5", guard_umt5.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(umt5 != nullptr);
        umt5->alloc_params_buffer();
        WAN_ASSERT_EQ(umt5->get_desc(), std::string("t5"));
    });
}

// ---------------------------------------------------------------------------
// WAN VAE registry test
// ---------------------------------------------------------------------------

void test_wan_vae_registry(TestSuite& suite) {
    suite.run("wan_vae_4_versions_in_registry", []() {
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanVAERunner>("wan-vae-t2v"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanVAERunner>("wan-vae-t2v-decode"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanVAERunner>("wan-vae-i2v"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<WAN::WanVAERunner>("wan-vae-ti2v"));
    });

    suite.run("wan_vae_create_via_registry", []() {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};
        auto vae = ModelRegistry::instance()->create<WAN::WanVAERunner>("wan-vae-t2v", guard.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(vae != nullptr);
        vae->alloc_params_buffer();
        // WAN::WanVAERunner::get_desc() returns "wan_vae"
        WAN_ASSERT_EQ(vae->get_desc(), std::string("wan_vae"));
    });
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"Model Registry Tests"};
    test_clip_registry(suite);
    test_t5_registry(suite);
    test_wan_vae_registry(suite);
    return suite.report();
}
