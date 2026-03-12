/**
 * @file wan_config.cpp
 * @brief Configuration parameter implementation
 *
 * This file implements parameter validation and configuration management
 * for the Wan public API.
 */

#include "wan.h"
#include "wan-internal.hpp"

#include <sstream>
#include <algorithm>

/* ============================================================================
 * Validation Constants
 * ============================================================================ */

namespace WanConfig {

// Valid value ranges
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

/**
 * @brief Validate seed value
 */
static bool validate_seed(int seed) {
    // Seed can be any integer (including -1 for random)
    return true;
}

/**
 * @brief Validate steps value
 */
static bool validate_steps(int steps) {
    return steps >= MIN_STEPS && steps <= MAX_STEPS;
}

/**
 * @brief Validate CFG scale value
 */
static bool validate_cfg(float cfg) {
    return cfg >= MIN_CFG && cfg <= MAX_CFG;
}

/**
 * @brief Validate width value
 *
 * For Wan models, width should be divisible by 32 or 64
 */
static bool validate_width(int width) {
    if (width < MIN_WIDTH || width > MAX_WIDTH) {
        return false;
    }
    // Check divisibility by 32 (common in video models)
    return (width % 32) == 0;
}

/**
 * @brief Validate height value
 */
static bool validate_height(int height) {
    if (height < MIN_HEIGHT || height > MAX_HEIGHT) {
        return false;
    }
    // Check divisibility by 32
    return (height % 32) == 0;
}

/**
 * @brief Validate number of frames
 */
static bool validate_num_frames(int num_frames) {
    return num_frames >= MIN_FRAMES && num_frames <= MAX_FRAMES;
}

/**
 * @brief Validate FPS value
 */
static bool validate_fps(int fps) {
    return fps >= MIN_FPS && fps <= MAX_FPS;
}

/**
 * @brief Validate thread count
 */
static bool validate_n_threads(int n_threads) {
    return n_threads >= MIN_THREADS && n_threads <= MAX_THREADS;
}

/**
 * @brief Validate backend type
 */
static bool validate_backend(const std::string& backend) {
    std::string lower_backend = backend;
    std::transform(lower_backend.begin(), lower_backend.end(),
                  lower_backend.begin(), ::tolower);

    // Valid backend types
    return (lower_backend == "cpu" ||
            lower_backend == "cuda" ||
            lower_backend == "metal" ||
            lower_backend == "vulkan" ||
            lower_backend == "opencl" ||
            lower_backend == "hipblas" ||
            lower_backend == "musa");
}

/**
 * @brief Clamp value to range
 */
template<typename T>
static T clamp_value(T value, T min_val, T max_val, T default_val) {
    if (value < min_val || value > max_val) {
        return default_val;
    }
    return value;
}

/* ============================================================================
 * Parameter Info
 * ============================================================================ */

struct ParameterInfo {
    std::string name;
    std::string description;
    std::string valid_range;
};

static std::vector<ParameterInfo> get_parameter_infos() {
    return {
        {"seed", "Random seed for generation", "-1 or any positive integer"},
        {"steps", "Number of denoising steps", std::to_string(MIN_STEPS) + "-" + std::to_string(MAX_STEPS)},
        {"cfg", "Classifier-free guidance scale", std::to_string(MIN_CFG) + "-" + std::to_string(MAX_CFG)},
        {"width", "Output video width", std::to_string(MIN_WIDTH) + "-" + std::to_string(MAX_WIDTH) + " (divisible by 32)"},
        {"height", "Output video height", std::to_string(MIN_HEIGHT) + "-" + std::to_string(MAX_HEIGHT) + " (divisible by 32)"},
        {"num_frames", "Number of output frames", std::to_string(MIN_FRAMES) + "-" + std::to_string(MAX_FRAMES)},
        {"fps", "Frames per second", std::to_string(MIN_FPS) + "-" + std::to_string(MAX_FPS)},
        {"n_threads", "Number of computation threads", "0 (auto) or " + std::to_string(MAX_THREADS)},
        {"backend", "GGML backend type", "cpu, cuda, metal, vulkan, opencl, hipblas, musa"},
        {"negative_prompt", "Negative prompt for conditioning", "Any text string"},
    };
}

} // namespace WanConfig

/* ============================================================================
 * C API Implementation (Configuration)
 * ============================================================================ */

