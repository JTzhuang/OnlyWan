/**
 * @file wan-api.cpp
 * @brief C-style public API implementation for wan-cpp
 *
 * This file implements of C API declared in wan.h.
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "wan.h"
#include "wan-internal.hpp"

// Full runner headers required here — wan_context holds shared_ptr members
// whose destructors need complete types at the point of destruction.
#include "wan.hpp"
#include "t5.hpp"
#include "clip.hpp"
#include "ggml_extend.hpp"
#include "model.h"

#include <cstdarg>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>

/* ============================================================================
 * Platform-specific API export macros
 * ============================================================================ */

#if defined(_WIN32) || defined(__CYGWIN__)
#ifndef WAN_BUILD_SHARED_LIB
#define WAN_API
#else
#ifdef WAN_BUILD_DLL
#define WAN_API __declspec(dllexport)
#else
#define WAN_API __declspec(dllimport)
#endif
#endif
#else
#if __GNUC__ >= 4
#define WAN_API __attribute__((visibility("default")))
#else
#define WAN_API
#endif
#endif


/* ============================================================================
 * Error Handling
 * ============================================================================ */

static void set_last_error(wan_context_t* ctx, const char* error_msg) {
    if (ctx) {
        ctx->last_error = error_msg ? error_msg : "Unknown error";
    }
}

static void set_last_error_fmt(wan_context_t* ctx, const char* fmt, ...) {
    if (!ctx) return;

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    ctx->last_error = std::string(buffer);
}

/* ============================================================================
 * Global State
 * ============================================================================ */

static wan_log_cb_t g_log_callback = nullptr;
static void* g_log_user_data = nullptr;

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

WAN_API const char* wan_version(void) {
    return "1.0.0";
}

WAN_API const char* wan_get_last_error(wan_context_t* ctx) {
    if (ctx) {
        return ctx->last_error.c_str();
    }
    return "Invalid context";
}

WAN_API void wan_set_log_callback(wan_log_cb_t callback, void* user_data) {
    // Store callback in global state
    g_log_callback = callback;
    g_log_user_data = user_data;
}

/* ============================================================================
 * WanModel::load — Full GGUF weight loading via ModelLoader
 * Implemented here (not wan_loader.cpp) to avoid ODR violations: wan.hpp,
 * t5.hpp, and clip.hpp contain non-inline definitions that must appear in
 * exactly one TU. wan-api.cpp already includes all three headers.
 * ============================================================================ */

