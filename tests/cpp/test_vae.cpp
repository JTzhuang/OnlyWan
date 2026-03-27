#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_factory.hpp"

// Including model headers directly is SAFE -- all are header-only with
// implicitly inline member functions. See review_notes in plan context.
#include "vae.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

// ---------------------------------------------------------------------------
// Factory helper
// ---------------------------------------------------------------------------

// Returns a ModelFactory<VAE, SDVersion> (polymorphic: base class factory).
// All 4 versions are created via AutoEncoderKL (concrete subclass).
// Using VAE base class tests D-04 (polymorphic factory usage).
static ModelFactory<VAE, SDVersion> register_vae_factory() {
    ModelFactory<VAE, SDVersion> factory;

    factory.register_version(VERSION_SD1,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<AutoEncoderKL>(
                backend, offload, map, prefix, /*decode_only=*/true, /*use_video_decoder=*/false, VERSION_SD1);
        });
    factory.register_version(VERSION_SD2,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<AutoEncoderKL>(
                backend, offload, map, prefix, /*decode_only=*/true, /*use_video_decoder=*/false, VERSION_SD2);
        });
    factory.register_version(VERSION_FLUX,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<AutoEncoderKL>(
                backend, offload, map, prefix, /*decode_only=*/true, /*use_video_decoder=*/false, VERSION_FLUX);
        });
    factory.register_version(VERSION_FLUX2,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<AutoEncoderKL>(
                backend, offload, map, prefix, /*decode_only=*/true, /*use_video_decoder=*/false, VERSION_FLUX2);
        });

    return factory;
}

// ---------------------------------------------------------------------------
// Test: initialization for all 4 VAE versions
// ---------------------------------------------------------------------------

// NOTE: AutoEncoderKL::get_desc() returns "vae" (not "AutoEncoderKL").
// This is verified from src/vae.hpp line 692. The plan docs were incorrect;
// source code is authoritative. This matches the correction applied in 17-01.

void test_vae_init_all_versions(TestSuite& suite) {
    struct VersionEntry {
        SDVersion version;
        const char* name;
    };
    const VersionEntry versions[] = {
        {VERSION_SD1,   "init_VERSION_SD1"},
        {VERSION_SD2,   "init_VERSION_SD2"},
        {VERSION_FLUX,  "init_VERSION_FLUX"},
        {VERSION_FLUX2, "init_VERSION_FLUX2"},
    };

    auto factory = register_vae_factory();
    String2TensorStorage empty_map{};

    // Note on empty tensor_storage_map: use_linear_projection defaults false.
    // This is acceptable for unit tests -- we test construction and initialization,
    // not inference correctness.
    for (const auto& entry : versions) {
        SDVersion version = entry.version;
        suite.run(entry.name, [&factory, version, &empty_map]() {
            // BackendRAII MUST be declared BEFORE runner (reverse destruction order).
            BackendRAII guard(ggml_backend_cpu_init());
            auto runner = factory.create(version, guard.backend, false, empty_map, "");
            runner->alloc_params_buffer();
            WAN_ASSERT_TRUE(runner != nullptr);
            // AutoEncoderKL::get_desc() returns "vae" (source-verified from vae.hpp:692)
            WAN_ASSERT_EQ(runner->get_desc(), std::string("vae"));
        });
    }
}

// ---------------------------------------------------------------------------
// Test: factory registration mechanics
// ---------------------------------------------------------------------------

void test_vae_factory_registration(TestSuite& suite) {
    suite.run("all_4_versions_registered", []() {
        auto factory = register_vae_factory();
        WAN_ASSERT_EQ(factory.registered_versions().size(), size_t(4));
    });

    suite.run("has_version_checks", []() {
        auto factory = register_vae_factory();
        WAN_ASSERT_TRUE(factory.has_version(VERSION_SD1));
        WAN_ASSERT_TRUE(factory.has_version(VERSION_SD2));
        WAN_ASSERT_TRUE(factory.has_version(VERSION_FLUX));
        WAN_ASSERT_TRUE(factory.has_version(VERSION_FLUX2));
        // VERSION_SD3 is NOT registered -- should return false
        WAN_ASSERT_TRUE(!factory.has_version(VERSION_SD3));
    });
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"VAE Model Tests"};
    test_vae_init_all_versions(suite);
    test_vae_factory_registration(suite);
    return suite.report();
}
