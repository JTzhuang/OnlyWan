#include "model_factory.hpp"

// CLIP
REGISTER_MODEL_FACTORY(CLIPTextModelRunner, "clip-vit-l-14", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<CLIPTextModelRunner>(backend, offload, map, prefix, OPENAI_CLIP_VIT_L_14, true);
})

REGISTER_MODEL_FACTORY(CLIPTextModelRunner, "clip-vit-h-14", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<CLIPTextModelRunner>(backend, offload, map, prefix, OPEN_CLIP_VIT_H_14, true);
})

REGISTER_MODEL_FACTORY(CLIPTextModelRunner, "clip-vit-bigg-14", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<CLIPTextModelRunner>(backend, offload, map, prefix, OPEN_CLIP_VIT_BIGG_14, true);
})

// T5
REGISTER_MODEL_FACTORY(T5Runner, "t5-standard", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<T5Runner>(backend, offload, map, prefix, false);
})

REGISTER_MODEL_FACTORY(T5Runner, "t5-umt5", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<T5Runner>(backend, offload, map, prefix, true);
})

// VAE
REGISTER_MODEL_FACTORY(VAE, "vae-sd1", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<AutoEncoderKL>(backend, offload, map, prefix, true, false, VERSION_SD1);
})

REGISTER_MODEL_FACTORY(VAE, "vae-sd2", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<AutoEncoderKL>(backend, offload, map, prefix, true, false, VERSION_SD2);
})

REGISTER_MODEL_FACTORY(VAE, "vae-flux", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<AutoEncoderKL>(backend, offload, map, prefix, true, false, VERSION_FLUX);
})

REGISTER_MODEL_FACTORY(VAE, "vae-flux2", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<AutoEncoderKL>(backend, offload, map, prefix, true, false, VERSION_FLUX2);
})

// Flux
REGISTER_MODEL_FACTORY(FluxRunner, "flux-dev", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<FluxRunner>(backend, offload, map, prefix, VERSION_FLUX, false);
})

REGISTER_MODEL_FACTORY(FluxRunner, "flux-fill", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<FluxRunner>(backend, offload, map, prefix, VERSION_FLUX_FILL, false);
})

REGISTER_MODEL_FACTORY(FluxRunner, "flux-flex2", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<FluxRunner>(backend, offload, map, prefix, VERSION_FLEX_2, false);
})

REGISTER_MODEL_FACTORY(FluxRunner, "flux-chroma", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<FluxRunner>(backend, offload, map, prefix, VERSION_CHROMA_RADIANCE, false);
})

REGISTER_MODEL_FACTORY(FluxRunner, "flux-ovis", [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
    return std::make_unique<FluxRunner>(backend, offload, map, prefix, VERSION_OVIS_IMAGE, false);
})