WanModelLoadResult Wan::WanModel::load(const std::string& file_path) {
    WanModelLoadResult result;
    result.success = false;

    // Validate file exists
    {
        std::ifstream test_file(file_path);
        if (!test_file.good()) {
            result.error_message = "Model file not found: " + file_path;
            return result;
        }
    }

    // Validate GGUF and read metadata (model_type, model_version)
    std::string model_type, model_version;
    if (!Wan::is_wan_gguf(file_path, model_type, model_version)) {
        result.error_message = "Not a valid Wan GGUF model: " + file_path;
        return result;
    }

    // Map model_type string to SDVersion
    SDVersion sd_version = VERSION_WAN2;
    if (model_type == "i2v")  sd_version = VERSION_WAN2_2_I2V;
    if (model_type == "ti2v") sd_version = VERSION_WAN2_2_TI2V;

    // Initialize CPU backend for weight loading
    ggml_backend_t backend = ggml_backend_cpu_init();
    if (!backend) {
        result.error_message = "Failed to initialize CPU backend for model loading";
        return result;
    }

    // Load all tensors from GGUF via ModelLoader
    ModelLoader model_loader;
    if (!model_loader.init_from_file_and_convert_name(file_path, "model.diffusion_model.")) {
        result.error_message = "ModelLoader failed to init from: " + file_path;
        ggml_backend_free(backend);
        return result;
    }
    auto& tensor_storage_map = model_loader.get_tensor_storage_map();

    // --- DiT (WanRunner) ---
    auto wan_runner = std::make_shared<WAN::WanRunner>(
        backend, /*offload_params_to_cpu=*/false,
        tensor_storage_map, "model.diffusion_model", sd_version);
    wan_runner->alloc_params_buffer();
    {
        std::map<std::string, ggml_tensor*> tensors;
        wan_runner->get_param_tensors(tensors, "model.diffusion_model");
        if (!model_loader.load_tensors(tensors)) {
            result.error_message = "Failed to load DiT tensors from: " + file_path;
            ggml_backend_free(backend);
            return result;
        }
    }

    // --- VAE (WanVAERunner) — prefix "first_stage_model" (no trailing dot) ---
    auto vae_runner = std::make_shared<WAN::WanVAERunner>(
        backend, /*offload_params_to_cpu=*/false,
        tensor_storage_map, "", /*decode_only=*/false, sd_version);
    vae_runner->alloc_params_buffer();
    {
        std::map<std::string, ggml_tensor*> vae_tensors;
        vae_runner->get_param_tensors(vae_tensors, "first_stage_model");
        // best-effort: VAE may be in a separate file
        model_loader.load_tensors(vae_tensors);
    }

    // --- T5 encoder — detect prefix dynamically (varies by GGUF file) ---
    std::string t5_prefix;
    for (auto& [name, _] : tensor_storage_map) {
        if (name.find("cond_stage_model.") == 0)    { t5_prefix = "cond_stage_model.";    break; }
        if (name.find("text_encoders.t5xxl.") == 0) { t5_prefix = "text_encoders.t5xxl."; break; }
    }
    std::shared_ptr<T5Embedder> t5_embedder;
    if (!t5_prefix.empty()) {
        t5_embedder = std::make_shared<T5Embedder>(
            backend, false, tensor_storage_map, t5_prefix, /*is_umt5=*/true);
        t5_embedder->alloc_params_buffer();
        std::map<std::string, ggml_tensor*> t5_tensors;
        t5_embedder->get_param_tensors(t5_tensors, t5_prefix);
        model_loader.load_tensors(t5_tensors);
    } else {
        // T5 not bundled in this GGUF — construct tokenizer-only embedder
        t5_embedder = std::make_shared<T5Embedder>(
            backend, false, tensor_storage_map, "", /*is_umt5=*/true);
    }

    // --- CLIP vision encoder — only for i2v / ti2v models ---
    std::shared_ptr<CLIPVisionModelProjectionRunner> clip_runner;
    if (model_type == "i2v" || model_type == "ti2v") {
        std::string clip_prefix;
        for (auto& [name, _] : tensor_storage_map) {
            if (name.find("cond_stage_model.visual.") == 0) { clip_prefix = "cond_stage_model.visual."; break; }
            if (name.find("clip_vision_model.") == 0)       { clip_prefix = "clip_vision_model.";       break; }
        }
        if (!clip_prefix.empty()) {
            clip_runner = std::make_shared<CLIPVisionModelProjectionRunner>(
                backend, false, tensor_storage_map, clip_prefix, OPEN_CLIP_VIT_H_14);
            clip_runner->alloc_params_buffer();
            std::map<std::string, ggml_tensor*> clip_tensors;
            clip_runner->get_param_tensors(clip_tensors, clip_prefix);
            model_loader.load_tensors(clip_tensors);
        }
    }

    result.success       = true;
    result.wan_runner    = wan_runner;
    result.vae_runner    = vae_runner;
    result.t5_embedder   = t5_embedder;
    result.clip_runner   = clip_runner;  // nullptr for t2v
    result.model_type    = model_type;
    result.model_version = model_version;
    return result;
}

/* ============================================================================
 * Model Loading
 * ============================================================================ */

WAN_API wan_error_t wan_load_model(const char* model_path,
                                           int n_threads,
                                           const char* backend_type,
                                           wan_context_t** out_ctx) {
    if (!model_path || !out_ctx) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    // Create context
    std::unique_ptr<wan_context> ctx(new wan_context());
    if (!ctx) {
        return WAN_ERROR_OUT_OF_MEMORY;
    }

    ctx->model_path = model_path;
    ctx->n_threads = n_threads;
    ctx->backend_type = backend_type ? backend_type : "cpu";

    // Load model weights via ModelLoader
    WanModelLoadResult result = Wan::WanModel::load(ctx->model_path);
    if (!result.success) {
        set_last_error(ctx.get(), result.error_message.c_str());
        return WAN_ERROR_MODEL_LOAD_FAILED;
    }

    ctx->wan_runner  = result.wan_runner;
    ctx->vae_runner  = result.vae_runner;
    ctx->t5_embedder = result.t5_embedder;
    ctx->clip_runner = result.clip_runner;
    ctx->model_type  = result.model_type;
    ctx->params.model_version = result.model_version;

    // Create backend
    ctx->backend = WanBackendPtr(Wan::WanBackend::create(ctx->backend_type, n_threads));
    if (!ctx->backend) {
        set_last_error(ctx.get(), "Failed to initialize backend");
        return WAN_ERROR_BACKEND_FAILED;
    }

    *out_ctx = ctx.release();
    return WAN_SUCCESS;
}

