/**
 * @file wan-api.cpp
 * @brief C-style public API implementation for wan-cpp
 *
 * This file implements of C API declared in wan.h.
 */

#include "wan.h"
#include "wan-internal.hpp"

#include <cstdarg>
#include <cstring>
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
 * Internal Context Structure
 * ============================================================================ */

/**
 * @brief Internal context structure for Wan operations
 *
 * This structure holds all state needed for Wan model operations.
 * It's opaque to C API user.
 */
struct wan_context {
    std::string last_error;
    std::string model_path;
    WanModelPtr model;           // Internal Wan model wrapper
    WanBackendPtr backend;        // GGML backend
    WanVAEPtr vae;              // VAE for encoding/decoding
    WanParams params;            // Generation parameters
    int n_threads;
    std::string backend_type;
};

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
 * Parameter Management
 * ============================================================================ */

WAN_API wan_params_t* wan_params_create(void) {
    wan_params_t* params = new wan_params_t();
    if (!params) return nullptr;

    // Initialize with defaults
    params->seed = -1;
    params->steps = 30;
    params->cfg = 7.0f;
    params->width = 640;
    params->height = 480;
    params->num_frames = 16;
    params->fps = 16;
    params->n_threads = 0;
    params->negative_prompt = nullptr;
    params->backend = "cpu";
    params->progress_cb = nullptr;
    params->user_data = nullptr;
    params->_internal_ptr = nullptr;

    return params;
}

WAN_API void wan_params_free(wan_params_t* params) {
    if (params) {
        delete params;
    }
}

WAN_API void wan_params_set_seed(wan_params_t* params, int seed) {
    if (params) params->seed = seed;
}

WAN_API void wan_params_set_steps(wan_params_t* params, int steps) {
    if (params) params->steps = steps;
}

WAN_API void wan_params_set_cfg(wan_params_t* params, float cfg) {
    if (params) params->cfg = cfg;
}

WAN_API void wan_params_set_size(wan_params_t* params, int width, int height) {
    if (params) {
        params->width = width;
        params->height = height;
    }
}

WAN_API void wan_params_set_num_frames(wan_params_t* params, int num_frames) {
    if (params) params->num_frames = num_frames;
}

WAN_API void wan_params_set_fps(wan_params_t* params, int fps) {
    if (params) params->fps = fps;
}

WAN_API void wan_params_set_negative_prompt(wan_params_t* params, const char* negative_prompt) {
    if (params) params->negative_prompt = negative_prompt;
}

WAN_API void wan_params_set_n_threads(wan_params_t* params, int n_threads) {
    if (params) params->n_threads = n_threads;
}

WAN_API void wan_params_set_backend(wan_params_t* params, const char* backend) {
    if (params) params->backend = backend;
}

WAN_API void wan_params_set_progress_callback(wan_params_t* params,
                                                   wan_progress_cb_t callback,
                                                   void* user_data) {
    if (params) {
        params->progress_cb = callback;
        params->user_data = user_data;
    }
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

    // Load model
    WanModelLoadResult result = Wan::WanModel::load(model_path);
    if (!result.success) {
        set_last_error(ctx.get(), result.error_message.c_str());
        return WAN_ERROR_MODEL_LOAD_FAILED;
    }

    ctx->model = result.model;
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
    if (!ctx || !ctx->model) {
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
    // TODO: Implement actual image loading
    (void)image_path;
    (void)out_image;
    return WAN_ERROR_UNSUPPORTED_OPERATION;
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
 * Video Generation (Stubs - to be implemented)
 * ============================================================================ */

WAN_API wan_error_t wan_generate_video_t2v(wan_context_t* ctx,
                                               const char* prompt,
                                               const char* output_path,
                                               int steps,
                                               float cfg,
                                               int seed,
                                               int width,
                                               int height,
                                               int num_frames,
                                               int fps,
                                               wan_progress_cb_t progress_cb,
                                               void* user_data) {
    (void)ctx;
    (void)prompt;
    (void)output_path;
    (void)steps;
    (void)cfg;
    (void)seed;
    (void)width;
    (void)height;
    (void)num_frames;
    (void)fps;
    (void)progress_cb;
    (void)user_data;
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

WAN_API wan_error_t wan_generate_video_t2v_ex(wan_context_t* ctx,
                                                   const char* prompt,
                                                   const wan_params_t* params,
                                                   const char* output_path) {
    (void)ctx;
    (void)prompt;
    (void)params;
    (void)output_path;
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

WAN_API wan_error_t wan_generate_video_i2v(wan_context_t* ctx,
                                                  const wan_image_t* image,
                                                  const char* prompt,
                                                  const char* output_path,
                                                  int steps,
                                                  float cfg,
                                                  int seed,
                                                  int num_frames,
                                                  int fps,
                                                  wan_progress_cb_t progress_cb,
                                                  void* user_data) {
    (void)ctx;
    (void)image;
    (void)prompt;
    (void)output_path;
    (void)steps;
    (void)cfg;
    (void)seed;
    (void)num_frames;
    (void)fps;
    (void)progress_cb;
    (void)user_data;
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

WAN_API wan_error_t wan_generate_video_i2v_ex(wan_context_t* ctx,
                                                      const wan_image_t* image,
                                                      const char* prompt,
                                                      const wan_params_t* params,
                                                      const char* output_path) {
    (void)ctx;
    (void)image;
    (void)prompt;
    (void)params;
    (void)output_path;
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}
