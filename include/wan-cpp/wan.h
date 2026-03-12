/**
 * @file wan.h
 * @brief C-style public API for wan-cpp video generation library
 *
 * This library provides a C API for Wan video generation models (WAN2.1, WAN2.2)
 * with support for text-to-video (T2V) and image-to-video (I2V) generation modes.
 *
 * @version 1.0.0
 * @license MIT
 */

#ifndef WAN_H
#define WAN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Error Codes
 * ============================================================================ */

/** Error codes returned by wan-c functions */
typedef enum {
    WAN_SUCCESS = 0,                    /**< Operation completed successfully */
    WAN_ERROR_INVALID_ARGUMENT,         /**< Invalid argument provided */
    WAN_ERROR_MODEL_LOAD_FAILED,       /**< Failed to load model file */
    WAN_ERROR_OUT_OF_MEMORY,           /**< Memory allocation failed */
    WAN_ERROR_GENERATION_FAILED,       /**< Video generation failed */
    WAN_ERROR_IMAGE_LOAD_FAILED,       /**< Failed to load input image */
    WAN_ERROR_UNSUPPORTED_OPERATION,    /**< Operation not supported */
    WAN_ERROR_BACKEND_FAILED,          /**< Backend operation failed */
    WAN_ERROR_INVALID_STATE,           /**< Invalid internal state */
    WAN_ERROR_IO,                      /**< File I/O error */
    WAN_ERROR_UNKNOWN                  /**< Unknown error */
} wan_error_t;

/* ============================================================================
 * Opaque Handle Types
 * ============================================================================ */

/** Opaque handle to a Wan context */
typedef struct wan_context wan_context_t;

/** Opaque handle to generation result */
typedef struct wan_result wan_result_t;

/* ============================================================================
 * Image Types
 * ============================================================================ */

/** Image data structure */
typedef struct {
    uint8_t* data;                     /**< Pointer to pixel data (RGB or RGBA) */
    int width;                         /**< Image width in pixels */
    int height;                        /**< Image height in pixels */
    int channels;                      /**< Number of color channels (3 for RGB, 4 for RGBA) */
} wan_image_t;

/* ============================================================================
 * Callback Types
 * ============================================================================ */

/** Progress callback type
 *
 * @param step Current step (0-based)
 * @param total_steps Total number of steps
 * @param progress Progress value (0.0 to 1.0)
 * @param user_data User-provided data pointer
 * @return Return non-zero to abort generation
 */
typedef int (*wan_progress_cb_t)(int step, int total_steps, float progress, void* user_data);

/** Generation parameters structure (not opaque - can be allocated and managed by user) */
typedef struct wan_params {
    int seed;                           /**< Random seed (-1 for random) */
    int steps;                          /**< Number of sampling steps */
    float cfg;                           /**< Classifier-free guidance scale */
    int width;                          /**< Output video width */
    int height;                          /**< Output video height */
    int num_frames;                      /**< Number of output frames */
    int fps;                             /**< Frames per second */
    int n_threads;                        /**< Number of threads (0 = auto-detect) */
    const char* negative_prompt;           /**< Negative prompt string */
    const char* backend;                  /**< Backend type string */
    wan_progress_cb_t progress_cb;       /**< Progress callback */
    void* user_data;                     /**< User data for callbacks */
    /* Private fields for internal use */
    void* _internal_ptr;
} wan_params_t;

/** Log callback type
 *
 * @param level Log level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)
 * @param message Log message
 * @param user_data User-provided data pointer
 */
typedef void (*wan_log_cb_t)(int level, const char* message, void* user_data);

/* ============================================================================
 * Model Loading
 * ============================================================================ */

/** Load a Wan model from a GGUF file
 *
 * @param model_path Path to the GGUF model file
 * @param n_threads Number of threads to use (0 = auto-detect)
 * @param backend_type Backend type string (e.g., "cpu", "cuda", "metal", "vulkan")
 * @param out_ctx Output pointer to receive the context handle
 * @return WAN_SUCCESS on success, error code on failure
 */
wan_error_t wan_load_model(const char* model_path,
                           int n_threads,
                           const char* backend_type,
                           wan_context_t** out_ctx);

/** Load a Wan model with custom backend parameters
 *
 * @param model_path Path to the GGUF model file
 * @param params Optional generation parameters (can be NULL)
 * @param out_ctx Output pointer to receive the context handle
 * @return WAN_SUCCESS on success, error code on failure
 */
wan_error_t wan_load_model_from_file(const char* model_path,
                                     const wan_params_t* params,
                                     wan_context_t** out_ctx);

/* ============================================================================
 * Text-to-Video Generation
 * ============================================================================ */

/** Generate video from text prompt
 *
 * @param ctx Wan context handle
 * @param prompt Text prompt for video generation
 * @param output_path Path to save the output video (AVI format)
 * @param steps Number of sampling steps (typically 20-50)
 * @param cfg Classifier-free guidance scale (typically 5-15)
 * @param seed Random seed for generation (-1 for random)
 * @param width Output video width (typically 480, 640, 720, 1024)
 * @param height Output video height (typically 360, 480, 576, 768)
 * @param num_frames Number of frames in output video
 * @param fps Frames per second for output
 * @param progress_cb Optional progress callback (NULL to disable)
 * @param user_data User data for progress callback
 * @return WAN_SUCCESS on success, error code on failure
 */
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
                                   void* user_data);