WAN_API wan_error_t wan_load_model_from_file(const char* model_path,
                                                 const wan_params_t* params,
                                                 wan_context_t** out_ctx) {
    int n_threads = params ? params->n_threads : 0;
    const char* backend_type = params && params->backend ? params->backend : "cpu";

    return wan_load_model(model_path, n_threads, backend_type, out_ctx);
}

/* ============================================================================
 * Model Info
 * ============================================================================ */

WAN_API wan_error_t wan_get_model_info(wan_context_t* ctx,
                                              char* out_version,
                                              size_t version_size) {
    if (!ctx || !ctx->wan_runner) {
        return WAN_ERROR_INVALID_STATE;
    }

    if (out_version && version_size > 0) {
        std::string version = ctx->params.model_version;
        strncpy(out_version, version.c_str(), version_size - 1);
        out_version[version_size - 1] = '\0';
    }

    return WAN_SUCCESS;
}

/* ============================================================================
 * Resource Cleanup
 * ============================================================================ */

WAN_API void wan_free(wan_context_t* ctx) {
    if (ctx) {
        // Unique pointers and shared pointers handle cleanup automatically
        delete ctx;
    }
}

/* ============================================================================
 * Image Loading (Stubs - to be implemented)
 * ============================================================================ */

WAN_API wan_error_t wan_load_image(const char* image_path, wan_image_t** out_image) {
    if (!image_path || !out_image) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }
    int w, h, c;
    uint8_t* data = stbi_load(image_path, &w, &h, &c, 3);  // force 3-channel RGB
    if (!data) {
        return WAN_ERROR_IMAGE_LOAD_FAILED;
    }
    wan_image_t* img = new wan_image_t();
    img->width    = w;
    img->height   = h;
    img->channels = 3;
    img->data     = data;  // stbi_load allocates with malloc; wan_free_image calls free()
    *out_image = img;
    return WAN_SUCCESS;
}

WAN_API void wan_free_image(wan_image_t* image) {
    if (image) {
        if (image->data) {
            free(image->data);
        }
        delete image;
    }
}

/* ============================================================================
 * T2V Generation — T5 encode + WanRunner::compute
 * Implemented here (not wan_t2v.cpp) to avoid ODR violations: wan.hpp and
 * t5.hpp contain non-inline definitions that must appear in exactly one TU.
 * ============================================================================ */

