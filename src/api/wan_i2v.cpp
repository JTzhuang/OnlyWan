/**
 * @file wan_i2v.cpp
 * @brief Image-to-video generation — legacy flat-arg entry point
 *
 * Delegates to wan_generate_video_i2v_ex in wan-api.cpp. The flat I2V API
 * has no width/height parameters; defaults are taken from ctx->params or
 * WAN2.2 I2V standard (832x480).
 */

#include "wan-internal.hpp"

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
    wan_params_t p = {};
    p.steps       = steps;
    p.cfg         = cfg;
    p.seed        = seed;
    p.width       = (ctx && ctx->params.width  > 0) ? ctx->params.width  : 832;
    p.height      = (ctx && ctx->params.height > 0) ? ctx->params.height : 480;
    p.num_frames  = num_frames;
    p.fps         = fps;
    p.progress_cb = progress_cb;
    p.user_data   = user_data;
    return wan_generate_video_i2v_ex(ctx, image, prompt, &p, output_path);
}

} // extern "C"
