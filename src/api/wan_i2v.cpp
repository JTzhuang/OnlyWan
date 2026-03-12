/**
 * @file wan_i2v.cpp
 * @brief Image-to-video generation implementation
 */

#include "wan-internal.hpp"

#include <algorithm>
#include <cstring>
#include <random>

#include "ggml-backend.h"
#include "ggml.h"

namespace WanI2V {

/* ============================================================================
 * Constants
 * ============================================================================ */

constexpr int DEFAULT_STEPS = 30;
constexpr float DEFAULT_CFG = 5.0f;
constexpr int DEFAULT_SEED = -1;  // -1 means random

/* ============================================================================
 * Image Preprocessing
 * ============================================================================ */

/**
 * @brief Validate and preprocess input image for I2V generation
 *
 * @param image Input image
 * @param target_width Target width (may be resized)
 * @param target_height Target height (may be resized)
 * @return Preprocessed image or nullptr on failure
 */
static wan_image_t* preprocess_input_image(const wan_image_t* image,
                                           int target_width,
                                           int target_height) {
    if (!image || !image->data) {
        return nullptr;
    }

    // Validate image dimensions
    if (image->width <= 0 || image->height <= 0) {
        return nullptr;
    }

    // Validate channels (expect RGB or RGBA)
    if (image->channels != 3 && image->channels != 4) {
        return nullptr;
    }

    // Convert RGBA to RGB if needed
    wan_image_t* rgb_image = nullptr;
    if (image->channels == 4) {
        rgb_image = WanImage::to_rgb(image);
        if (!rgb_image) {
            return nullptr;
        }
    }

    // Use the appropriate source image
    const wan_image_t* source = (image->channels == 4) ? rgb_image : image;

    // Resize to target dimensions if needed
    wan_image_t* resized = nullptr;
    if (source->width != target_width || source->height != target_height) {
        resized = WanImage::resize(source, target_width, target_height);
        if (!resized) {
            if (rgb_image) {
                WanImage::to_rgb(rgb_image);  // Free
            }
            return nullptr;
        }
    } else {
        // Create a copy of the image
        resized = new wan_image_t();
        resized->width = source->width;
        resized->height = source->height;
        resized->channels = source->channels;
        size_t size = source->width * source->height * source->channels;
        resized->data = new uint8_t[size];
        memcpy(resized->data, source->data, size);
    }

    // Cleanup temporary image
    if (rgb_image) {
        WanImage::to_rgb(rgb_image);  // Free
    }

    return resized;
}

/* ============================================================================
 * Image Encoding
 * ============================================================================ */

/**
 * @brief Encode input image to latent space for I2V conditioning
 *
 * @param image Preprocessed RGB image
 * @param vae VAE for encoding
 * @return Latent tensor or nullptr on failure
 */
static struct ggml_tensor* encode_image_to_latent(const wan_image_t* image,
                                                   Wan::WanVAE* vae) {
    if (!image || !image->data || !vae) {
        return nullptr;
    }

    // Convert image data to vector for VAE encoding
    size_t size = image->width * image->height * image->channels;
    std::vector<uint8_t> image_data(image->data, image->data + size);

    // Encode to latent space
    struct ggml_tensor* latent = vae->encode(image_data, image->width, image->height);

    return latent;
}

/* ============================================================================
 * I2V Denoising Loop
 * ============================================================================ */

/**
 * @brief Generate noise tensor for I2V sampling
 *
 * @param n_tokens Number of latent tokens
 * @param hidden_dim Hidden dimension
 * @param seed Random seed
 * @return Noise tensor or nullptr on failure
 */
static struct ggml_tensor* generate_i2v_noise(int64_t n_tokens, int64_t hidden_dim, int seed) {
    std::mt19937 gen(seed == -1 ? std::random_device{}() : seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);

    struct ggml_context* ctx = ggml_init({1024 * 1024, nullptr, false});
    if (!ctx) {
        return nullptr;
    }

