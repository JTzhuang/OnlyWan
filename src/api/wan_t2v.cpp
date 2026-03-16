/**
 * @file wan_t2v.cpp
 * @brief Text-to-video generation — legacy stub entry points
 *
 * wan_generate_video_t2v_ex (the real implementation with T5 encode +
 * WanRunner::compute) lives in wan-api.cpp, which is the single TU that
 * owns the full runner headers (wan.hpp, t5.hpp, clip.hpp) to avoid ODR
 * violations from their non-inline definitions.
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
    (void)ctx;
    (void)prompt;
    (void)output_path;
    (void)steps;
    (void)cfg;
    (void)seed;
    (void)width;
    (void)height;
    (void)num_frames;
    (void)fps;
    (void)progress_cb;
    (void)user_data;
    return WAN_ERROR_UNSUPPORTED_OPERATION;
}

} // extern "C"
