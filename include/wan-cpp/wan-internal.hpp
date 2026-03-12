/**
 * @file wan-internal.hpp
 * @brief Internal headers for Wan C API implementation
 *
 * This file contains internal structures and declarations used by the
 * C API implementation. It is not part of the public API.
 */

#ifndef WAN_INTERNAL_HPP
#define WAN_INTERNAL_HPP

#include <memory>
#include <string>
#include <vector>

#include "wan.h"

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

namespace Wan {

struct WanModel;
struct WanVAE;
struct WanBackend;

} // namespace Wan

/* ============================================================================
 * Smart Pointer Types
 * ============================================================================ */

using WanModelPtr = std::shared_ptr<Wan::WanModel>;
using WanVAEPtr = std::shared_ptr<Wan::WanVAE>;
using WanBackendPtr = std::unique_ptr<Wan::WanBackend>;

/* ============================================================================
 * Model Loading Result
 * ============================================================================ */

struct WanModelLoadResult {
    bool success;
    WanModelPtr model;
    WanVAEPtr vae;
    std::string error_message;
    std::string model_version;  // "WAN2.1", "WAN2.2", etc.
};

/* ============================================================================
 * Generation Parameters (Internal)
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
};

/* ============================================================================
 * Internal Model Loader
 * ============================================================================ */

namespace Wan {

/**
 * @brief Internal Wan model structure
 *
 * Contains the loaded Wan diffusion model with all its components.
 */
struct WanModel {
    WanParams params;
    std::string model_path;
    std::string model_type;  // "t2v", "i2v", etc.
    std::string model_version;  // "WAN2.1", "WAN2.2", etc.

    // Tensor storage maps for model weights
    std::map<std::string, struct ggml_tensor*> diffusion_tensors;
    std::map<std::string, struct ggml_tensor*> vae_tensors;
    std::map<std::string, struct ggml_tensor*> text_encoder_tensors;

    /**
     * @brief Load a Wan model from a GGUF file
     *
     * @param file_path Path to the GGUF model file
     * @param backend GGML backend for computation
     * @return Loading result with model or error information
     */
    static WanModelLoadResult load(const std::string& file_path,
                                  Wan::WanBackend* backend);

    /**
     * @brief Check if model is a valid Wan model
     */
    bool is_valid() const;

    /**
     * @brief Get model information
     */
    std::string get_info() const() const;
};

/**
 * @brief Internal VAE structure
 *
 * Contains the VAE encoder/decoder for latent space operations.
 */
struct WanVAE {
    std::map<std::string, struct ggml_tensor*> encoder_tensors;
    std::map<std::string, struct ggml_tensor*> decoder_tensors;

    /**
     * @brief Decode latent tensors to RGB images
     */
    std::vector<uint8_t> decode(struct ggml_tensor* latent);

    /**
     * @brief Encode RGB images to latent tensors
     */
    struct ggml_tensor* encode(const std::vector<uint8_t>& image,
                              int width, int height);
};

/**
 * @brief Internal backend structure
 *
 * Manages GGML backend initialization and resource allocation.
 */
struct WanBackend {
    ggml_backend_t backend;
    ggml_backend_buffer_t buffer;
    ggml_context* ctx;
    int n_threads;
    std::string backend_type;

    /**
     * @brief Create a new GGML backend
     *
     * @param type Backend type ("cpu", "cuda", "metal", "vulkan", etc.)
     * @param n_threads Number of threads (for CPU backend)
     * @return Backend instance or nullptr on failure
     */
    static WanBackend* create(const std::string& type, int n_threads);

    /**
     * @brief Allocate tensor buffer
     */
    bool allocate_buffer(size_t size);

    /**
     * @brief Free backend resources
     */
    ~WanBackend();
};

} // namespace Wan

/* ============================================================================
 * Image Loading Utilities
 * ============================================================================ */

namespace WanImage {

/**
 * @brief Load an image from file
 *
 * @param file_path Path to image file (PNG, JPEG, etc.)
 * @return Image data or nullptr on failure
 */
wan_image_t* load_from_file(const std::string& file_path);

/**
 * @brief Resize an image
 *
 * @param image Source image
 * @param new_width Target width
 * @param new_height Target height
 * @return Resized image (caller must free)
 */
wan_image_t* resize(const wan_image_t* image, int new_width, int new_height);

/**
 * @brief Convert image to RGB if it's RGBA
 *
 * @param image Source image
 * @return RGB image (caller must free)
 */
wan_image_t* to_rgb(const wan_image_t* image);

} // namespace WanImage

/* ============================================================================
 * Video Generation Utilities
 * ============================================================================ */

namespace WanVideo {

/**
 * @brief Save frames as AVI video file
 *
 * @param frames Vector of image data
 * @param width Frame width
 * @param height Frame height
 * @param fps Frames per second
 * @param output_path Output file path
 * @return true on success
 */
bool save_as_avi(const std::vector<std::vector<uint8_t>>& frames,
                  int width, int height, int fps,
                  const std::string& output_path);

} // namespace WanVideo

#endif /* WAN_INTERNAL_HPP */