    int64_t ne[2] = {n_tokens, hidden_dim};
    struct ggml_tensor* noise = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, ne[0], ne[1]);
    if (!noise) {
        ggml_free(ctx);
        return nullptr;
    }

    float* data = (float*)ggml_get_data(noise);
    for (int64_t i = 0; i < n_tokens * hidden_dim; i++) {
        data[i] = dist(gen);
    }

    return noise;
}

/**
 * @brief I2V conditioning structure
 */
struct I2VConditioning {
    struct ggml_tensor* image_latent;    // Encoded input image
    struct ggml_tensor* text_embedding;    // Optional text embedding
    struct ggml_tensor* combined_condition; // Combined conditioning for model
};

/**
 * @brief Create I2V conditioning from image and optional text
 */
static I2VConditioning create_i2v_conditioning(const wan_image_t* image,
                                                  const std::string& text_prompt,
                                                  Wan::WanModel* model,
                                                  Wan::WanBackend* backend) {
    I2VConditioning result = {nullptr, nullptr, nullptr};

    // Encode image to latent
    if (model && model->vae) {
        result.image_latent = encode_image_to_latent(image, model->vae.get());
    }

    // Encode text if provided
    if (!text_prompt.empty() && model) {
        // TODO: Implement text encoding
        // result.text_embedding = WanT2V::encode_text_prompt(text_prompt, model, backend);
    }

    // TODO: Combine image and text conditioning
    // This is model-specific and depends on Wan architecture

    return result;
}

/**
 * @brief I2V denoising loop with image conditioning
 */
static struct ggml_tensor* denoise_i2v_loop(struct ggml_tensor* latent,
                                              const I2VConditioning& conditioning,
                                              const WanParams& params,
                                              Wan::WanModel* model,
                                              Wan::WanBackend* backend) {
    // TODO: Implement I2V denoising loop
    // This is similar to T2V but includes image conditioning:
    // 1. At each step, feed image latent into model
    // 2. Model predicts noise conditioned on image + text
    // 3. Apply sampling step

    // Simplified loop structure:
    /*
    for (int step = 0; step < params.steps; step++) {
        // 1. Get timestep
        float t = timesteps[step];

        // 2. Model forward with image conditioning
        struct ggml_tensor* noise_pred = model->forward_i2v(
            latent, t, conditioning, params);

        // 3. Apply CFG (if using)
        if (params.cfg > 1.0f) {
            noise_pred = apply_cfg_i2v(noise_pred, conditioning, params);
        }

        // 4. Sampler step
        latent = sampler_step(latent, noise_pred, t, t_next);
    }
    */

    return latent;
}

/* ============================================================================
 * I2V Video Generation
 * ============================================================================ */

/**
 * @brief Generate video from input image
 */
static std::vector<std::vector<uint8_t>> generate_i2v_video(const wan_image_t* input_image,
                                                             const std::string& text_prompt,
                                                             const WanParams& params,
                                                             Wan::WanModel* model,
                                                             Wan::WanBackend* backend) {
    std::vector<std::vector<uint8_t>> frames;

    // 1. Preprocess input image
    wan_image_t* preprocessed = preprocess_input_image(input_image, params.width, params.height);
    if (!preprocessed) {
        return frames;
    }

    // 2. Create I2V conditioning
    I2VConditioning conditioning = create_i2v_conditioning(
        preprocessed, text_prompt, model, backend);

    if (!conditioning.image_latent) {
        WanImage::to_rgb(preprocessed);  // Free
        return frames;
    }

    // 3. Generate initial noise
    int64_t n_tokens = (params.width / 4) * (params.height / 4) * params.num_frames;
    struct ggml_tensor* latent = generate_i2v_noise(
        n_tokens, model->params.dim, params.seed);

    if (!latent) {
        WanImage::to_rgb(preprocessed);  // Free
        return frames;
    }

    // 4. Denoise loop
    struct ggml_tensor* denoised = denoise_i2v_loop(
        latent, conditioning, params, model, backend);

    // 5. Decode to frames
    if (denoised && model->vae) {
        frames = WanT2V::decode_video(denoised, model->vae.get(), params);
    }

    // Cleanup
    WanImage::to_rgb(preprocessed);  // Free

    return frames;
}

} // namespace WanI2V

