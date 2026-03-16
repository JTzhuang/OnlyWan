/**
 * @file wan_config.cpp
 * @brief Configuration parameter implementation
 *
 * This file implements parameter validation and configuration management
 * for Wan public API.
 */

#include "wan.h"

#include <cstring>
#include <string>
#include <cctype>
#include <algorithm>

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
 * Validation Constants
 * ============================================================================ */

namespace WanConfig {

constexpr int MIN_STEPS = 1;
constexpr int MAX_STEPS = 100;
constexpr int DEFAULT_STEPS = 30;

constexpr float MIN_CFG = 1.0f;
constexpr float MAX_CFG = 20.0f;
constexpr float DEFAULT_CFG = 5.0f;

constexpr int MIN_WIDTH = 64;
constexpr int MAX_WIDTH = 4096;
constexpr int DEFAULT_WIDTH = 640;

constexpr int MIN_HEIGHT = 64;
constexpr int MAX_HEIGHT = 4096;
constexpr int DEFAULT_HEIGHT = 480;

constexpr int MIN_FRAMES = 1;
constexpr int MAX_FRAMES = 360;
constexpr int DEFAULT_FRAMES = 16;

constexpr int MIN_FPS = 1;
constexpr int MAX_FPS = 120;
constexpr int DEFAULT_FPS = 8;

constexpr int MIN_THREADS = 0;  // 0 = auto-detect
constexpr int MAX_THREADS = 256;

/* ============================================================================
 * Parameter Validation
 * ============================================================================ */

static bool validate_seed(int seed) {
    // Seed can be any integer (including -1 for random)
    return true;
}

static bool validate_steps(int steps) {
    return steps >= MIN_STEPS && steps <= MAX_STEPS;
}

static bool validate_cfg(float cfg) {
    return cfg >= MIN_CFG && cfg <= MAX_CFG;
}

static bool validate_width(int width) {
    if (width < MIN_WIDTH || width > MAX_WIDTH) {
        return false;
    }
    // Check divisibility by 32 (common in video models)
    return (width % 32) == 0;
}

static bool validate_height(int height) {
    if (height < MIN_HEIGHT || height > MAX_HEIGHT) {
        return false;
    }
    // Check divisibility by 32
    return (height % 32) == 0;
}

static bool validate_num_frames(int num_frames) {
    return num_frames >= MIN_FRAMES && num_frames <= MAX_FRAMES;
}

static bool validate_fps(int fps) {
    return fps >= MIN_FPS && fps <= MAX_FPS;
}

static bool validate_n_threads(int n_threads) {
    return n_threads >= MIN_THREADS && n_threads <= MAX_THREADS;
}

static bool validate_backend(const char* backend) {
    if (!backend) return true;  // nullptr means default

    std::string be(backend);
    std::transform(be.begin(), be.end(), be.begin(), [](unsigned char c){ return std::tolower(c); });

    return (be == "cpu" ||
            be == "cuda" ||
            be == "metal" ||
            be == "vulkan" ||
            be == "opencl" ||
            be == "hipblas" ||
            be == "musa");
}

} // namespace WanConfig

/* ============================================================================
 * C API Implementation
 * ============================================================================ */

extern "C" {

WAN_API wan_params_t* wan_params_create(void) {
    wan_params_t* params = new wan_params_t();
    if (!params) return nullptr;

    params->seed = -1;
    params->steps = WanConfig::DEFAULT_STEPS;
    params->cfg = WanConfig::DEFAULT_CFG;
    params->width = WanConfig::DEFAULT_WIDTH;
    params->height = WanConfig::DEFAULT_HEIGHT;
    params->num_frames = WanConfig::DEFAULT_FRAMES;
    params->fps = WanConfig::DEFAULT_FPS;
    params->negative_prompt = nullptr;
    params->n_threads = 0;
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
    if (params && WanConfig::validate_seed(seed)) {
        params->seed = seed;
    }
}

WAN_API void wan_params_set_steps(wan_params_t* params, int steps) {
    if (params && WanConfig::validate_steps(steps)) {
        params->steps = steps;
    }
}

WAN_API void wan_params_set_cfg(wan_params_t* params, float cfg) {
    if (params && WanConfig::validate_cfg(cfg)) {
        params->cfg = cfg;
    }
}

WAN_API void wan_params_set_size(wan_params_t* params, int width, int height) {
    if (params) {
        if (WanConfig::validate_width(width)) {
            params->width = width;
        }
        if (WanConfig::validate_height(height)) {
            params->height = height;
        }
    }
}

WAN_API void wan_params_set_num_frames(wan_params_t* params, int num_frames) {
    if (params && WanConfig::validate_num_frames(num_frames)) {
        params->num_frames = num_frames;
    }
}

WAN_API void wan_params_set_fps(wan_params_t* params, int fps) {
    if (params && WanConfig::validate_fps(fps)) {
        params->fps = fps;
    }
}

WAN_API void wan_params_set_negative_prompt(wan_params_t* params, const char* negative_prompt) {
    if (params) {
        params->negative_prompt = negative_prompt;
    }
}

WAN_API void wan_params_set_n_threads(wan_params_t* params, int n_threads) {
    if (params && WanConfig::validate_n_threads(n_threads)) {
        params->n_threads = n_threads;
    }
}

WAN_API void wan_params_set_backend(wan_params_t* params, const char* backend) {
    if (params && WanConfig::validate_backend(backend)) {
        params->backend = backend;
    }
}

WAN_API void wan_params_set_progress_callback(wan_params_t* params,
                                               wan_progress_cb_t callback,
                                               void* user_data) {
    if (params) {
        params->progress_cb = callback;
        params->user_data = user_data;
    }
}

} // extern "C"
