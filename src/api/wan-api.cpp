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

#include "../../examples/cli/avi_writer.h"

#include <cstdarg>
#include <cstring>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#ifndef _WIN32
#  include <sys/stat.h>
#else
#  include <windows.h>
#endif

#include "../../src/vocab/vocab.h"

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

WAN_API wan_error_t wan_set_vocab_dir(const char* dir) {
#ifdef WAN_EMBED_VOCAB
    return WAN_ERROR_INVALID_ARGUMENT;
#endif
    if (!dir) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }
    wan_vocab_set_dir(std::string(dir));
    return WAN_SUCCESS;
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

    // Detect format: safetensors takes priority over GGUF
    std::string model_type, model_version;
    SDVersion sd_version = VERSION_WAN2;

    bool is_st   = is_safetensors_file(file_path);
    bool is_gguf = !is_st && Wan::is_wan_gguf(file_path, model_type, model_version);

    if (!is_st && !is_gguf) {
        result.error_message = "Not a valid WAN GGUF or safetensors file: " + file_path;
        return result;
    }

    // Initialize CPU backend for weight loading
    ggml_backend_t backend = ggml_backend_cpu_init();
    if (!backend) {
        result.error_message = "Failed to initialize CPU backend for model loading";
        return result;
    }

    ModelLoader model_loader;

    if (is_st) {
        // Safetensors path: HF WAN checkpoints already use "model.diffusion_model.*" names.
        // Do NOT pass a prefix — adding it would double the prefix and break all lookups.
        if (!model_loader.init_from_file(file_path)) {
            result.error_message = "Failed to parse safetensors file: " + file_path;
            ggml_backend_free(backend);
            return result;
        }
        // Normalize any variant (diffusers-style) tensor names
        model_loader.convert_tensors_name();

        // Infer WAN model type from tensor names (no metadata in safetensors)
        SDVersion sv = model_loader.get_sd_version();
        if (!sd_version_is_wan(sv)) {
            result.error_message = "safetensors file does not contain a WAN model: " + file_path;
            ggml_backend_free(backend);
            return result;
        }
        sd_version = sv;
        if (sv == VERSION_WAN2_2_I2V)       { model_type = "i2v";   model_version = "WAN2.2"; }
        else if (sv == VERSION_WAN2_2_TI2V) { model_type = "ti2v";  model_version = "WAN2.2"; }
        else                                { model_type = "t2v";   model_version = "WAN2.1"; }
    } else {
        // GGUF path: existing logic — model_type and model_version already set by is_wan_gguf()
        if (model_type == "i2v")  sd_version = VERSION_WAN2_2_I2V;
        if (model_type == "ti2v") sd_version = VERSION_WAN2_2_TI2V;

        if (!model_loader.init_from_file_and_convert_name(file_path, "model.diffusion_model.")) {
            result.error_message = "ModelLoader failed to init from: " + file_path;
            ggml_backend_free(backend);
            return result;
        }
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

#ifndef WAN_EMBED_VOCAB
    if (!wan_vocab_dir_is_set()) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }
    {
        const std::string& vdir = wan_vocab_get_dir();
#ifndef _WIN32
        struct stat st;
        if (stat(vdir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
            return WAN_ERROR_INVALID_ARGUMENT;
        }
#else
        DWORD attr = GetFileAttributesA(vdir.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            return WAN_ERROR_INVALID_ARGUMENT;
        }
#endif
    }
#endif

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
    ctx->backend = WanBackendPtr(Wan::WanBackend::create(ctx->backend_type, n_threads, 0));
    if (!ctx->backend) {
        set_last_error(ctx.get(), "Failed to initialize backend");
        return WAN_ERROR_BACKEND_FAILED;
    }

    // OP-01: Auto-enable Flash Attention for non-CPU backends
    ggml_backend_t backend = ctx->backend->backend;
    if (backend && !ggml_backend_is_cpu(backend)) {
        if (ctx->wan_runner) {
            ctx->wan_runner->set_flash_attention_enabled(true);
        }
        if (ctx->vae_runner) {
            ctx->vae_runner->set_flash_attention_enabled(true);
        }
        if (ctx->clip_runner) {
            ctx->clip_runner->set_flash_attention_enabled(true);
        }
        LOG_INFO("Auto-enabled flash attention for non-CPU backend");
    }

    *out_ctx = ctx.release();
    return WAN_SUCCESS;
}