/* ============================================================================
 * C API Implementation (I2V)
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

    // Create parameters structure
    WanParams params;
    params.seed = seed;
    params.steps = (steps > 0) ? steps : WanI2V::DEFAULT_STEPS;
    params.cfg = (cfg > 0.0f) ? cfg : WanI2V::DEFAULT_CFG;
    params.width = image->width;  // Use input image dimensions
    params.height = image->height;
    params.num_frames = (num_frames > 0) ? num_frames : 16;
    params.fps = (fps > 0) ? fps : 8;
    // Note: progress callback handled via params or context

    return wan_generate_video_i2v_ex(ctx, image, prompt, &params, output_path);
}

wan_error_t wan_generate_video_i2v_ex(wan_context_t* ctx,
                                      const wan_image_t* image,
                                      const char* prompt,
                                      const wan_params_t* params_ptr,
                                      const char* output_path) {
    if (!ctx || !image || !output_path) {
        return WAN_ERROR_INVALID_ARGUMENT;
    }

    try {
        // Convert C params to internal params
        WanParams params;
        if (params_ptr) {
            params.seed = params_ptr->seed;
            params.steps = params_ptr->steps;
            params.cfg = params_ptr->cfg;
            params.width = params_ptr->width;
            params.height = params_ptr->height;
            params.num_frames = params_ptr->num_frames;
            params.fps = params_ptr->fps;
            params.negative_prompt = params_ptr->negative_prompt;
            params.n_threads = params_ptr->n_threads;
        } else {
            // Use input image dimensions by default
            params.seed = -1;
            params.steps = 30;
            params.cfg = 5.0f;
            params.width = image->width;
            params.height = image->height;
            params.num_frames = 16;
            params.fps = 8;
        }

        // Validate model is loaded
        if (!ctx->model || !ctx->model->is_valid()) {
            set_last_error(ctx, "No valid model loaded");
            return WAN_ERROR_INVALID_STATE;
        }

        // Validate input image
        if (!image || !image->data || image->width <= 0 || image->height <= 0) {
            set_last_error(ctx, "Invalid input image");
            return WAN_ERROR_IMAGE_LOAD_FAILED;
        }

        // Check if model supports I2V
        if (ctx->model->model_type != "i2v" && ctx->model->model_type != "ti2v") {
            set_last_error(ctx, "Model does not support I2V generation. Model type: " + ctx->model->model_type);
            return WAN_ERROR_UNSUPPORTED_OPERATION;
        }

        // TODO: Implement full I2V generation pipeline:
        // 1. Preprocess input image
        // 2. Create image conditioning
        // 3. Generate initial noise
        // 4. Run denoising loop with image conditioning
        // 5. Decode latents to frames
        // 6. Save as AVI video

        set_last_error(ctx, "I2V generation not fully implemented yet");
        return WAN_ERROR_UNSUPPORTED_OPERATION;

        /*
        std::string text_prompt = prompt ? prompt : "";

        auto frames = WanI2V::generate_i2v_video(
            image, text_prompt, params,
            ctx->model.get(), ctx->backend.get());

        if (frames.empty()) {
            set_last_error(ctx, "Failed to generate video frames");
            return WAN_ERROR_GENERATION_FAILED;
        }

        if (!WanVideo::save_as_avi(frames, params.width, params.height, params.fps, output_path)) {
            set_last_error(ctx, "Failed to save output video");
            return WAN_ERROR_IO;
        }

        return WAN_SUCCESS;
        */
    } catch (const std::exception& e) {
        set_last_error(ctx, e.what());
        return WAN_ERROR_UNKNOWN;
    }
}
