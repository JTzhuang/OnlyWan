/**
 * @file wan-api.cpp
 * @brief C-style public API implementation for wan-cpp
 *
 * This file implements the C API declared in wan.h.
 */

#include "wan.h"
#include "wan-internal.hpp"

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
 * This structure holds all the state needed for Wan model operations.
 * It's opaque to the C API user.
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

/**
 * @brief Internal parameters structure for Wan generation
 */
struct wan_params {
    int seed;
    int steps;
    float cfg;
    int width;
    int height;
    int num_frames;
    int fps;
    std::string negative_prompt;
    int n_threads;
    std::string backend;
    wan_progress_cb_t progress_cb;
    void* user_data;
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

    ctx->last_error = buffer;
}

/* ============================================================================
 * Global State
 * ============================================================================ */

static wan_log_cb_t g_log_callback = nullptr;
static void* g_log_user_data = nullptr;

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char* wan_version(void) {
    return "1.0.0";
}

const char* wan_get_last_error(wan_context_t* ctx) {
    if (ctx) {
        return ctx->last_error.c_str();
    }
    return "Invalid context";
}

void wan_set_log_callback(wan_log_cb_t callback, void* user_data) {
    g_log_callback = callback;
    g_log_user_data = user_data;
}

wan_error_t wan_get_model_info(wan_context_t* ctx,
                                char* out_version,
                                size_t version_size) {
    if (!ctx) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    // For now, we don't have version info from the model
    // This would be populated during model loading
    if (out_version && version_size > 0) {
        const char* version = "unknown";
        strncpy(out_version, version, version_size - 1);
        out_version[version_size - 1] = '\0';
    }

    return WAN_SUCCESS;
}

/* ============================================================================
 * Parameter Configuration
 * ============================================================================ */

wan_params_t* wan_params_create(void) {
    try {
        wan_params_t* params = new wan_params_t();
        params->seed = -1;           // Random seed
        params->steps = 30;         // Default sampling steps
        params->cfg = 5.0f;        // Default guidance scale
        params->width = 640;       // Default width
        params->height = 480;       // Default height
        params->num_frames = 16;    // Default number of frames
        params->fps = 8;            // Default FPS
        params->n_threads = 0;       // Auto-detect
        params->progress_cb = nullptr;
        params->user_data = nullptr;
        return params;
    } catch (const std::exception& e) {
        if (g_log_callback) {
            g_log_callback(3, e.what(), g_log_user_data);
        }
        return nullptr;
    }
}

void wan_params_free(wan_params_t* params) {
    if (params) {
        delete params;
    }
}

void wan_params_set_seed(wan_params_t* params, int seed) {
    if (params) {
        params->seed = seed;
    }
}

void wan_params_set_steps(wan_params_t* params, int steps) {
    if (params) {
        params->steps = steps;
    }
}

void wan_params_set_cfg(wan_params_t* params, float cfg) {
    if (params) {
        params->cfg = cfg;
    }
}

void wan_params_set_size(wan_params_t* params, int width, int height) {
    if (params) {
        params->width = width;
        params->height = height;
    }
}

void wan_params_set_num_frames(wan_params_t* params, int num_frames) {
    if (params) {
        params->num_frames = num_frames;
    }
}

void wan_params_set_fps(wan_params_t* params, int fps) {
    if (params) {
        params->fps = fps;
    }
}

void wan_params_set_negative_prompt(wan_params_t* params, const char* negative_prompt) {
    if (params) {
        params->negative_prompt = negative_prompt ? negative_prompt : "";
    }
}

void wan_params_set_n_threads(wan_params_t* params, int n_threads) {
    if (params) {
        params->n_threads = n_threads;
    }
}

void wan_params_set_backend(wan_params_t* params, const char* backend) {
    if (params) {
        params->backend = backend ? backend : "cpu";
    }
}

void wan_params_set_progress_callback(wan_params_t* params,
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

wan_error_t wan_load_model(const char* model_path,
                           int n_threads,
                           const char* backend_type,
                           wan_context_t** out_ctx) {
    if (!model_path || !out_ctx) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    try {
        // Create context
        auto ctx = std::make_unique<wan_context>();
        ctx->model_path = model_path;
        ctx->n_threads = n_threads > 0 ? n_threads : 4;
        ctx->backend_type = backend_type ? backend_type : "cpu";

        // Initialize backend
        ctx->backend = WanBackend::create(ctx->backend_type, ctx->n_threads);
        if (!ctx->backend) {
            set_last_error(ctx.get(), "Failed to initialize GGML backend");
            return WAN_ERROR_BACKEND_FAILED;
        }

        // Load model (will be implemented in wan_loader.cpp)
        auto result = WanModel::load(model_path, ctx->backend.get());
        if (!result.success) {
            set_last_error(ctx.get(), result.error_message.c_str());
            return WAN_ERROR_MODEL_LOAD_FAILED;
        }

        ctx->model = std::move(result.model);
        ctx->vae = std::move(result.vae);

        // Transfer ownership to caller
        *out_ctx = ctx.release();

        if (g_log_callback) {
            g_log_callback(1, "WAN model loaded successfully", g_log_user_data);
        }

        return WAN_SUCCESS;
    } catch (const std::exception& e) {
        if (out_ctx && *out_ctx) {
            set_last_error(*out_ctx, e.what());
        } else {
            // Can't set error on non-existent context
            if (g_log_callback) {
                g_log_callback(3, e.what(), g_log_user_data);
            }
        }
        return WAN_ERROR_UNKNOWN;
    }
}

wan_error_t wan_load_model_from_file(const char* model_path,
                                     const wan_params_t* params,
                                     wan_context_t** out_ctx) {
    if (!model_path || !out_ctx) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    int n_threads = params ? params->n_threads : 0;
    const char* backend_type = params ? params->backend.c_str() : "cpu";

    return wan_load_model(model_path, n_threads, backend_type, out_ctx);
}

/* ============================================================================
 * Resource Cleanup
 * ============================================================================ */

void wan_free(wan_context_t* ctx) {
    if (ctx) {
        delete ctx;
    }
}

/* ============================================================================
 * Image Operations
 * ============================================================================ */

wan_error_t wan_load_image(wan_image_t** out_image) {
    if (!out_image) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }
    // Implementation pending in wan_image.cpp
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

void wan_free_image(wan_image_t* image) {
    if (image) {
        if (image->data) {
            delete[] image->data;
        }
        delete image;
    }
}

/* ============================================================================
 * Text-to-Video Generation
 * ============================================================================ */

wan_error_t wan_generate_video_t2v(wan_context_t* ctx,
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
    if (!ctx || !prompt || !output_path) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    // Implementation pending in wan_t2v.cpp
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

wan_error_t wan_generate_video_t2v_ex(wan_context_t* ctx,
                                       const char* prompt,
                                       const wan_params_t* params,
                                       const char* output_path) {
    if (!ctx || !prompt || !output_path) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    // Implementation pending in wan_t2v.cpp
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

/* ============================================================================
 * Image-to-Video Generation
 * ============================================================================ */

wan_error_t wan_generate_video_i2v(wan_context_t* ctx,
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
    if (!ctx || !image || !output_path) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    // Implementation pending in wan_i2v.cpp
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

wan_error_t wan_generate_video_i2v_ex(wan_context_t* ctx,
                                      const wan_image_t* image,
                                      const char* prompt,
                                      const wan_params_t* params,
                                      const char* output_path) {
    if (!ctx || !image || !output_path) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    // Implementation pending in wan_i2v.cpp
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}