WAN_API wan_error_t wan_load_model_from_file(const char* model_path,
                                                 const wan_params_t* params,
                                                 wan_context_t** out_ctx) {
    if (!params) {
        return wan_load_model(model_path, 0, "cpu", out_ctx);
    }

    int n_threads = params->n_threads;
    const char* backend_type = params->backend ? params->backend : "cpu";

#ifdef WAN_USE_MULTI_GPU
    // Multi-GPU path: num_gpus > 1 triggers tensor parallel initialization
    if (params->num_gpus > 1 && params->gpu_ids) {
        std::vector<int> gpu_ids(params->gpu_ids, params->gpu_ids + params->num_gpus);

        // Create multi-GPU state with device validation
        MultiGPUState* multi_gpu = create_multi_gpu_state(gpu_ids, params->distribution_strategy);
        if (!multi_gpu) {
            return WAN_ERROR_GPU_FAILURE;
        }

        // Load model via standard path first
        wan_error_t err = wan_load_model(model_path, n_threads, backend_type, out_ctx);
        if (err != WAN_SUCCESS) {
            delete multi_gpu;
            return err;
        }

        wan_context_t* ctx = *out_ctx;
        ctx->multi_gpu_state.reset(multi_gpu);
        ctx->params.num_gpus = params->num_gpus;
        ctx->params.distribution_strategy = params->distribution_strategy;
        ctx->params.gpu_ids = gpu_ids;

        // Create backend scheduler for multi-GPU coordination
#ifdef WAN_USE_CUDA
        ggml_backend_sched_t sched = ggml_backend_sched_new(
            multi_gpu->backends.data(),
            nullptr,  // backend_ids (NULL = use default)
            multi_gpu->backends.size(),
            GGML_DEFAULT_GRAPH_SIZE,
            false  // parallel
        );

        if (!sched) {
            set_last_error(ctx, "Failed to create backend scheduler");
            return WAN_ERROR_GPU_FAILURE;
        }

        multi_gpu->scheduler = sched;
        LOG_INFO("Multi-GPU scheduler created for %d GPUs", params->num_gpus);

        // Allocate model weights with tensor split across GPUs
        int main_device = gpu_ids[0];
        std::vector<float> tensor_split(params->num_gpus, 1.0f / params->num_gpus);

        ggml_backend_buffer_type_t split_buft = ggml_backend_cuda_split_buffer_type(
            main_device,
            tensor_split.data()
        );

        // Re-allocate params buffers with split buffer type
        if (ctx->wan_runner) {
            if (!ctx->wan_runner->alloc_params_buffer_split(split_buft)) {
                set_last_error(ctx, "Failed to allocate split buffer for WanRunner");
                return WAN_ERROR_GPU_FAILURE;
            }
        }
        if (ctx->vae_runner) {
            if (!ctx->vae_runner->alloc_params_buffer_split(split_buft)) {
                set_last_error(ctx, "Failed to allocate split buffer for VAERunner");
                return WAN_ERROR_GPU_FAILURE;
            }
        }
        if (ctx->t5_embedder) {
            if (!ctx->t5_embedder->alloc_params_buffer_split(split_buft)) {
                set_last_error(ctx, "Failed to allocate split buffer for T5Embedder");
                return WAN_ERROR_GPU_FAILURE;
            }
        }
        if (ctx->clip_runner) {
            if (!ctx->clip_runner->alloc_params_buffer_split(split_buft)) {
                set_last_error(ctx, "Failed to allocate split buffer for CLIPRunner");
                return WAN_ERROR_GPU_FAILURE;
            }
        }

        // Override primary backend to target main_device
        ctx->backend.reset(Wan::WanBackend::create("cuda", n_threads, main_device));
        if (!ctx->backend || !ctx->backend->backend) {
            set_last_error(ctx, "Failed to create main device backend");
            return WAN_ERROR_GPU_FAILURE;
        }

        LOG_INFO("Multi-GPU tensor parallel initialization complete: %d GPUs, main_device=%d",
                 params->num_gpus, main_device);

        return WAN_SUCCESS;
#else
        set_last_error(ctx, "Multi-GPU requires CUDA support");
        return WAN_ERROR_UNSUPPORTED_OPERATION;
#endif
    }
#endif

    // Single-GPU fallback
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
 * WAN 16-channel latent normalization constants (from stable-diffusion.cpp:2424-2427)
 * ============================================================================ */

static const float wan_latents_mean[16] = {
    -0.7571f, -0.7089f, -0.9113f,  0.1075f, -0.1745f,  0.9653f, -0.1517f,  1.5508f,
     0.4134f, -0.0715f,  0.5517f, -0.3632f, -0.1922f, -0.9497f,  0.2503f, -0.2921f};
static const float wan_latents_std[16] = {
    2.8184f, 1.4541f, 2.3275f, 2.6558f, 1.2196f, 1.7708f, 2.6052f, 2.0743f,
    3.2687f, 2.1526f, 2.8652f, 1.5579f, 1.6382f, 1.1253f, 2.8251f, 1.9160f};

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

    // --- Latent dimensions ---
    int64_t lW = params->width  / 8;
    int64_t lH = params->height / 8;
    int64_t lT = (params->num_frames - 1) / 4 + 1;
    int steps  = params->steps > 0 ? params->steps : 20;

    // --- Linear sigma schedule: 1.0 -> 0.0 ---
    std::vector<float> sigmas(steps + 1);
    for (int i = 0; i <= steps; i++)
        sigmas[i] = 1.0f - (float)i / (float)steps;

    // --- Denoising work context ---
    size_t denoise_mem = (size_t)(lW * lH * lT * 16 * 4) * 32 + 64 * 1024 * 1024;
    ggml_init_params denoise_params = { denoise_mem, nullptr, false };
    ggml_context* denoise_ctx = ggml_init(denoise_params);
    if (!denoise_ctx) { ggml_free(output_ctx); return WAN_ERROR_OUT_OF_MEMORY; }

    // --- Initialize noise latent [lW, lH, lT, 16] with randn * sigma[0] ---
    ggml_tensor* x = ggml_new_tensor_4d(denoise_ctx, GGML_TYPE_F32, lW, lH, lT, 16);
    {
        std::mt19937 rng(params->seed >= 0 ? (uint32_t)params->seed : 42u);
        std::normal_distribution<float> nd(0.0f, 1.0f);
        float* xd = (float*)x->data;
        int64_t n = ggml_nelements(x);
        for (int64_t i = 0; i < n; i++) xd[i] = nd(rng) * sigmas[0];
    }

    // --- process_latent_in: normalize channel-wise ---
    for (int64_t c = 0; c < 16; c++)
        for (int64_t t = 0; t < lT; t++)
            for (int64_t h = 0; h < lH; h++)
                for (int64_t w = 0; w < lW; w++) {
                    float* v = (float*)((char*)x->data +
                        w*x->nb[0] + h*x->nb[1] + t*x->nb[2] + c*x->nb[3]);
                    *v = (*v - wan_latents_mean[c]) / wan_latents_std[c];
                }

    // --- Unconditional T5 context for CFG (empty prompt) ---
    ggml_tensor* uncond_context = nullptr;
    {
        ggml_init_params uc_params = { 64 * 1024 * 1024, nullptr, false };
        ggml_context* uc_work = ggml_init(uc_params);
        if (uc_work) {
            auto [uc_tok, uc_w, uc_mask] =
                ctx->t5_embedder->tokenize(std::string(""), 512, true);
            if (!uc_tok.empty()) {
                ggml_tensor* uc_ids = ggml_new_tensor_2d(uc_work, GGML_TYPE_I32,
                                                          (int64_t)uc_tok.size(), 1);
                memcpy(uc_ids->data, uc_tok.data(), uc_tok.size() * sizeof(int32_t));
                ggml_tensor* uc_attn = ggml_new_tensor_2d(uc_work, GGML_TYPE_F32,
                                                           (int64_t)uc_mask.size(), 1);
                memcpy(uc_attn->data, uc_mask.data(), uc_mask.size() * sizeof(float));
                ctx->t5_embedder->model.compute(n_threads, uc_ids, uc_attn,
                                                &uncond_context, output_ctx);
            }
            ggml_free(uc_work);
        }
    }

    // --- Euler loop ---
    float cfg_scale = params->cfg;
    for (int i = 0; i < steps; i++) {
        float sigma    = sigmas[i];
        float sigma_dt = sigmas[i + 1] - sigma;

        ggml_init_params step_params = { denoise_mem, nullptr, false };
        ggml_context* step_ctx = ggml_init(step_params);
        if (!step_ctx) {
            ggml_free(denoise_ctx); ggml_free(output_ctx);
            return WAN_ERROR_OUT_OF_MEMORY;
        }

        ggml_tensor* ts = ggml_new_tensor_1d(step_ctx, GGML_TYPE_F32, 1);
        ggml_set_f32(ts, sigma);

        ggml_tensor* cond_out = nullptr;
#ifdef WAN_USE_MULTI_GPU
        if (ctx->is_multi_gpu()) {
            ctx->wan_runner->compute_with_sched(ctx->multi_gpu_state->scheduler, n_threads,
                                                x, ts, context,
                                                nullptr, nullptr, nullptr, nullptr, 1.f,
                                                &cond_out, step_ctx);
        } else {
#endif
            ctx->wan_runner->compute(n_threads, x, ts, context,
                                     nullptr, nullptr, nullptr, nullptr, 1.f,
                                     &cond_out, step_ctx);
#ifdef WAN_USE_MULTI_GPU
        }
#endif

        ggml_tensor* uncond_out = nullptr;
        if (uncond_context && cfg_scale != 1.0f) {
#ifdef WAN_USE_MULTI_GPU
            if (ctx->is_multi_gpu()) {
                ctx->wan_runner->compute_with_sched(ctx->multi_gpu_state->scheduler, n_threads,
                                                    x, ts, uncond_context,
                                                    nullptr, nullptr, nullptr, nullptr, 1.f,
                                                    &uncond_out, step_ctx);
            } else {
#endif
                ctx->wan_runner->compute(n_threads, x, ts, uncond_context,
                                         nullptr, nullptr, nullptr, nullptr, 1.f,
                                         &uncond_out, step_ctx);
#ifdef WAN_USE_MULTI_GPU
            }
#endif
        }

        float* xd  = (float*)x->data;
        float* cd  = cond_out  ? (float*)cond_out->data  : nullptr;
        float* ucd = uncond_out ? (float*)uncond_out->data : cd;
        if (cd) {
            int64_t n = ggml_nelements(x);
            for (int64_t j = 0; j < n; j++) {
                float denoised = ucd[j] + cfg_scale * (cd[j] - ucd[j]);
                float d        = (xd[j] - denoised) / (sigma > 1e-6f ? sigma : 1e-6f);
                xd[j]          = xd[j] + d * sigma_dt;
            }
        }
        ggml_free(step_ctx);
        if (params->progress_cb) {
            int abort = params->progress_cb(i, steps,
                                            (float)(i + 1) / (float)steps,
                                            params->user_data);
            if (abort) {
                ggml_free(denoise_ctx);
                ggml_free(output_ctx);
                return WAN_ERROR_GENERATION_FAILED;
            }
        }
    }

    // --- process_latent_out: inverse normalize ---
    for (int64_t c = 0; c < 16; c++)
        for (int64_t t = 0; t < lT; t++)
            for (int64_t h = 0; h < lH; h++)
                for (int64_t w = 0; w < lW; w++) {
                    float* v = (float*)((char*)x->data +
                        w*x->nb[0] + h*x->nb[1] + t*x->nb[2] + c*x->nb[3]);
                    *v = *v * wan_latents_std[c] + wan_latents_mean[c];
                }

    // --- VAE decode ---
    ggml_init_params vae_params = { 512 * 1024 * 1024, nullptr, false };
    ggml_context* vae_ctx = ggml_init(vae_params);
    if (!vae_ctx) {
        ggml_free(denoise_ctx); ggml_free(output_ctx);
        return WAN_ERROR_OUT_OF_MEMORY;
    }
    ggml_tensor* decoded = nullptr;
    ctx->vae_runner->compute(n_threads, x, /*decode_graph=*/true, &decoded, vae_ctx);
    if (!decoded) {
        ggml_free(vae_ctx); ggml_free(denoise_ctx); ggml_free(output_ctx);
        return WAN_ERROR_BACKEND_FAILED;
    }
    ggml_ext_tensor_clamp_inplace(decoded, 0.f, 1.f);

    // --- Float -> uint8 frames ---
    int T_out = (int)decoded->ne[2];
    int W     = (int)decoded->ne[0];
    int H     = (int)decoded->ne[1];
    std::vector<std::vector<uint8_t>> frame_bufs(T_out, std::vector<uint8_t>(W * H * 3));
    for (int t = 0; t < T_out; t++)
        for (int h = 0; h < H; h++)
            for (int w = 0; w < W; w++)
                for (int c = 0; c < 3; c++) {
                    float v = ggml_get_f32_nd(decoded, w, h, t, c);
                    frame_bufs[t][(h * W + w) * 3 + c] = (uint8_t)(v * 255.0f + 0.5f);
                }

    // --- Write AVI ---
    std::vector<const uint8_t*> frame_ptrs(T_out);
    for (int t = 0; t < T_out; t++) frame_ptrs[t] = frame_bufs[t].data();
    int fps = params->fps > 0 ? params->fps : 16;
    int ret = create_mjpg_avi_from_rgb_frames(output_path, frame_ptrs.data(),
                                               T_out, W, H, fps, 90);
    ggml_free(vae_ctx);
    ggml_free(denoise_ctx);
    ggml_free(output_ctx);
    return (ret == 0) ? WAN_SUCCESS : WAN_ERROR_BACKEND_FAILED;
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

    // --- T5 encode prompt (optional for I2V) ---
    ggml_tensor* context = nullptr;
    if (ctx->t5_embedder && prompt && prompt[0] != '\0') {
        ggml_init_params t5_work_params = { 64 * 1024 * 1024, nullptr, false };
        ggml_context* t5_work = ggml_init(t5_work_params);
        if (t5_work) {
            auto [tok, wts, mask] = ctx->t5_embedder->tokenize(std::string(prompt), 512, true);
            if (!tok.empty()) {
                ggml_tensor* t5_ids = ggml_new_tensor_2d(t5_work, GGML_TYPE_I32,
                                                          (int64_t)tok.size(), 1);
                memcpy(t5_ids->data, tok.data(), tok.size() * sizeof(int32_t));
                ggml_tensor* t5_attn = ggml_new_tensor_2d(t5_work, GGML_TYPE_F32,
                                                           (int64_t)mask.size(), 1);
                memcpy(t5_attn->data, mask.data(), mask.size() * sizeof(float));
                ctx->t5_embedder->model.compute(n_threads, t5_ids, t5_attn,
                                                &context, output_ctx);
            }
            ggml_free(t5_work);
        }
    }

    // --- VAE-encode input image to get c_concat ---
    ggml_init_params img_enc_params = { 128 * 1024 * 1024, nullptr, false };
    ggml_context* img_enc_ctx = ggml_init(img_enc_params);
    if (!img_enc_ctx) { ggml_free(output_ctx); return WAN_ERROR_OUT_OF_MEMORY; }

    int iW = image->width, iH = image->height;
    ggml_tensor* img_tensor = ggml_new_tensor_4d(img_enc_ctx, GGML_TYPE_F32, iW, iH, 3, 1);
    {
        float* dst = (float*)img_tensor->data;
        const uint8_t* src = image->data;
        for (int c = 0; c < 3; c++)
            for (int h = 0; h < iH; h++)
                for (int w = 0; w < iW; w++)
                    dst[c * iH * iW + h * iW + w] =
                        (float)src[(h * iW + w) * 3 + c] / 127.5f - 1.0f;
    }
    ggml_tensor* c_concat = nullptr;
    ctx->vae_runner->compute(n_threads, img_tensor, /*decode_graph=*/false, &c_concat, img_enc_ctx);
    if (!c_concat) {
        ggml_free(img_enc_ctx); ggml_free(output_ctx);
        return WAN_ERROR_BACKEND_FAILED;
    }

    // --- Latent dimensions and sigma schedule ---
    int64_t lW = params->width  / 8;
    int64_t lH = params->height / 8;
    int64_t lT = (params->num_frames - 1) / 4 + 1;
    int steps  = params->steps > 0 ? params->steps : 20;
    std::vector<float> sigmas(steps + 1);
    for (int i = 0; i <= steps; i++)
        sigmas[i] = 1.0f - (float)i / (float)steps;

    // --- Denoising work context ---
    size_t denoise_mem = (size_t)(lW * lH * lT * 16 * 4) * 32 + 64 * 1024 * 1024;
    ggml_init_params denoise_params = { denoise_mem, nullptr, false };
    ggml_context* denoise_ctx = ggml_init(denoise_params);
    if (!denoise_ctx) {
        ggml_free(img_enc_ctx); ggml_free(output_ctx);
        return WAN_ERROR_OUT_OF_MEMORY;
    }

    // --- Initialize noise latent ---
    ggml_tensor* x = ggml_new_tensor_4d(denoise_ctx, GGML_TYPE_F32, lW, lH, lT, 16);
    {
        std::mt19937 rng(params->seed >= 0 ? (uint32_t)params->seed : 42u);
        std::normal_distribution<float> nd(0.0f, 1.0f);
        float* xd = (float*)x->data;
        int64_t n = ggml_nelements(x);
        for (int64_t i = 0; i < n; i++) xd[i] = nd(rng) * sigmas[0];
    }

    // --- process_latent_in: normalize channel-wise ---
    for (int64_t c = 0; c < 16; c++)
        for (int64_t t = 0; t < lT; t++)
            for (int64_t h = 0; h < lH; h++)
                for (int64_t w = 0; w < lW; w++) {
                    float* v = (float*)((char*)x->data +
                        w*x->nb[0] + h*x->nb[1] + t*x->nb[2] + c*x->nb[3]);
                    *v = (*v - wan_latents_mean[c]) / wan_latents_std[c];
                }

    // --- Unconditional T5 context for CFG ---
    ggml_tensor* uncond_context = nullptr;
    if (ctx->t5_embedder) {
        ggml_init_params uc_params = { 64 * 1024 * 1024, nullptr, false };
        ggml_context* uc_work = ggml_init(uc_params);
        if (uc_work) {
            auto [uc_tok, uc_w, uc_mask] =
                ctx->t5_embedder->tokenize(std::string(""), 512, true);
            if (!uc_tok.empty()) {
                ggml_tensor* uc_ids = ggml_new_tensor_2d(uc_work, GGML_TYPE_I32,
                                                          (int64_t)uc_tok.size(), 1);
                memcpy(uc_ids->data, uc_tok.data(), uc_tok.size() * sizeof(int32_t));
                ggml_tensor* uc_attn = ggml_new_tensor_2d(uc_work, GGML_TYPE_F32,
                                                           (int64_t)uc_mask.size(), 1);
                memcpy(uc_attn->data, uc_mask.data(), uc_mask.size() * sizeof(float));
                ctx->t5_embedder->model.compute(n_threads, uc_ids, uc_attn,
                                                &uncond_context, output_ctx);
            }
            ggml_free(uc_work);
        }
    }

    // --- Euler loop (I2V: pass clip_fea and c_concat on conditional pass) ---
    float cfg_scale = params->cfg;
    for (int i = 0; i < steps; i++) {
        float sigma    = sigmas[i];
        float sigma_dt = sigmas[i + 1] - sigma;

        ggml_init_params step_params = { denoise_mem, nullptr, false };
        ggml_context* step_ctx = ggml_init(step_params);
        if (!step_ctx) {
            ggml_free(denoise_ctx); ggml_free(img_enc_ctx); ggml_free(output_ctx);
            return WAN_ERROR_OUT_OF_MEMORY;
        }

        ggml_tensor* ts = ggml_new_tensor_1d(step_ctx, GGML_TYPE_F32, 1);
        ggml_set_f32(ts, sigma);

        ggml_tensor* cond_out = nullptr;
#ifdef WAN_USE_MULTI_GPU
        if (ctx->is_multi_gpu()) {
            ctx->wan_runner->compute_with_sched(ctx->multi_gpu_state->scheduler, n_threads,
                                                x, ts, context,
                                                clip_fea, c_concat,
                                                nullptr, nullptr, 1.f,
                                                &cond_out, step_ctx);
        } else {
#endif
            ctx->wan_runner->compute(n_threads, x, ts, context,
                                     clip_fea, c_concat,
                                     nullptr, nullptr, 1.f,
                                     &cond_out, step_ctx);
#ifdef WAN_USE_MULTI_GPU
        }
#endif

        ggml_tensor* uncond_out = nullptr;
        if (uncond_context && cfg_scale != 1.0f) {
#ifdef WAN_USE_MULTI_GPU
            if (ctx->is_multi_gpu()) {
                ctx->wan_runner->compute_with_sched(ctx->multi_gpu_state->scheduler, n_threads,
                                                    x, ts, uncond_context,
                                                    nullptr, nullptr,
                                                    nullptr, nullptr, 1.f,
                                                    &uncond_out, step_ctx);
            } else {
#endif
                ctx->wan_runner->compute(n_threads, x, ts, uncond_context,
                                         nullptr, nullptr,
                                         nullptr, nullptr, 1.f,
                                         &uncond_out, step_ctx);
#ifdef WAN_USE_MULTI_GPU
            }
#endif
        }

        float* xd  = (float*)x->data;
        float* cd  = cond_out  ? (float*)cond_out->data  : nullptr;
        float* ucd = uncond_out ? (float*)uncond_out->data : cd;
        if (cd) {
            int64_t n = ggml_nelements(x);
            for (int64_t j = 0; j < n; j++) {
                float denoised = ucd[j] + cfg_scale * (cd[j] - ucd[j]);
                float d        = (xd[j] - denoised) / (sigma > 1e-6f ? sigma : 1e-6f);
                xd[j]          = xd[j] + d * sigma_dt;
            }
        }
        ggml_free(step_ctx);
        if (params->progress_cb) {
            int abort = params->progress_cb(i, steps,
                                            (float)(i + 1) / (float)steps,
                                            params->user_data);
            if (abort) {
                ggml_free(denoise_ctx);
                ggml_free(img_enc_ctx);
                ggml_free(output_ctx);
                return WAN_ERROR_GENERATION_FAILED;
            }
        }
    }

    // --- process_latent_out: inverse normalize ---
    for (int64_t c = 0; c < 16; c++)
        for (int64_t t = 0; t < lT; t++)
            for (int64_t h = 0; h < lH; h++)
                for (int64_t w = 0; w < lW; w++) {
                    float* v = (float*)((char*)x->data +
                        w*x->nb[0] + h*x->nb[1] + t*x->nb[2] + c*x->nb[3]);
                    *v = *v * wan_latents_std[c] + wan_latents_mean[c];
                }

    // --- VAE decode ---
    ggml_init_params vae_params = { 512 * 1024 * 1024, nullptr, false };
    ggml_context* vae_ctx = ggml_init(vae_params);
    if (!vae_ctx) {
        ggml_free(denoise_ctx); ggml_free(img_enc_ctx); ggml_free(output_ctx);
        return WAN_ERROR_OUT_OF_MEMORY;
    }
    ggml_tensor* decoded = nullptr;
    ctx->vae_runner->compute(n_threads, x, /*decode_graph=*/true, &decoded, vae_ctx);
    if (!decoded) {
        ggml_free(vae_ctx); ggml_free(denoise_ctx); ggml_free(img_enc_ctx); ggml_free(output_ctx);
        return WAN_ERROR_BACKEND_FAILED;
    }
    ggml_ext_tensor_clamp_inplace(decoded, 0.f, 1.f);

    // --- Float -> uint8 frames ---
    int T_out = (int)decoded->ne[2];
    int W     = (int)decoded->ne[0];
    int H     = (int)decoded->ne[1];
    std::vector<std::vector<uint8_t>> frame_bufs(T_out, std::vector<uint8_t>(W * H * 3));
    for (int t = 0; t < T_out; t++)
        for (int h = 0; h < H; h++)
            for (int w = 0; w < W; w++)
                for (int c = 0; c < 3; c++) {
                    float v = ggml_get_f32_nd(decoded, w, h, t, c);
                    frame_bufs[t][(h * W + w) * 3 + c] = (uint8_t)(v * 255.0f + 0.5f);
                }

    // --- Write AVI ---
    std::vector<const uint8_t*> frame_ptrs(T_out);
    for (int t = 0; t < T_out; t++) frame_ptrs[t] = frame_bufs[t].data();
    int fps = params->fps > 0 ? params->fps : 16;
    int ret = create_mjpg_avi_from_rgb_frames(output_path, frame_ptrs.data(),
                                               T_out, W, H, fps, 90);
    ggml_free(vae_ctx);
    ggml_free(denoise_ctx);
    ggml_free(img_enc_ctx);
    ggml_free(output_ctx);
    return (ret == 0) ? WAN_SUCCESS : WAN_ERROR_BACKEND_FAILED;
}

