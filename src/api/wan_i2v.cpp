/**
 * @file wan_i2v.cpp
 * @brief Image-to-video generation implementation
 *
 * Note: Full I2V implementation requires integration with
 * existing WAN::WanRunner, ModelLoader, and image encoder classes.
 * For now, this provides stub implementations that can be compiled.
 */

#include "wan.h"

extern "C" {

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
    (void)ctx;
    (void)image;
    (void)prompt;
    (void)output_path;
    (void)steps;
    (void)cfg;
    (void)seed;
    (void)num_frames;
    (void)fps;
    (void)progress_cb;
    (void)user_data;
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

wan_error_t wan_generate_video_i2v_ex(wan_context_t* ctx,
                                                      const wan_image_t* image,
                                                      const char* prompt,
                                                      const wan_params_t* params,
                                                      const char* output_path) {
    (void)ctx;
    (void)image;
    (void)prompt;
    (void)params;
    (void)output_path;
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

} // extern "C"