wan_params_t* wan_params_create(void) {
    try {
        auto params = new wan_params_t();
        params->seed = -1;
        params->steps = WanConfig::DEFAULT_STEPS;
        params->cfg = WanConfig::DEFAULT_CFG;
        params->width = WanConfig::DEFAULT_WIDTH;
        params->height = WanConfig::DEFAULT_HEIGHT;
        params->num_frames = WanConfig::DEFAULT_FRAMES;
        params->fps = WanConfig::DEFAULT_FPS;
        params->negative_prompt = "";
        params->n_threads = 0;
        params->backend = "cpu";
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
    if (params && WanConfig::validate_seed(seed)) {
        params->seed = seed;
    }
}

void wan_params_set_steps(wan_params_t* params, int steps) {
    if (params) {
        if (!WanConfig::validate_steps(steps)) {
            // Clamp to valid range
            steps = WanConfig::clamp_value(steps, WanConfig::MIN_STEPS, WanConfig::MAX_STEPS, WanConfig::DEFAULT_STEPS);
            if (g_log_callback) {
                std::string msg = "Steps value out of range, clamping to " + std::to_string(steps);
                g_log_callback(2, msg.c_str(), g_log_user_data);
            }
        }
        params->steps = steps;
    }
}

void wan_params_set_cfg(wan_params_t* params, float cfg) {
    if (params) {
        if (!WanConfig::validate_cfg(cfg)) {
            // Clamp to valid range
            cfg = WanConfig::clamp_value(cfg, WanConfig::MIN_CFG, WanConfig::MAX_CFG, WanConfig::DEFAULT_CFG);
            if (g_log_callback) {
                std::ostringstream oss;
                oss << "CFG value out of range, clamping to " << cfg;
                g_log_callback(2, oss.str().c_str(), g_log_user_data);
            }
        }
        params->cfg = cfg;
    }
}

void wan_params_set_size(wan_params_t* params, int width, int height) {
    if (params) {
        if (!WanConfig::validate_width(width)) {
            width = WanConfig::clamp_value(width, WanConfig::MIN_WIDTH, WanConfig::MAX_WIDTH, WanConfig::DEFAULT_WIDTH);
            // Adjust to be divisible by 32
            width = ((width + 31) / 32) * 32;
            if (g_log_callback) {
                std::string msg = "Width value invalid, adjusting to " + std::to_string(width);
                g_log_callback(2, msg.c_str(), g_log_user_data);
            }
        }
        if (!WanConfig::validate_height(height)) {
            height = WanConfig::clamp_value(height, WanConfig::MIN_HEIGHT, WanConfig::MAX_HEIGHT, WanConfig::DEFAULT_HEIGHT);
            height = ((height + 31) / 32) * 32;
            if (g_log_callback) {
                std::string msg = "Height value invalid, adjusting to " + std::to_string(height);
                g_log_callback(2, msg.c_str(), g_log_user_data);
            }
        }
        params->width = width;
        params->height = height;
    }
}

void wan_params_set_num_frames(wan_params_t* params, int num_frames) {
    if (params) {
        if (!WanConfig::validate_num_frames(num_frames)) {
            num_frames = WanConfig::clamp_value(num_frames, WanConfig::MIN_FRAMES, WanConfig::MAX_FRAMES, WanConfig::DEFAULT_FRAMES);
            if (g_log_callback) {
                std::string msg = "Num frames value out of range, clamping to " + std::to_string(num_frames);
                g_log_callback(2, msg.c_str(), g_log_user_data);
            }
        }
        params->num_frames = num_frames;
    }
}

void wan_params_set_fps(wan_params_t* params, int fps) {
    if (params) {
        if (!WanConfig::validate_fps(fps)) {
            fps = WanConfig::clamp_value(fps, WanConfig::MIN_FPS, WanConfig::MAX_FPS, WanConfig::DEFAULT_FPS);
            if (g_log_callback) {
                std::string msg = "FPS value out of range, clamping to " + std::to_string(fps);
                g_log_callback(2, msg.c_str(), g_log_user_data);
            }
        }
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
        if (!WanConfig::validate_n_threads(n_threads)) {
            n_threads = WanConfig::clamp_value(n_threads, WanConfig::MIN_THREADS, WanConfig::MAX_THREADS, 0);
            if (g_log_callback) {
                std::string msg = "Thread count value out of range, clamping to " + std::to_string(n_threads);
                g_log_callback(2, msg.c_str(), g_log_user_data);
            }
        }
        params->n_threads = n_threads;
    }
}

void wan_params_set_backend(wan_params_t* params, const char* backend) {
    if (params) {
        std::string backend_str = backend ? backend : "cpu";
        if (!WanConfig::validate_backend(backend_str)) {
            backend_str = "cpu";
            if (g_log_callback && backend && backend[0] != '\0') {
                std::string msg = "Invalid backend type, defaulting to CPU";
                g_log_callback(2, msg.c_str(), g_log_user_data);
            }
        }
        params->backend = backend_str;
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
 * Additional Configuration Utilities (Internal)
 * ============================================================================ */

namespace WanConfig {

/**
 * @brief Validate all parameters
 *
 * @param params Parameters to validate
 * @return true if all parameters are valid
 */
bool validate_all(const WanParams& params) {
    if (!validate_seed(params.seed)) return false;
    if (!validate_steps(params.steps)) return false;
    if (!validate_cfg(params.cfg)) return false;
    if (!validate_width(params.width)) return false;
    if (!validate_height(params.height)) return false;
    if (!validate_num_frames(params.num_frames)) return false;
    if (!validate_fps(params.fps)) return false;
    if (!validate_n_threads(params.n_threads)) return false;
    if (!validate_backend(params.backend)) return false;
    return true;
}

/**
 * @brief Get parameter description string
 */
std::string get_parameter_description() {
    std::ostringstream oss;
    oss << "Wan Generation Parameters:\n";
    oss << "===========================\n";

    auto infos = get_parameter_infos();
    for (const auto& info : infos) {
        oss << "\n" << info.name << ":\n";
        oss << "  Description: " << info.description << "\n";
        oss << "  Valid range: " << info.valid_range << "\n";
    }

    return oss.str();
}

/**
 * @brief Apply default values to invalid parameters
 */
void apply_defaults(WanParams& params) {
    if (!validate_seed(params.seed)) params.seed = -1;
    if (!validate_steps(params.steps)) params.steps = DEFAULT_STEPS;
    if (!validate_cfg(params.cfg)) params.cfg = DEFAULT_CFG;
    if (!validate_width(params.width)) params.width = DEFAULT_WIDTH;
    if (!validate_height(params.height)) params.height = DEFAULT_HEIGHT;
    if (!validate_num_frames(params.num_frames)) params.num_frames = DEFAULT_FRAMES;
    if (!validate_fps(params.fps)) params.fps = DEFAULT_FPS;
    if (!validate_n_threads(params.n_threads)) params.n_threads = 0;
    if (!validate_backend(params.backend)) params.backend = "cpu";
}

} // namespace WanConfig
