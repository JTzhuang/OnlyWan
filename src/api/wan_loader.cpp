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
        // Try to infer version from model size or architecture
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
 * WanModel Implementation
 * ============================================================================ */

WanModelLoadResult WanModel::load(const std::string& file_path) {
    WanModelLoadResult result;
    result.success = false;

    // Check if file exists
    std::ifstream test_file(file_path);
    if (!test_file.good()) {
        result.error_message = "Model file not found: " + file_path;
        return result;
    }
    test_file.close();

    // Validate GGUF file
    std::string model_type, model_version;
    if (!is_wan_gguf(file_path, model_type, model_version)) {
        result.error_message = "Not a valid Wan GGUF model: " + file_path;
        return result;
    }

    // Initialize GGML context
    ggml_init_params ctx_params = {
        /* .mem_size = */   0,  // Auto-detect
        /* .mem_buffer = */ nullptr,
        /* .no_alloc = */   true,  // We'll allocate manually
    };

    std::unique_ptr<WanModel> model(new WanModel());
    model->model_path = file_path;
    model->model_type = model_type;
    model->model_version = model_version;

    // Initialize GGUF context
    gguf_init_params gguf_params = {
        /* .no_alloc = */ 0,
        /* .ctx = */      nullptr,
    };

    model->gguf_ctx = gguf_init_from_file(file_path.c_str(), gguf_params);
    if (!model->gguf_ctx) {
        result.error_message = "Failed to initialize GGUF context";
        return result;
    }

    // Allocate parameter context (will be populated during load)
    // For now, we just mark as successful
    // Full tensor loading would be done with ModelLoader
    result.success = true;
    result.model = WanModelPtr(model.release());
    result.model_version = model_version;

    return result;
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
        // SYCL backend initialization (implementation depends on SYCL version)
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
    // Start with a reasonable size, will grow if needed
    size_t buffer_size = 256 * 1024 * 1024;  // 256 MB
    backend->buffer = ggml_backend_alloc_buffer(backend->backend, buffer_size);

    return backend.release();
}

} // namespace Wan