/* ============================================================================
 * Multi-GPU Batch Generation (Data Parallel)
 * ============================================================================ */

#ifdef WAN_USE_MULTI_GPU

#include <thread>
#include <mutex>

namespace {

struct BatchWorkerContext {
    int request_id;
    int gpu_id;
    std::string model_path;
    std::string prompt;
    std::string output_path;
    wan_params_t params;
    wan_batch_result_t* result;
};

void batch_worker_thread(BatchWorkerContext* ctx) {
    // Load model on specific GPU
    wan_context_t* wan_ctx = nullptr;
    wan_error_t err = wan_load_model(ctx->model_path.c_str(),
                                      ctx->params.n_threads,
                                      ctx->params.backend_type,
                                      &wan_ctx);

    if (err != WAN_SUCCESS) {
        ctx->result->error = err;
        snprintf(ctx->result->error_message, sizeof(ctx->result->error_message),
                 "Failed to load model on GPU %d", ctx->gpu_id);
        return;
    }

    // Override backend to target specific device
    wan_ctx->backend.reset(Wan::WanBackend::create_on_device(
        ctx->params.backend_type ? ctx->params.backend_type : "cuda",
        ctx->params.n_threads,
        ctx->gpu_id
    ));

    if (!wan_ctx->backend || !wan_ctx->backend->backend) {
        ctx->result->error = WAN_ERROR_GPU_FAILURE;
        snprintf(ctx->result->error_message, sizeof(ctx->result->error_message),
                 "Failed to create backend on GPU %d", ctx->gpu_id);
        wan_free(wan_ctx);
        return;
    }

    // Generate video
    err = wan_generate_video_t2v_ex(wan_ctx, ctx->prompt.c_str(),
                                     &ctx->params, ctx->output_path.c_str());

    ctx->result->error = err;
    if (err != WAN_SUCCESS) {
        const char* error_msg = wan_get_last_error(wan_ctx);
        snprintf(ctx->result->error_message, sizeof(ctx->result->error_message),
                 "GPU %d: %s", ctx->gpu_id, error_msg ? error_msg : "Generation failed");
    } else {
        ctx->result->error_message[0] = '\0';
    }

    wan_free(wan_ctx);
}

} // anonymous namespace

