/**
 * @ @file wan_loader.cpp
 * @brief Wan model loading implementation
 */

#include "wan-internal.hpp"

#include <fstream>
#include <cstring>

#include "gguf.h"
#include "ggml-backend.h"
#include "ggml.h"

namespace Wan {

/* ============================================================================
 * GGUF Reading Utilities
 * ============================================================================ */

/**
 * @brief Check if a GGUF file is a valid Wan model
 *
 * Reads the metadata from the GGUF file and checks for Wan-specific keys.
 */
static bool is_wan_gguf(const std::string& file_path, std::string& model_type, std::string& model_version) {
    gguf_context* ctx = gguf_init_from_file(file_path.c_str());
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
 * Tensor Loading
 * ============================================================================ */

/**
 * @brief Load a tensor from GGUF file
 *
 * @param ctx GGUF context
 * @param tensor_name Name of tensor to load
 * @param tensor_storage Tensor storage metadata
 * @param buffer_context GGML buffer context for allocation
 * @param backend GGML backend for buffer allocation
 * @return Loaded tensor or nullptr on failure
 */
static struct ggml_tensor* load_tensor_from_gguf(gguf_context* ctx,
                                                   const std::string& tensor_name,
                                                   const WanModelLoadResult& result) {
    // Find tensor index in GGUF file
    int tensor_idx = gguf_find_tensor(ctx, tensor_name.c_str());
    if (tensor_idx < 0) {
        return nullptr;
    }

    // Get tensor info
    const gguf_tensor_info* info = gguf_get_tensor_info(ctx, tensor_idx);
    if (!info) {
        return nullptr;
    }

    // Create tensor in GGML context
    // Note: This is a simplified implementation
    // Full implementation would handle quantization, memory mapping, etc.
    struct ggml_tensor* tensor = ggml_new_tensor(nullptr,
                                                 (ggml_type)info->type,
                                                 info->n_dims,
                                                 info->ne);
    if (!tensor) {
        return nullptr;
    }

    return tensor;
}

/* ============================================================================
 * WanModel Implementation
 * ============================================================================ */

WanModelLoadResult WanModel::load(const std::string& file_path,
                                     Wan::WanBackend* backend) {
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

    // Open GGUF file
    gguf_context* gguf_ctx = gguf_init_from_file(file_path.c_str());
    if (!gguf_ctx) {
        result.error_message = "Failed to open GGUF file: " + file_path;
        return result;
    }

    // Create model instance
    auto model = std::make_shared<WanModel>();
    model->model_path = file_path;
    model->model_type = model_type;
    model->model_version = model_version;

    // Load tensors based on model type
    // This is a simplified implementation - full version would handle
    // quantized tensors, memory mapping, backend allocation, etc.

    int n_tensors = gguf_get_n_tensors(gguf_ctx);
    for (int i = 0; i < n_tensors; i++) {
        const char* tensor_name = gguf_get_tensor_name(gguf_ctx, i);

        // Categorize tensor by name prefix
        std::string name(tensor_name);

        // Wan diffusion model tensors
        if (name.find("diffusion_model.") == 0 || name.find("model.") == 0) {
            // Store diffusion tensor mapping
            // model->diffusion_tensors[name] = load_tensor_from_gguf(gguf_ctx, tensor_name, result);
        }
        // VAE tensors
        else if (name.find("first_stage_model.") == 0 || name.find("vae.") == 0) {
            // Store VAE tensor mapping
            // model->vae_tensors[name] = load_tensor_from_gguf(gguf_ctx, tensor_name, result);
        }
        // Text encoder tensors (if separate)
        else if (name.find("cond_stage_model.") == 0 || name.find("text_encoder.") == 0) {
            // Store text encoder tensor mapping
            // model->text_encoder_tensors[name] = load_tensor_from_gguf(gguf_ctx, tensor_name, result);
        }
    }

    gguf_free(gguf_ctx);

    result.success = true;
    result.model = model;
    result.model_version = model_version;

    return result;
}

bool WanModel::is_valid() const {
    return !model_path.empty() && !model_type.empty();
}

std::string WanModel::get_info() const {
    std::string info = "Wan Model:\n";
    info += "  Type: " + model_type + "\n";
    info += "  Version: " + model_version + "\n";
    info += "  Path: " + model_path + "\n";
    return info;
}

/* ============================================================================
 * WanVAE Implementation
 * ============================================================================ */

std::vector<uint8_t> WanVAE::decode(struct ggml_tensor* latent) {
    // Implementation pending - requires VAE decode logic from vae.hpp
    return std::vector<uint8_t>();
}

struct ggml_tensor* WanVAE::encode(const std::vector<uint8_t>& image,
                                    int width, int height) {
    // Implementation pending - requires VAE encode logic from vae.hpp
    return nullptr;
}

/* ============================================================================
 * WanBackend Implementation
 * ============================================================================ */

WanBackend* WanBackend::create(const std::string& type, int n_threads) {
    auto backend = std::make_unique<WanBackend>();
    backend->backend_type = type;
    backend->n_threads = n_threads > 0 ? n_threads : 4;
    backend->backend = nullptr;
    backend->buffer = nullptr;
    backend->ctx = nullptr;

    // Create GGML backend based on type
    if (type == "cpu" || type.empty()) {
        backend->backend = ggml_backend_cpu_init();
        if (!backend->backend) {
            return nullptr;
        }
    }
    else if (type == "cuda" || type == "gpu") {
#ifdef GGML_USE_CUDA
        backend->backend = ggml_backend_cuda_init();
        if (!backend->backend) {
            return nullptr;
        }
#else
        return nullptr;  // CUDA not compiled in
#endif
    }
    else if (type == "metal") {
#ifdef GGML_USE_METAL
        backend->backend = ggml_backend_metal_init();
        if (!backend->backend) {
            return nullptr;
        }
#else
        return nullptr;  // Metal not compiled in
#endif
    }
    else if (type == "vulkan") {
#ifdef GGML_USE_VULKAN
        backend->backend = ggml_backend_vulkan_init();
        if (!backend->backend) {
            return nullptr;
        }
#else
        return nullptr;  // Vulkan not compiled in
#endif
    }
    else {
        return nullptr;  // Unknown backend
    }

    return backend.release();
}

bool WanBackend::allocate_buffer(size_t size) {
    if (!backend) {
        return false;
    }

    buffer = ggml_backend_alloc_buffer(backend, size);
    return buffer != nullptr;
}

WanBackend::~WanBackend() {
    if (buffer) {
        ggml_backend_buffer_free(buffer);
    }
    if (backend) {
        ggml_backend_free(backend);
    }
    if (ctx) {
        ggml_free(ctx);
    }
}

} // namespace Wan

/* ============================================================================
 * C API Helper Functions
 * ============================================================================ */

/* These functions will be called from wan-api.cpp to perform
 * the actual model loading and tensor operations.
 */

WanModelLoadResult wan_model_load(const std::string& file_path,
                                    Wan::WanBackend* backend) {
    return Wan::WanModel::load(file_path, backend);
}
