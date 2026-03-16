/**
 * @file wan_loader.cpp
 * @brief Wan model loading implementation
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
 * @brief Check if a GGUF file is a valid Wan model
 *
 * Reads metadata from GGUF file and checks for Wan-specific keys.
 */
static bool is_wan_gguf(const std::string& file_path, std::string& model_type, std::string& model_version) {
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
            if (arch.find("I2V") != std::string::npos || arch.find("i2v") != std::string::npos) {
                model_type = "i2v";
            } else if (arch.find("TI2V") != std::string::npos || arch.find("ti2v") != std::string::npos) {
                model_type = "ti2v";
            } else if (arch.find("T2V") != std::string::npos || arch.find("t2v") != std::string::npos) {
                model_type = "t2v";
            } else {
                model_type = "t2v";  // Default to T2V
            }
        }
    }

    // Default version if not found
    if (is_wan && model_version.empty()) {
        if (model_type == "i2v") {
            model_version = "WAN2.2";  // I2V is typically WAN2.2
        } else {
            model_version = "WAN2.1";  // Default
        }
    }

    gguf_free(ctx);
    return is_wan;
}

/* ============================================================================
 * WanBackend Implementation
 * ============================================================================ */

WanBackend* WanBackend::create(const std::string& type, int n_threads) {
    std::unique_ptr<WanBackend> backend(new WanBackend());
    backend->backend_type = type;
    backend->n_threads = n_threads;

    // Initialize the appropriate GGML backend
    if (type == "cpu" || type == "" || type == "default") {
        backend->backend = ggml_backend_cpu_init();
    }
#ifdef WAN_USE_CUDA
    else if (type == "cuda") {
        backend->backend = ggml_backend_cuda_init(0);
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

    // Allocate backend buffer
    size_t buffer_size = 256 * 1024 * 1024;  // 256 MB
    backend->buffer = ggml_backend_alloc_buffer(backend->backend, buffer_size);

    return backend.release();
}

} // namespace Wan