WAN_API wan_error_t wan_generate_batch_t2v(const char* model_path,
                                            const char** prompts,
                                            const char** output_paths,
                                            int batch_size,
                                            const wan_params_t* params,
                                            wan_batch_result_t* results) {
    if (!model_path || !prompts || !output_paths || !params || !results || batch_size <= 0) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    if (!params->gpu_ids || params->num_gpus <= 0) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    // Create worker contexts
    std::vector<BatchWorkerContext> contexts(batch_size);
    std::vector<std::thread> threads;
    threads.reserve(batch_size);

    // Initialize results
    for (int i = 0; i < batch_size; i++) {
        results[i].error = WAN_SUCCESS;
        results[i].error_message[0] = '\0';
    }

    // Launch worker threads with round-robin GPU assignment
    for (int i = 0; i < batch_size; i++) {
        contexts[i].request_id = i;
        contexts[i].gpu_id = params->gpu_ids[i % params->num_gpus];
        contexts[i].model_path = model_path;
        contexts[i].prompt = prompts[i];
        contexts[i].output_path = output_paths[i];
        contexts[i].params = *params;
        contexts[i].result = &results[i];

        threads.emplace_back(batch_worker_thread, &contexts[i]);
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Check if any request failed
    bool any_failed = false;
    for (int i = 0; i < batch_size; i++) {
        if (results[i].error != WAN_SUCCESS) {
            any_failed = true;
        }
    }

    return any_failed ? WAN_ERROR_GENERATION_FAILED : WAN_SUCCESS;
}

#else

WAN_API wan_error_t wan_generate_batch_t2v(const char* model_path,
                                            const char** prompts,
                                            const char** output_paths,
                                            int batch_size,
                                            const wan_params_t* params,
                                            wan_batch_result_t* results) {
    (void)model_path;
    (void)prompts;
    (void)output_paths;
    (void)batch_size;
    (void)params;
    (void)results;
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

#endif /* WAN_USE_MULTI_GPU */

/* ============================================================================
 * GPU Info Query
 * ============================================================================ */

WAN_API wan_error_t wan_get_gpu_info(int* device_count, char** device_names, int max_devices) {
    if (!device_count) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

#ifdef GGML_USE_CUDA
    // Query CUDA device count
    int cuda_device_count = ggml_backend_cuda_get_device_count();
    *device_count = cuda_device_count;

    // Fill device names if requested
    if (device_names && max_devices > 0) {
        int count = cuda_device_count < max_devices ? cuda_device_count : max_devices;
        for (int i = 0; i < count; i++) {
            // Get device name from CUDA
            char buffer[256];
            ggml_backend_cuda_get_device_description(i, buffer, sizeof(buffer));
            device_names[i] = strdup(buffer);
        }
    }

    return WAN_SUCCESS;
#else
    // No CUDA support
    *device_count = 0;
    return WAN_ERROR_UNSUPPORTED_OPERATION;
#endif
}