/** Generate video from text prompt with parameters
 *
 * @param ctx Wan context handle
 * @param prompt Text prompt for video generation
 * @param params Generation parameters
 * @param output_path Path to save the output video
 * @return WAN_SUCCESS on success, error code on failure
 */
wan_error_t wan_generate_video_t2v_ex(wan_context_t* ctx,
                                       const char* prompt,
                                       const wan_params_t* params,
                                       const char* output_path);

/* ============================================================================
 * Image-to-Video Generation
 * ============================================================================ */

/** Generate video from image
 *
 * @param ctx Wan context handle
 * @param image Input image
 * @param prompt Optional text prompt for conditioning (can be NULL)
 * @param output_path Path to save the output video
 * @param steps Number of sampling steps
 * @param cfg Classifier-free guidance scale
 * @param seed Random seed for generation
 * @param num_frames Number of frames in output video
 * @param fps Frames per second for output
 * @param progress_cb Optional progress callback (NULL to disable)
 * @param user_data User data for progress callback
 * @return WAN_SUCCESS on success, error code on failure
 */
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
                                  void* user_data);

/** Generate video from image with parameters
 *
 * @param ctx Wan context handle
 * @param image Input image
 * @param prompt Optional text prompt for conditioning (can be NULL)
 * @param params Generation parameters
 * @param output_path Path to save the output video
 * @return WAN_SUCCESS on success, error code on failure
 */
wan_error_t wan_generate_video_i2v_ex(wan_context_t* ctx,
                                      const wan_image_t* image,
                                      const char* prompt,
                                      const wan_params_t* params,
                                      const char* output_path);

/** Load an image from file for I2V generation
 *
 * @param image_path Path to the image file (PNG, JPEG, etc.)
 * @param out_image Output pointer to receive the image data
 * @return WAN_SUCCESS on success, error code on failure
 *
 * @note The caller is responsible for freeing the image data using wan_free_image()
 */
wan_error_t wan_load_image(const char* image_path, wan_image_t** out_image);

/** Free image data
 *
 * @param image Image to free
 */
void wan_free_image(wan_image_t* image);

/* ============================================================================
 * Parameter Configuration
 * ============================================================================ */

/** Create default generation parameters
 *
 * @return Pointer to new parameters structure, or NULL on failure
 */
wan_params_t* wan_params_create(void);

/** Free generation parameters
 *
 * @param params Parameters to free
 */
void wan_params_free(wan_params_t* params);

/** Set random seed
 *
 * @param params Parameters structure
 * @param seed Random seed (-1 for random)
 */
void wan_params_set_seed(wan_params_t* params, int seed);

/** Set number of sampling steps
 *
 * @param params Parameters structure
 * @param steps Number of steps (typically 20-50)
 */
void wan_params_set_steps(wan_params_t* params, int steps);

/** Set guidance scale (CFG)
 *
 * @param params Parameters structure
 * @param cfg Classifier-free guidance scale (typically 5-15)
 */
void wan_params_set_cfg(wan_params_t* params, float cfg);

/** Set output dimensions
 *
 * @param params Parameters structure
 * @param width Video width in pixels
 * @param height Video height in pixels
 */
void wan_params_set_size(wan_params_t* params, int width, int height);

/** Set number of output frames
 *
 * @param params Parameters structure
 * @param num_frames Number of frames to generate
 */
void wan_params_set_num_frames(wan_params_t* params, int num_frames);

/** Set output FPS
 *
 * @param params Parameters structure
 * @param fps Frames per second
 */
void wan_params_set_fps(wan_params_t* params, int fps);

/** Set negative prompt
 *
 * @param params Parameters structure
 * @param negative_prompt Negative prompt to guide generation away from
 */
void wan_params_set_negative_prompt(wan_params_t* params, const char* negative_prompt);

/** Set thread count
 *
 * @param params Parameters structure
 * @param n_threads Number of threads (0 = auto-detect)
 */
void wan_params_set_n_threads(wan_params_t* params, int n_threads);

/** Set backend type
 *
 * @param params Parameters structure
 * @param backend Backend type string ("cpu", "cuda", "metal", "vulkan", etc.)
 */
void wan_params_set_backend(wan_params_t* params, const char* backend);

/** Set progress callback
 *
 * @param params Parameters structure
 * @param callback Progress callback function
 * @param user_data User data for callback
 */
void wan_params_set_progress_callback(wan_params_t* params,
                                       wan_progress_cb_t callback,
                                       void* user_data);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/** Get version string
 *
 * @return Version string (e.g., "1.0.0")
 */
const char* wan_version(void);

/** Get last error message
 *
 * @param ctx Wan context handle
 * @return Error message string (valid until next operation)
 */
const char* wan_get_last_error(wan_context_t* ctx);

/** Set global log callback
 *
 * @param callback Log callback function
 * @param user_data User data for callback
 */
void wan_set_log_callback(wan_log_cb_t callback, void* user_data);

/** Get model information
 *
 * @param ctx Wan context handle
 * @param out_version Output buffer for model version string (NULL to skip)
 * @param version_size Size of version buffer
 * @return WAN_SUCCESS on success, error code on failure
 */
wan_error_t wan_get_model_info(wan_context_t* ctx,
                                char* out_version,
                                size_t version_size);

/* ============================================================================
 * Resource Cleanup
 * ============================================================================ */

/** Free Wan context and release resources
 *
 * @param ctx Wan context handle (set to NULL after call)
 */
void wan_free(wan_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* WAN_H */
