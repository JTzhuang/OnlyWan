#ifndef __WAN_TYPES_H__
#define __WAN_TYPES_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum rng_type_t {
    STD_DEFAULT_RNG,
    CUDA_RNG,
    CPU_RNG,
    RNG_TYPE_COUNT
};

enum sample_method_t {
    EULER_SAMPLE_METHOD,
    EULER_A_SAMPLE_METHOD,
    HEUN_SAMPLE_METHOD,
    DPM2_SAMPLE_METHOD,
    DPMPP2S_A_SAMPLE_METHOD,
    DPMPP2M_SAMPLE_METHOD,
    DPMPP2Mv2_SAMPLE_METHOD,
    IPNDM_SAMPLE_METHOD,
    IPNDM_V_SAMPLE_METHOD,
    LCM_SAMPLE_METHOD,
    DDIM_TRAILING_SAMPLE_METHOD,
    TCD_SAMPLE_METHOD,
    RES_MULTISTEP_SAMPLE_METHOD,
    RES_2S_SAMPLE_METHOD,
    SAMPLE_METHOD_COUNT
};

enum scheduler_t {
    DISCRETE_SCHEDULER,
    KARRAS_SCHEDULER,
    EXPONENTIAL_SCHEDULER,
    AYS_SCHEDULER,
    GITS_SCHEDULER,
    SGM_UNIFORM_SCHEDULER,
    SIMPLE_SCHEDULER,
    SMOOTHSTEP_SCHEDULER,
    KL_OPTIMAL_SCHEDULER,
    LCM_SCHEDULER,
    BONG_TANGENT_SCHEDULER,
    SCHEDULER_COUNT
};

enum prediction_t {
    EPS_PRED,
    V_PRED,
    EDM_V_PRED,
    FLOW_PRED,
    FLUX_FLOW_PRED,
    FLUX2_FLOW_PRED,
    PREDICTION_COUNT
};

enum sd_type_t {
    SD_TYPE_F32  = 0,
    SD_TYPE_F16  = 1,
    SD_TYPE_Q4_0 = 2,
    SD_TYPE_Q4_1 = 3,
    SD_TYPE_Q5_0    = 6,
    SD_TYPE_Q5_1    = 7,
    SD_TYPE_Q8_0    = 8,
    SD_TYPE_Q8_1    = 9,
    SD_TYPE_Q2_K    = 10,
    SD_TYPE_Q3_K    = 11,
    SD_TYPE_Q4_K    = 12,
    SD_TYPE_Q5_K    = 13,
    SD_TYPE_Q6_K    = 14,
    SD_TYPE_Q8_K    = 15,
    SD_TYPE_IQ2_XXS = 16,
    SD_TYPE_IQ2_XS  = 17,
    SD_TYPE_IQ3_XXS = 18,
    SD_TYPE_IQ1_S   = 19,
    SD_TYPE_IQ4_NL  = 20,
    SD_TYPE_IQ3_S   = 21,
    SD_TYPE_IQ2_S   = 22,
    SD_TYPE_IQ4_XS  = 23,
    SD_TYPE_I8      = 24,
    SD_TYPE_I16     = 25,
    SD_TYPE_I32     = 26,
    SD_TYPE_I64     = 27,
    SD_TYPE_F64     = 28,
    SD_TYPE_IQ1_M   = 29,
    SD_TYPE_BF16    = 30,
    SD_TYPE_TQ1_0 = 34,
    SD_TYPE_TQ2_0 = 35,
    SD_TYPE_MXFP4 = 39,
    SD_TYPE_COUNT = 40,
};

enum sd_log_level_t {
    SD_LOG_DEBUG,
    SD_LOG_INFO,
    SD_LOG_WARN,
    SD_LOG_ERROR
};

enum preview_t {
    PREVIEW_NONE,
    PREVIEW_PROJ,
    PREVIEW_TAE,
    PREVIEW_VAE,
    PREVIEW_COUNT
};

enum lora_apply_mode_t {
    LORA_APPLY_AUTO,
    LORA_APPLY_IMMEDIATELY,
    LORA_APPLY_AT_RUNTIME,
    LORA_APPLY_MODE_COUNT,
};

typedef struct {
    bool enabled;
    int tile_size_x;
    int tile_size_y;
    float target_overlap;
    float rel_size_x;
    float rel_size_y;
} sd_tiling_params_t;

