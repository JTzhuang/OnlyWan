#include "model_factory.hpp"

// ---- CLIP: 3 registrations ----
// All use with_final_ln=true (standard WAN usage pattern)
REGISTER_MODEL_FACTORY(CLIPTextModelRunner, "clip-vit-l-14",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<CLIPTextModelRunner>(
            backend, offload, map, prefix, OPENAI_CLIP_VIT_L_14, /*with_final_ln=*/true);
    })

REGISTER_MODEL_FACTORY(CLIPTextModelRunner, "clip-vit-h-14",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<CLIPTextModelRunner>(
            backend, offload, map, prefix, OPEN_CLIP_VIT_H_14, /*with_final_ln=*/true);
    })

REGISTER_MODEL_FACTORY(CLIPTextModelRunner, "clip-vit-bigg-14",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<CLIPTextModelRunner>(
            backend, offload, map, prefix, OPEN_CLIP_VIT_BIGG_14, /*with_final_ln=*/true);
    })

// ---- T5: 2 registrations ----
REGISTER_MODEL_FACTORY(T5Runner, "t5-standard",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<T5Runner>(backend, offload, map, prefix, /*is_umt5=*/false);
    })

REGISTER_MODEL_FACTORY(T5Runner, "t5-umt5",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<T5Runner>(backend, offload, map, prefix, /*is_umt5=*/true);
    })

// ---- WAN VAE: 4 registrations ----
// Uses WAN::WanVAERunner (from wan.hpp), NOT AutoEncoderKL.
// WAN::WanVAERunner::get_desc() returns "wan_vae".
REGISTER_MODEL_FACTORY(WAN::WanVAERunner, "wan-vae-t2v",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<WAN::WanVAERunner>(backend, offload, map, prefix, /*decode_only=*/false, VERSION_WAN2);
    })

REGISTER_MODEL_FACTORY(WAN::WanVAERunner, "wan-vae-t2v-decode",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<WAN::WanVAERunner>(backend, offload, map, prefix, /*decode_only=*/true, VERSION_WAN2);
    })

REGISTER_MODEL_FACTORY(WAN::WanVAERunner, "wan-vae-i2v",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<WAN::WanVAERunner>(backend, offload, map, prefix, /*decode_only=*/false, VERSION_WAN2_2_I2V);
    })

REGISTER_MODEL_FACTORY(WAN::WanVAERunner, "wan-vae-ti2v",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<WAN::WanVAERunner>(backend, offload, map, prefix, /*decode_only=*/false, VERSION_WAN2_2_TI2V);
    })

// ---- WAN Transformer (DiT): 3 registrations ----
// WAN::WanRunner supports T2V, I2V, and TI2V variants via SDVersion.
REGISTER_MODEL_FACTORY(WAN::WanRunner, "wan-runner-t2v",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<WAN::WanRunner>(backend, offload, map, prefix, VERSION_WAN2);
    })

REGISTER_MODEL_FACTORY(WAN::WanRunner, "wan-runner-i2v",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<WAN::WanRunner>(backend, offload, map, prefix, VERSION_WAN2_2_I2V);
    })

REGISTER_MODEL_FACTORY(WAN::WanRunner, "wan-runner-ti2v",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<WAN::WanRunner>(backend, offload, map, prefix, VERSION_WAN2_2_TI2V);
    })

// ---- Force-load function ----
// Called by tests to prevent linker dead-code elimination of this TU.
extern "C" void wan_force_model_registrations() {
    // Intentionally empty — its existence forces the linker to include this TU.
}