WAN_API wan_error_t wan_generate_video_t2v_ex(wan_context_t* ctx,
                                              const char* prompt,
                                              const wan_params_t* params,
                                              const char* output_path) {
    if (!ctx || !prompt || !output_path || !params) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }
    if (!ctx->wan_runner) {
        return WAN_ERROR_INVALID_STATE;
    }
    if (!ctx->t5_embedder) {
        return WAN_ERROR_INVALID_STATE;
    }

    int n_threads = ctx->n_threads > 0 ? ctx->n_threads : 1;

    // --- T5 encode ---
    ggml_init_params work_params = { 64 * 1024 * 1024, nullptr, false };
    ggml_context* work_ctx = ggml_init(work_params);
    if (!work_ctx) return WAN_ERROR_OUT_OF_MEMORY;

    auto [tokens, weights, attention_mask] =
        ctx->t5_embedder->tokenize(std::string(prompt), 512, true);

    if (tokens.empty()) {
        ggml_free(work_ctx);
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    ggml_tensor* input_ids = ggml_new_tensor_2d(work_ctx, GGML_TYPE_I32,
                                                 (int64_t)tokens.size(), 1);
    memcpy(input_ids->data, tokens.data(), tokens.size() * sizeof(int32_t));

    ggml_tensor* attn_mask = ggml_new_tensor_2d(work_ctx, GGML_TYPE_F32,
                                                 (int64_t)attention_mask.size(), 1);
    memcpy(attn_mask->data, attention_mask.data(),
           attention_mask.size() * sizeof(float));

    ggml_init_params out_params = { 256 * 1024 * 1024, nullptr, false };
    ggml_context* output_ctx = ggml_init(out_params);
    if (!output_ctx) {
        ggml_free(work_ctx);
        return WAN_ERROR_OUT_OF_MEMORY;
    }

    ggml_tensor* context = nullptr;
    ctx->t5_embedder->model.compute(n_threads, input_ids, attn_mask,
                                    &context, output_ctx);

    ggml_free(work_ctx);

    if (!context) {
        ggml_free(output_ctx);
        return WAN_ERROR_INVALID_STATE;
    }

    // --- WanRunner::compute (single denoising step stub — full loop in Phase 8) ---
    ggml_tensor* output = nullptr;
    ctx->wan_runner->compute(n_threads,
        /*x=*/nullptr, /*timesteps=*/nullptr,
        context,
        /*clip_fea=*/nullptr,
        /*c_concat=*/nullptr, /*time_dim_concat=*/nullptr,
        /*vace_context=*/nullptr, /*vace_strength=*/1.f,
        &output, output_ctx);

    ggml_free(output_ctx);

    // Phase 8 will: run full denoising loop, decode latents, write AVI
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

/* ============================================================================
 * I2V Generation — CLIP encode + WanRunner::compute
 * Implemented here (not wan_i2v.cpp) to avoid ODR violations: clip.hpp and
 * preprocessing.hpp contain non-inline definitions that must appear in
 * exactly one TU.
 * ============================================================================ */

WAN_API wan_error_t wan_generate_video_i2v_ex(wan_context_t* ctx,
                                              const wan_image_t* image,
                                              const char* prompt,
                                              const wan_params_t* params,
                                              const char* output_path) {
    if (!ctx || !image || !output_path || !params) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }
    if (!ctx->wan_runner) {
        return WAN_ERROR_INVALID_STATE;
    }
    if (!ctx->clip_runner) {
        return WAN_ERROR_INVALID_STATE;
    }
    if (!image->data || image->width <= 0 || image->height <= 0) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    int n_threads = ctx->n_threads > 0 ? ctx->n_threads : 1;

    // --- Convert wan_image_t to sd_image_t (identical layout) ---
    sd_image_t sd_img;
    sd_img.width   = image->width;
    sd_img.height  = image->height;
    sd_img.channel = image->channels;
    sd_img.data    = image->data;

    // --- Build pixel_values tensor for CLIP ---
    ggml_init_params work_params = { 128 * 1024 * 1024, nullptr, false };
    ggml_context* work_ctx = ggml_init(work_params);
    if (!work_ctx) return WAN_ERROR_OUT_OF_MEMORY;

    ggml_tensor* pixel_values = ggml_new_tensor_4d(work_ctx, GGML_TYPE_F32,
                                                    sd_img.width, sd_img.height,
                                                    sd_img.channel, 1);
    if (!pixel_values) {
        ggml_free(work_ctx);
        return WAN_ERROR_IMAGE_LOAD_FAILED;
    }
    sd_image_to_ggml_tensor(sd_img, pixel_values);

    // --- CLIP vision encode via ctx->clip_runner (weights loaded by wan_loader.cpp) ---
    // clip_fea shape: [N, 257, 1280] with return_pooled=false
    ggml_init_params out_params = { 256 * 1024 * 1024, nullptr, false };
    ggml_context* output_ctx = ggml_init(out_params);
    if (!output_ctx) {
        ggml_free(work_ctx);
        return WAN_ERROR_OUT_OF_MEMORY;
    }

    ggml_tensor* clip_fea = nullptr;
    ctx->clip_runner->compute(n_threads, pixel_values,
                              /*return_pooled=*/false, /*clip_skip=*/-1,
                              &clip_fea, output_ctx);

    ggml_free(work_ctx);

    if (!clip_fea) {
        ggml_free(output_ctx);
        return WAN_ERROR_INVALID_STATE;
    }

    // --- WanRunner::compute with real clip_fea (ENCODER-02) ---
    ggml_tensor* output = nullptr;
    ctx->wan_runner->compute(n_threads,
        /*x=*/nullptr, /*timesteps=*/nullptr,
        /*context=*/nullptr,
        clip_fea,
        /*c_concat=*/nullptr, /*time_dim_concat=*/nullptr,
        /*vace_context=*/nullptr, /*vace_strength=*/1.f,
        &output, output_ctx);

    ggml_free(output_ctx);

    // Phase 8 will: run full denoising loop, decode latents, write AVI
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