enum sd_cache_mode_t {
    SD_CACHE_DISABLED = 0,
    SD_CACHE_EASYCACHE,
    SD_CACHE_UCACHE,
    SD_CACHE_DBCACHE,
    SD_CACHE_TAYLORSEER,
    SD_CACHE_CACHE_DIT,
    SD_CACHE_SPECTRUM,
};

typedef struct {
    enum sd_cache_mode_t mode;
    float reuse_threshold;
    float start_percent;
    float end_percent;
    float error_decay_rate;
    bool use_relative_threshold;
    bool reset_error_on_compute;
    int Fn_compute_blocks;
    int Bn_compute_blocks;
    float residual_diff_threshold;
    int max_warmup_steps;
    int max_cached_steps;
    int max_continuous_cached_steps;
    int taylorseer_n_derivatives;
    int taylorseer_skip_interval;
    const char* scm_mask;
    bool scm_policy_dynamic;
    float spectrum_w;
    int spectrum_m;
    float spectrum_lam;
    int spectrum_window_size;
    float spectrum_flex_window;
    int spectrum_warmup_steps;
    float spectrum_stop_percent;
} sd_cache_params_t;

typedef struct {
    const char* name;
    const char* path;
} sd_embedding_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t channel;
    uint8_t* data;
} sd_image_t;

typedef struct {
    int* layers;
    size_t layer_count;
    float layer_start;
    float layer_end;
    float scale;
} sd_slg_params_t;

typedef struct {
    float txt_cfg;
    float img_cfg;
    float distilled_guidance;
    sd_slg_params_t slg;
} sd_guidance_params_t;

typedef struct {
    sd_guidance_params_t guidance;
    enum scheduler_t scheduler;
    enum sample_method_t sample_method;
    int sample_steps;
    float eta;
    int shifted_timestep;
    float* custom_sigmas;
    int custom_sigmas_count;
    float flow_shift;
} sd_sample_params_t;

typedef struct {
    const char* model_path;
    const char* clip_l_path;
    const char* clip_g_path;
    const char* clip_vision_path;
    const char* t5xxl_path;
    const char* llm_path;
    const char* llm_vision_path;
    const char* diffusion_model_path;
    const char* high_noise_diffusion_model_path;
    const char* vae_path;
    const char* taesd_path;
    const char* control_net_path;
    const sd_embedding_t* embeddings;
    uint32_t embedding_count;
    const char* photo_maker_path;
    const char* tensor_type_rules;
    bool vae_decode_only;
    bool free_params_immediately;
    int n_threads;
    enum sd_type_t wtype;
    enum rng_type_t rng_type;
    enum rng_type_t sampler_rng_type;
    enum prediction_t prediction;
    enum lora_apply_mode_t lora_apply_mode;
    bool offload_params_to_cpu;
    bool enable_mmap;
    bool keep_clip_on_cpu;
    bool keep_control_net_on_cpu;
    bool keep_vae_on_cpu;
    bool flash_attn;
    bool diffusion_flash_attn;
    bool tae_preview_only;
    bool diffusion_conv_direct;
    bool vae_conv_direct;
    bool circular_x;
    bool circular_y;
    bool force_sdxl_vae_conv_scale;
    bool chroma_use_dit_mask;
    bool chroma_use_t5_mask;
    int chroma_t5_mask_pad;
    bool qwen_image_zero_cond_t;
} sd_ctx_params_t;

typedef struct {
    const char* prompt;
    const char* negative_prompt;
    int clip_skip;
    sd_image_t init_image;
    sd_image_t end_image;
    sd_image_t* control_frames;
    int control_frames_size;
    int width;
    int height;
    sd_sample_params_t sample_params;
    sd_sample_params_t high_noise_sample_params;
    float moe_boundary;
    float strength;
    int64_t seed;
    int video_frames;
    float vace_strength;
    sd_tiling_params_t vae_tiling_params;
    sd_cache_params_t cache;
} sd_vid_gen_params_t;

typedef struct sd_ctx_t sd_ctx_t;

typedef void (*sd_log_cb_t)(const char* text, void* data);
typedef void (*sd_progress_cb_t)(int step, int steps, float time, void* data);
typedef void (*sd_preview_cb_t)(int step, int frame_count, sd_image_t* frames, bool is_noisy, void* data);

#ifdef __cplusplus
}
#endif

#endif  // __WAN_TYPES_H__
