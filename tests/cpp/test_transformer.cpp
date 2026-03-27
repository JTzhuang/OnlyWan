#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_factory.hpp"

// Including model headers directly is SAFE -- all are header-only with
// implicitly inline member functions. See review_notes in plan context.
// flux.hpp has a guard (added in 17-01) to prevent SIGFPE on empty tensor_storage_map:
//   if (head_dim > 0) { flux_params.num_heads = flux_params.hidden_size / head_dim; }
#include "flux.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

// ---------------------------------------------------------------------------
// Factory helper
// ---------------------------------------------------------------------------

// Returns a ModelFactory with all 5 Flux SDVersion values registered.
// Using Flux::FluxRunner (fully qualified per Pitfall 2 from RESEARCH.md).
//
// VERSION_FLEX_2 is the CORRECT enum name (verified in src/model.h line 42).
// It is NOT a typo. VERSION_FLUX2 (line 49) is a separate enum value used for VAE.
static ModelFactory<Flux::FluxRunner, SDVersion> register_flux_factory() {
    ModelFactory<Flux::FluxRunner, SDVersion> factory;

    factory.register_version(VERSION_FLUX,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<Flux::FluxRunner>(
                backend, offload, map, prefix, VERSION_FLUX, /*use_mask=*/false);
        });
    factory.register_version(VERSION_FLUX_FILL,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<Flux::FluxRunner>(
                backend, offload, map, prefix, VERSION_FLUX_FILL, /*use_mask=*/false);
        });
    // NOTE: VERSION_FLEX_2 is the correct enum (verified in src/model.h line 42).
    // Not a typo. VERSION_FLUX2 is separate (used for VAE).
    factory.register_version(VERSION_FLEX_2,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<Flux::FluxRunner>(
                backend, offload, map, prefix, VERSION_FLEX_2, /*use_mask=*/false);
        });
    factory.register_version(VERSION_CHROMA_RADIANCE,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<Flux::FluxRunner>(
                backend, offload, map, prefix, VERSION_CHROMA_RADIANCE, /*use_mask=*/false);
        });
    factory.register_version(VERSION_OVIS_IMAGE,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<Flux::FluxRunner>(
                backend, offload, map, prefix, VERSION_OVIS_IMAGE, /*use_mask=*/false);
        });

    return factory;
}

// ---------------------------------------------------------------------------
// Test: initialization for all 5 Flux versions
// ---------------------------------------------------------------------------

void test_flux_init_all_versions(TestSuite& suite) {
    struct VersionEntry {
        SDVersion version;
        const char* name;
    };
    const VersionEntry versions[] = {
        {VERSION_FLUX,            "init_VERSION_FLUX"},
        {VERSION_FLUX_FILL,       "init_VERSION_FLUX_FILL"},
        {VERSION_FLEX_2,          "init_VERSION_FLEX_2"},
        {VERSION_CHROMA_RADIANCE, "init_VERSION_CHROMA_RADIANCE"},
        {VERSION_OVIS_IMAGE,      "init_VERSION_OVIS_IMAGE"},
    };

    auto factory = register_flux_factory();
    String2TensorStorage empty_map{};

    for (const auto& entry : versions) {
        SDVersion version = entry.version;
        suite.run(entry.name, [&factory, version, &empty_map]() {
            // BackendRAII MUST be declared BEFORE runner (reverse destruction order).
            BackendRAII guard(ggml_backend_cpu_init());
            auto runner = factory.create(version, guard.backend, false, empty_map, "");
            runner->alloc_params_buffer();
            WAN_ASSERT_TRUE(runner != nullptr);
            WAN_ASSERT_EQ(runner->get_desc(), std::string("flux"));
        });
    }
}

// ---------------------------------------------------------------------------
// Test: factory registration mechanics
// ---------------------------------------------------------------------------

void test_flux_factory_registration(TestSuite& suite) {
    suite.run("all_5_versions_registered", []() {
        auto factory = register_flux_factory();
        WAN_ASSERT_EQ(factory.registered_versions().size(), size_t(5));
    });

    suite.run("has_version_checks", []() {
        auto factory = register_flux_factory();
        WAN_ASSERT_TRUE(factory.has_version(VERSION_FLUX));
        WAN_ASSERT_TRUE(factory.has_version(VERSION_FLUX_FILL));
        WAN_ASSERT_TRUE(factory.has_version(VERSION_FLEX_2));
        WAN_ASSERT_TRUE(factory.has_version(VERSION_CHROMA_RADIANCE));
        WAN_ASSERT_TRUE(factory.has_version(VERSION_OVIS_IMAGE));
    });
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"Transformer/Flux Model Tests"};
    test_flux_init_all_versions(suite);
    test_flux_factory_registration(suite);
    return suite.report();
}
