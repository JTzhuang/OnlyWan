#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_registry.hpp"
#include "clip.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

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
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"CLIP Model Tests"};
    test_clip_init_all_versions(suite);
    test_clip_registry_roundtrip(suite);
    return suite.report();
}
