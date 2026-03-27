#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_factory.hpp"

// Including model headers directly is SAFE -- all are header-only with
// implicitly inline member functions. See review_notes in plan context.
#include "clip.hpp"
#include "t5.hpp"
#include "vae.hpp"
#include "flux.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

// ---------------------------------------------------------------------------
// CLIP factory tests
// ---------------------------------------------------------------------------

void test_clip_factory(TestSuite& suite) {
    suite.run("register_and_create_3_clip_versions", []() {
        using CLIPFactory = ModelFactory<CLIPTextModelRunner, CLIPVersion>;
        CLIPFactory factory;

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

        WAN_ASSERT_EQ(factory.registered_versions().size(), size_t(3));

        // Create one instance and verify
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};
        auto runner = factory.create(OPENAI_CLIP_VIT_L_14, guard.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();
        WAN_ASSERT_EQ(runner->get_desc(), std::string("clip"));
    });

    suite.run("clip_factory_unknown_version_throws", []() {
        using CLIPFactory = ModelFactory<CLIPTextModelRunner, CLIPVersion>;
        CLIPFactory factory;
        // Register only L14 -- BIGG_14 is NOT registered
        factory.register_version(OPENAI_CLIP_VIT_L_14,
            [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
                return std::make_unique<CLIPTextModelRunner>(
                    backend, offload, map, prefix, OPENAI_CLIP_VIT_L_14, /*with_final_ln=*/true);
            });

        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};
        WAN_ASSERT_THROWS(factory.create(OPEN_CLIP_VIT_BIGG_14, guard.backend, false, empty_map, ""));
    });

    suite.run("clip_factory_has_version", []() {
        using CLIPFactory = ModelFactory<CLIPTextModelRunner, CLIPVersion>;
        CLIPFactory factory;
        factory.register_version(OPENAI_CLIP_VIT_L_14,
            [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
                return std::make_unique<CLIPTextModelRunner>(
                    backend, offload, map, prefix, OPENAI_CLIP_VIT_L_14, /*with_final_ln=*/true);
            });

        WAN_ASSERT_TRUE(factory.has_version(OPENAI_CLIP_VIT_L_14));
        WAN_ASSERT_TRUE(!factory.has_version(OPEN_CLIP_VIT_H_14));
        WAN_ASSERT_TRUE(!factory.has_version(OPEN_CLIP_VIT_BIGG_14));
    });
}

// ---------------------------------------------------------------------------
// T5 factory tests
// ---------------------------------------------------------------------------

void test_t5_factory(TestSuite& suite) {
    suite.run("register_and_create_t5_versions", []() {
        using T5Factory = ModelFactory<T5Runner, T5Version>;
        T5Factory factory;

        // T5Version::STANDARD_T5 creator — explicit enum-to-bool conversion
        factory.register_version(T5Version::STANDARD_T5,
            [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
                return std::make_unique<T5Runner>(backend, offload, map, prefix, /*is_umt5=*/false);
            });
        // T5Version::UMT5 creator — explicit enum-to-bool conversion
        factory.register_version(T5Version::UMT5,
            [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
                return std::make_unique<T5Runner>(backend, offload, map, prefix, /*is_umt5=*/true);
            });

        WAN_ASSERT_EQ(factory.registered_versions().size(), size_t(2));

        BackendRAII guard_t5(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};

        // Create standard T5 and verify
        auto t5_runner = factory.create(T5Version::STANDARD_T5, guard_t5.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(t5_runner != nullptr);
        t5_runner->alloc_params_buffer();
        WAN_ASSERT_EQ(t5_runner->get_desc(), std::string("t5"));

        // Destroy t5_runner before backend is freed (BackendRAII destructs after block)
        t5_runner.reset();

        BackendRAII guard_umt5(ggml_backend_cpu_init());
        // Create UMT5 and verify
        auto umt5_runner = factory.create(T5Version::UMT5, guard_umt5.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(umt5_runner != nullptr);
        umt5_runner->alloc_params_buffer();
        WAN_ASSERT_EQ(umt5_runner->get_desc(), std::string("t5"));
    });
}

// ---------------------------------------------------------------------------
// VAE factory tests
// ---------------------------------------------------------------------------

void test_vae_factory(TestSuite& suite) {
    suite.run("register_and_create_vae_versions", []() {
        // Factory returns unique_ptr<VAE> (base class) created via AutoEncoderKL
        using VAEFactory = ModelFactory<VAE, SDVersion>;
        VAEFactory factory;

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

        WAN_ASSERT_EQ(factory.registered_versions().size(), size_t(4));

        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};

        auto vae_runner = factory.create(VERSION_SD1, guard.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(vae_runner != nullptr);
        vae_runner->alloc_params_buffer();
        WAN_ASSERT_EQ(vae_runner->get_desc(), std::string("vae"));
    });
}

// ---------------------------------------------------------------------------
// Flux transformer factory tests
// ---------------------------------------------------------------------------

void test_flux_factory(TestSuite& suite) {
    suite.run("register_and_create_flux_versions", []() {
        using FluxFactory = ModelFactory<Flux::FluxRunner, SDVersion>;
        FluxFactory factory;

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
        // NOTE: VERSION_FLEX_2 is correct (verified in src/model.h line 42).
        // It is NOT a typo for FLUX_2. VERSION_FLUX2 is a separate enum value.
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

        WAN_ASSERT_EQ(factory.registered_versions().size(), size_t(5));

        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};

        auto flux_runner = factory.create(VERSION_FLUX, guard.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(flux_runner != nullptr);
        flux_runner->alloc_params_buffer();
        WAN_ASSERT_EQ(flux_runner->get_desc(), std::string("flux"));
    });
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"Model Factory Tests"};
    test_clip_factory(suite);
    test_t5_factory(suite);
    test_vae_factory(suite);
    test_flux_factory(suite);
    return suite.report();
}
