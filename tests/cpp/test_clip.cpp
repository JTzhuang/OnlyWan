#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_factory.hpp"

// Including model headers directly is SAFE -- all are header-only with
// implicitly inline member functions. See review_notes in plan context.
#include "clip.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

// ---------------------------------------------------------------------------
// Factory helper
// ---------------------------------------------------------------------------

// Returns a ModelFactory with all 3 CLIPVersion values registered.
static ModelFactory<CLIPTextModelRunner, CLIPVersion> register_clip_factory() {
    ModelFactory<CLIPTextModelRunner, CLIPVersion> factory;

    factory.register_version(OPENAI_CLIP_VIT_L_14,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<CLIPTextModelRunner>(
                backend, offload, map, prefix, OPENAI_CLIP_VIT_L_14, /*with_final_ln=*/true);
        });
    factory.register_version(OPEN_CLIP_VIT_H_14,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<CLIPTextModelRunner>(
                backend, offload, map, prefix, OPEN_CLIP_VIT_H_14, /*with_final_ln=*/true);
        });
    factory.register_version(OPEN_CLIP_VIT_BIGG_14,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<CLIPTextModelRunner>(
                backend, offload, map, prefix, OPEN_CLIP_VIT_BIGG_14, /*with_final_ln=*/true);
        });

    return factory;
}

// ---------------------------------------------------------------------------
// Test: initialization for all 3 CLIP versions
// ---------------------------------------------------------------------------

void test_clip_init_all_versions(TestSuite& suite) {
    struct VersionEntry {
        CLIPVersion version;
        const char* name;
    };
    const VersionEntry versions[] = {
        {OPENAI_CLIP_VIT_L_14,   "init_OPENAI_CLIP_VIT_L_14"},
        {OPEN_CLIP_VIT_H_14,     "init_OPEN_CLIP_VIT_H_14"},
        {OPEN_CLIP_VIT_BIGG_14,  "init_OPEN_CLIP_VIT_BIGG_14"},
    };

    auto factory = register_clip_factory();
    String2TensorStorage empty_map{};

    for (const auto& entry : versions) {
        CLIPVersion version = entry.version;
        suite.run(entry.name, [&factory, version, &empty_map]() {
            // BackendRAII MUST be declared BEFORE runner (reverse destruction order).
            BackendRAII guard(ggml_backend_cpu_init());
            auto runner = factory.create(version, guard.backend, false, empty_map, "");
            runner->alloc_params_buffer();
            WAN_ASSERT_TRUE(runner != nullptr);
            WAN_ASSERT_EQ(runner->get_desc(), std::string("clip"));
        });
    }
}

// ---------------------------------------------------------------------------
// Test: factory mechanics round-trip
// ---------------------------------------------------------------------------

void test_clip_factory_roundtrip(TestSuite& suite) {
    suite.run("all_3_versions_registered", []() {
        auto factory = register_clip_factory();
        WAN_ASSERT_EQ(factory.registered_versions().size(), size_t(3));
    });

    suite.run("create_each_version", []() {
        auto factory = register_clip_factory();
        String2TensorStorage empty_map{};

        const CLIPVersion versions[] = {
            OPENAI_CLIP_VIT_L_14,
            OPEN_CLIP_VIT_H_14,
            OPEN_CLIP_VIT_BIGG_14,
        };
        for (CLIPVersion version : versions) {
            BackendRAII guard(ggml_backend_cpu_init());
            auto runner = factory.create(version, guard.backend, false, empty_map, "");
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
    test_clip_factory_roundtrip(suite);
    return suite.report();
}
