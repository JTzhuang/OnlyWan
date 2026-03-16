/**
 * @file wan_t2v.cpp
 * @brief Text-to-video generation — legacy flat-arg entry point
 *
 * Delegates to wan_generate_video_t2v_ex in wan-api.cpp, which owns the
 * full runner headers to avoid ODR violations.
 */

#include "wan-internal.hpp"

extern "C" {

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
                                   void* user_data) {
    wan_params_t p = {};
    p.steps       = steps;
    p.cfg         = cfg;
    p.seed        = seed;
    p.width       = width;
    p.height      = height;
    p.num_frames  = num_frames;
    p.fps         = fps;
    p.progress_cb = progress_cb;
    p.user_data   = user_data;
    return wan_generate_video_t2v_ex(ctx, prompt, &p, output_path);
}

} // extern "C"
