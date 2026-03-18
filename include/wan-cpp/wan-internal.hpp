/**
 * @file wan-internal.hpp
 * @brief Internal headers for Wan C API implementation
 *
 * This file contains internal structures and declarations used by
 * C API implementation. It is not part of public API.
 */

#ifndef WAN_INTERNAL_HPP
#define WAN_INTERNAL_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "wan.h"

#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "gguf.h"

#ifdef WAN_USE_MULTI_GPU
#include "ggml-backend-sched.h"
#endif

// Forward-declare runner types so wan-internal.hpp can be included by multiple
// translation units without pulling in the heavy header-only implementations
// (wan.hpp, t5.hpp, clip.hpp) that contain non-inline function definitions.
// Translation units that construct these types must include the full headers.
namespace WAN {
struct WanRunner;
struct WanVAERunner;
} // namespace WAN

struct T5Embedder;
struct CLIPVisionModelProjectionRunner;

/* ============================================================================
 * Internal Parameters Structure
 * ============================================================================ */

struct WanParams {
    int seed = -1;
    int steps = 30;
    float cfg = 5.0f;
    int width = 640;
    int height = 480;
    int num_frames = 16;
    int fps = 8;
    std::string negative_prompt;
    std::string sampler = "euler";
    std::string scheduler = "exponential";
    float flow_shift = 0.0f;
    bool use_slg = false;
    std::vector<int> skip_layers;
    float slg_scale = 0.0f;
    float slg_layer_start = 0.5f;
    float slg_layer_end = 1.0f;
    int n_threads = 0;
    std::string backend = "cpu";
    std::string model_version;  // "WAN2.1", "WAN2.2", etc.
    wan_progress_cb_t progress_cb = nullptr;
    void* user_data = nullptr;

    // Multi-GPU fields
    std::vector<int> gpu_ids;
    int num_gpus = 0;
    wan_distribution_strategy_t distribution_strategy = WAN_DISTRIBUTION_AUTO;
};

/* ============================================================================
 * Model Loading Result
 * ============================================================================ */

struct WanModelLoadResult {
    bool success = false;
    std::shared_ptr<WAN::WanRunner>                  wan_runner;
    std::shared_ptr<WAN::WanVAERunner>               vae_runner;
    std::shared_ptr<T5Embedder>                      t5_embedder;
    std::shared_ptr<CLIPVisionModelProjectionRunner> clip_runner;
    std::string model_type;     // "t2v", "i2v", "ti2v"
    std::string error_message;
    std::string model_version;  // "WAN2.1", "WAN2.2", etc.
};

// WanModel::load — implemented in wan-api.cpp (needs full runner headers)
// is_wan_gguf — implemented in wan_loader.cpp
namespace Wan {
bool is_wan_gguf(const std::string& file_path, std::string& model_type, std::string& model_version);

struct WanModel {
    static WanModelLoadResult load(const std::string& file_path);
};
} // namespace Wan

/* ============================================================================
 * Internal Backend
 * ============================================================================ */

namespace Wan {

struct WanBackend {
    ggml_backend_t backend;
    ggml_backend_buffer_t buffer;
    ggml_context* ctx;
    int n_threads;
    std::string backend_type;

    WanBackend() : backend(nullptr), buffer(nullptr), ctx(nullptr),
                   n_threads(0), backend_type("cpu") {}

    ~WanBackend() {
        if (buffer) {
            ggml_backend_buffer_free(buffer);
            buffer = nullptr;
        }
        if (backend) {
            ggml_backend_free(backend);
            backend = nullptr;
        }
    }

    static WanBackend* create(const std::string& type, int n_threads, int device_id = 0);
#ifdef WAN_USE_MULTI_GPU
    static WanBackend* create_on_device(const std::string& type, int n_threads, int device_id);
#endif
};

} // namespace Wan

using WanBackendPtr = std::unique_ptr<Wan::WanBackend>;

#ifdef WAN_USE_MULTI_GPU
// Forward declaration for multi-GPU state creation
MultiGPUState* create_multi_gpu_state(const std::vector<int>& gpu_ids, wan_distribution_strategy_t strategy);
#endif

/* ============================================================================
 * Multi-GPU State
 * ============================================================================ */

#ifdef WAN_USE_MULTI_GPU
struct MultiGPUState {
    std::vector<ggml_backend_t> backends;
    ggml_backend_sched_t scheduler;
    std::vector<int> gpu_ids;
    wan_distribution_strategy_t strategy;
    bool initialized;

    MultiGPUState() : scheduler(nullptr), strategy(WAN_DISTRIBUTION_AUTO), initialized(false) {}

    ~MultiGPUState() {
        if (scheduler) {
            ggml_backend_sched_free(scheduler);
            scheduler = nullptr;
        }
        for (auto backend : backends) {
            if (backend) {
                ggml_backend_free(backend);
            }
        }
        backends.clear();
    }
};
#endif

/* ============================================================================
 * Internal Context Structure
 * ============================================================================ */

struct wan_context {
    std::string last_error;
    std::string model_path;
    std::string model_type;    // "t2v", "i2v", "ti2v"
    std::shared_ptr<WAN::WanRunner>                  wan_runner;
    std::shared_ptr<WAN::WanVAERunner>               vae_runner;
    std::shared_ptr<T5Embedder>                      t5_embedder;
    std::shared_ptr<CLIPVisionModelProjectionRunner> clip_runner;  // null for T2V
    WanBackendPtr backend;
    WanParams params;
    int n_threads = 0;
    std::string backend_type;

#ifdef WAN_USE_MULTI_GPU
    std::unique_ptr<MultiGPUState> multi_gpu_state;
#endif
};

/* ============================================================================
 * Image Loading Utilities
 * ============================================================================ */

namespace WanImage {

wan_image_t* load_from_file(const std::string& file_path);
wan_image_t* resize(const wan_image_t* image, int new_width, int new_height);
wan_image_t* to_rgb(const wan_image_t* image);

} // namespace WanImage

/* ============================================================================
 * Video Generation Utilities
 * ============================================================================ */

namespace WanVideo {

bool save_as_avi(const std::vector<std::vector<uint8_t>>& frames,
                  int width, int height, int fps,
                  const std::string& output_path);

} // namespace WanVideo

#endif /* WAN_INTERNAL_HPP */
