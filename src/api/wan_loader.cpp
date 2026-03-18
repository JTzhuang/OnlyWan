/**
 * @file wan_loader.cpp
 * @brief Wan backend creation and GGUF validation utilities
 *
 * WanModel::load() is implemented in wan-api.cpp which already includes
 * the full runner headers (wan.hpp, t5.hpp, clip.hpp). Keeping those
 * includes out of this TU avoids ODR violations from non-inline definitions
 * in the header-only runner files.
 */

#include "wan-internal.hpp"

#include <fstream>
#include <cstring>

#include "gguf.h"
#include "ggml-backend.h"
#include "ggml.h"
#include "ggml-cpu.h"

#ifdef WAN_USE_CUDA
#include "ggml-cuda.h"
#endif

namespace Wan {

/* ============================================================================
 * GGUF Reading Utilities
 * ============================================================================ */

/**
 * @brief Check if a GGUF file is a valid Wan model and read metadata.
 *
 * Sets model_type ("t2v", "i2v", "ti2v") and model_version ("WAN2.1", etc.).
 */
bool is_wan_gguf(const std::string& file_path, std::string& model_type, std::string& model_version) {
    gguf_init_params params = {
        /* .no_alloc = */ 0,
        /* .ctx = */      nullptr,
    };

    gguf_context* ctx = gguf_init_from_file(file_path.c_str(), params);
    if (!ctx) {
        return false;
    }

    bool is_wan = false;
    model_type.clear();
    model_version.clear();

    // Check for general stable-diffusion key
    int key_idx = gguf_find_key(ctx, "general.model_type");
    if (key_idx >= 0) {
        const char* type_str = gguf_get_val_str(ctx, key_idx);
        if (type_str && (strcmp(type_str, "wan") == 0 || strcmp(type_str, "WAN") == 0)) {
            is_wan = true;
        }
    }

    // Check for specific Wan version
    key_idx = gguf_find_key(ctx, "general.wan.version");
    if (key_idx >= 0) {
        const char* version_str = gguf_get_val_str(ctx, key_idx);
        if (version_str) {
            model_version = version_str;
        }
    }

    // Determine model type from architecture name
    key_idx = gguf_find_key(ctx, "general.architecture");
    if (key_idx >= 0) {
        const char* arch_str = gguf_get_val_str(ctx, key_idx);
        if (arch_str) {
            std::string arch = arch_str;
            if (arch.find("TI2V") != std::string::npos || arch.find("ti2v") != std::string::npos) {
                model_type = "ti2v";
            } else if (arch.find("I2V") != std::string::npos || arch.find("i2v") != std::string::npos) {
                model_type = "i2v";
            } else if (arch.find("T2V") != std::string::npos || arch.find("t2v") != std::string::npos) {
                model_type = "t2v";
            } else {
                model_type = "t2v";  // Default to T2V
            }
        }
    }

    // Default version if not found
    if (is_wan && model_version.empty()) {
        model_version = (model_type == "i2v") ? "WAN2.2" : "WAN2.1";
    }

    gguf_free(ctx);
    return is_wan;
}

/* ============================================================================
 * WanBackend Implementation
 * ============================================================================ */

WanBackend* WanBackend::create(const std::string& type, int n_threads, int device_id) {
    std::unique_ptr<WanBackend> backend(new WanBackend());
    backend->backend_type = type;
    backend->n_threads = n_threads;

    if (type == "cpu" || type == "" || type == "default") {
        backend->backend = ggml_backend_cpu_init();
    }
#ifdef WAN_USE_CUDA
    else if (type == "cuda") {
        backend->backend = ggml_backend_cuda_init(device_id);
    }
#endif
#ifdef WAN_USE_METAL
    else if (type == "metal") {
        backend->backend = ggml_backend_metal_init();
    }
#endif
#ifdef WAN_USE_VULKAN
    else if (type == "vulkan") {
        backend->backend = ggml_backend_vulkan_init();
    }
#endif
#ifdef WAN_USE_OPENCL
    else if (type == "opencl") {
        backend->backend = ggml_backend_opencl_init();
    }
#endif
#ifdef WAN_USE_SYCL
    else if (type == "sycl") {
        backend->backend = ggml_backend_cpu_init();  // Fallback
    }
#endif
#ifdef WAN_USE_HIPBLAS
    else if (type == "hipblas" || type == "rocm") {
        backend->backend = ggml_backend_hip_init(0);
    }
#endif
#ifdef WAN_USE_MUSA
    else if (type == "musa") {
        backend->backend = ggml_backend_musa_init(0);
    }
#endif
    else {
        return nullptr;  // Unsupported backend
    }

    if (!backend->backend) {
        return nullptr;
    }

    size_t buffer_size = 256 * 1024 * 1024;  // 256 MB
    backend->buffer = ggml_backend_alloc_buffer(backend->backend, buffer_size);

    return backend.release();
}

#ifdef WAN_USE_MULTI_GPU
/**
 * @brief Create a backend targeting a specific CUDA device.
 *
 * Helper for data parallel mode where each GPU needs its own backend instance.
 * Falls back to device 0 for non-CUDA backends.
 */
WanBackend* WanBackend::create_on_device(const std::string& type, int n_threads, int device_id) {
    return create(type, n_threads, device_id);
}

/**
 * @brief Create multi-GPU state with backend initialization and device validation.
 *
 * Validates GPU homogeneity (same device description) and creates CUDA backends
 * for each GPU. Returns nullptr if validation fails or backends cannot be created.
 */
MultiGPUState* create_multi_gpu_state(const std::vector<int>& gpu_ids, wan_distribution_strategy_t strategy) {
    if (gpu_ids.empty()) {
        LOG_ERROR("create_multi_gpu_state: empty gpu_ids");
        return nullptr;
    }

#ifdef WAN_USE_CUDA
    int device_count = ggml_backend_cuda_get_device_count();

    // Validate all GPU IDs are within range
    for (int gpu_id : gpu_ids) {
        if (gpu_id < 0 || gpu_id >= device_count) {
            LOG_ERROR("create_multi_gpu_state: GPU ID %d out of range (0-%d)", gpu_id, device_count - 1);
            return nullptr;
        }
    }

    // Validate GPU homogeneity - all GPUs must have same device description
    std::string first_device_desc;
    for (size_t i = 0; i < gpu_ids.size(); i++) {
        ggml_backend_t temp_backend = ggml_backend_cuda_init(gpu_ids[i]);
        if (!temp_backend) {
            LOG_ERROR("create_multi_gpu_state: failed to init CUDA backend for GPU %d", gpu_ids[i]);
            return nullptr;
        }

        std::string device_desc = ggml_backend_name(temp_backend);
        ggml_backend_free(temp_backend);

        if (i == 0) {
            first_device_desc = device_desc;
        } else if (device_desc != first_device_desc) {
            LOG_ERROR("create_multi_gpu_state: GPU homogeneity check failed - GPU %d (%s) != GPU %d (%s)",
                      gpu_ids[0], first_device_desc.c_str(), gpu_ids[i], device_desc.c_str());
            return nullptr;
        }
    }

    // Create MultiGPUState and initialize backends
    std::unique_ptr<MultiGPUState> state(new MultiGPUState());
    state->gpu_ids = gpu_ids;
    state->strategy = strategy;

    for (int gpu_id : gpu_ids) {
        ggml_backend_t backend = ggml_backend_cuda_init(gpu_id);
        if (!backend) {
            LOG_ERROR("create_multi_gpu_state: failed to create backend for GPU %d", gpu_id);
            return nullptr;
        }
        state->backends.push_back(backend);
    }

    state->initialized = true;
    LOG_INFO("Multi-GPU state created: %zu GPUs, strategy=%d", gpu_ids.size(), strategy);

    return state.release();
#else
    LOG_ERROR("create_multi_gpu_state: WAN_USE_CUDA not defined");
    return nullptr;
#endif
}
#endif

} // namespace Wan
