/**
 * @file main.cpp
 * @brief CLI example for wan-cpp video generation
 *
 * This is a full-featured CLI demonstrating the wan-cpp API
 * with support for T2V (text-to-video) and I2V (image-to-video) generation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wan.h"

/* ============================================================================
 * Command Line Options Structure
 * ============================================================================ */

typedef struct {
    /* Required options */
    char* model_path;              // Path to GGUF model file
    char* prompt;                  // Text prompt for generation

    /* Optional options */
    char* input_image;             // Input image for I2V mode
    char* output_path;             // Output video path
    char* backend;                 // Backend type (cpu, cuda, metal, vulkan)
    char* negative_prompt;         // Negative prompt

    /* Parameters */
    int threads;                   // Number of threads (0 = auto)
    int width;                    // Output width
    int height;                   // Output height
    int num_frames;               // Number of frames
    int fps;                      // Frames per second
    int steps;                    // Sampling steps
    int seed;                     // Random seed
    float cfg;                    // Classifier-free guidance scale

    /* Flags */
    int verbose;                   // Verbose output
    int show_help;                // Show help message
    int show_version;             // Show version info

    /* Status */
    int mode;                     // 0 = none, 1 = T2V, 2 = I2V
} cli_options_t;

/* ============================================================================
 * Default Values
 * ============================================================================ */

#define DEFAULT_WIDTH      640
#define DEFAULT_HEIGHT     360
#define DEFAULT_FRAMES    16
#define DEFAULT_FPS       8
#define DEFAULT_STEPS     30
#define DEFAULT_CFG       7.5f
#define DEFAULT_THREADS   0
#define DEFAULT_SEED      -1

/* ============================================================================
 * Progress Callback
 * ============================================================================ */

static int progress_callback(int step, int total_steps, float progress, void* user_data) {
    (void)user_data;

    // Clear line and show progress bar
    printf("\r[");
    int bar_width = 40;
    int filled = (int)(progress * bar_width);
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            printf("=");
        } else {
            printf(" ");
        }
    }
    printf("] Step %d/%d (%.1f%%)", step + 1, total_steps, progress * 100.0f);
    fflush(stdout);

    return 0;  // Continue generation
}

/* ============================================================================
 * Help Message
 * ============================================================================ */

static void print_usage(const char* program_name) {
    printf("wan-cpp CLI - Video generation using Wan models (WAN2.1, WAN2.2)\n\n");
    printf("Usage: %s [options]\n\n", program_name);
    printf("Required Options:\n");
    printf("  -m, --model <path>         Path to GG model file\n");
    printf("  -p, --prompt <text>         Text prompt for generation\n\n");
    printf("Optional Options:\n");
    printf("  -i, --input <path>         Input image for I2V mode\n");
    printf("  -o, --output <path>        Output video path (default: output.avi)\n");
    printf("  -b, --backend <type>       Backend: cpu, cuda, metal, vulkan (default: cpu)\n");
    printf("  -t, --threads <num>        Number of threads (0 = auto, default: 0)\n");
    printf("\nGeneration Parameters:\n");
    printf("  -W, --width <pixels>        Output width (default: %d)\n", DEFAULT_WIDTH);
    printf("  -H, --height <pixels>       Output height (default: %d)\n", DEFAULT_HEIGHT);
    printf("  -f, --frames <num>         Number of frames (default: %d)\n", DEFAULT_FRAMES);
    printf("  --fps <num>                Frames per second (default: %d)\n", DEFAULT_FPS);
    printf("  -s, --steps <num>          Sampling steps (default: %d)\n", DEFAULT_STEPS);
    printf("  --seed <num>               Random seed (default: random)\n");
    printf("  --cfg <float>               Guidance scale (default: %.1f)\n", DEFAULT_CFG);
    printf("  -n, --negative <text>      Negative prompt\n");
    printf("\nOther Options:\n");
    printf("  -v, --verbose              Verbose output\n");
    printf("  -h, --help                 Show this help message\n");
    printf("  --version                   Show version information\n\n");
    printf("Examples:\n");
    printf("  # Text-to-Video generation\n");
    printf("  %s -m model.gguf -p \"A cat playing\" -o video.avi\n\n", program_name);
    printf("  # Image-to-Video generation\n");
    printf("  %s -m model.gguf -i frame.jpg -p \"Make it move\" -o output.avi\n\n", program_name);
    printf("  # High quality generation\n");
    printf("  %s -m model.gguf -p \"A sunset beach\" -s 50 --cfg 12.0 -W 1024 -H 576\n\n", program_name);
}

/* ============================================================================
 * Parse Command Line Arguments
 * ============================================================================ */

static void init_options(cli_options_t* opts) {
    opts->model_path = NULL;
    opts->prompt = NULL;
    opts->input_image = NULL;
    opts->output_path = NULL;
    opts->backend = NULL;
    opts->negative_prompt = NULL;

    opts->threads = DEFAULT_THREADS;
    opts->width = DEFAULT_WIDTH;
    opts->height = DEFAULT_HEIGHT;
    opts->num_frames = DEFAULT_FRAMES;
    opts->fps = DEFAULT_FPS;
    opts->steps = DEFAULT_STEPS;
    opts->seed = DEFAULT_SEED;
    opts->cfg = DEFAULT_CFG;

    opts->verbose = 0;
    opts->show_help = 0;
    opts->show_version = 0;
    opts->mode = 0;
}

static int parse_args(cli_options_t* opts, int argc, char** argv) {
    init_options(opts);

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        /* Help */
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            opts->show_help = 1;
            return 0;
        }

        /* Version */
        if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0 && (i == 1 || strcmp(argv[i-1], "-v") != 0)) {
            if (strcmp(arg, "--version") == 0) {
                opts->show_version = 1;
                return 0;
            }
        }

        /* Model path */
        if ((strcmp(arg, "-m") == 0 || strcmp(arg, "--model") == 0) && i + 1 < argc) {
            opts->model_path = argv[++i];
            continue;
        }

        /* Prompt */
        if ((strcmp(arg, "-p") == 0 || strcmp(arg, "--prompt") == 0) && i + 1 < argc) {
            opts->prompt = argv[++i];
            continue;
        }

        /* Input image */
        if ((strcmp(arg, "-i") == 0 || strcmp(arg, "--input") == 0) && i + 1 < argc) {
            opts->input_image = argv[++i];
            continue;
        }

        /* Output path */
        if ((strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) && i + 1 < argc) {
            opts->output_path = argv[++i];
            continue;
        }

        /* Backend */
        if ((strcmp(arg, "-b") == 0 || strcmp(arg, "--backend") == 0) && i + 1 < argc) {
            opts->backend = argv[++i];
            continue;
        }

        /* Threads */
        if ((strcmp(arg, "-t") == 0 || strcmp(arg, "--threads") == 0) && i + 1 < argc) {
            opts->threads = atoi(argv[++i]);
            continue;
        }

        /* Width */
        if ((strcmp(arg, "-W") == 0 || strcmp(arg, "--width") == 0) && i + 1 < argc) {
            opts->width = atoi(argv[++i]);
            continue;
        }

        /* Height */
        if ((strcmp(arg, "-H") == 0 || strcmp(arg, "--height") == 0) && i + 1 < argc) {
            opts->height = atoi(argv[++i]);
            continue;
        }

        /* Frames */
        if ((strcmp(arg, "-f") == 0 || strcmp(arg, "--frames") == 0) && i + 1 < argc) {
            opts->num_frames = atoi(argv[++i]);
            continue;
        }

        /* FPS */
        if (strcmp(arg, "--fps") == 0 && i + 1 < argc) {
            opts->fps = atoi(argv[++i]);
            continue;
        }

        /* Steps */
        if ((strcmp(arg, "-s") == 0 || strcmp(arg, "--steps") == 0) && i + 1 < argc) {
            opts->steps = atoi(argv[++i]);
            continue;
        }

        /* Seed */
        if (strcmp(arg, "--seed") == 0 && i + 1 < argc) {
            opts->seed = atoi(argv[++i]);
            continue;
        }

        /* CFG */
        if (strcmp(arg, "--cfg") == 0 && i + 1 < argc) {
            opts->cfg = (float)atof(argv[++i]);
            continue;
        }

        /* Negative prompt */
        if ((strcmp(arg, "-n") == 0 || strcmp(arg, "--negative") == 0 ||
             strcmp(arg, "--negative-prompt") == 0) && i + 1 < argc) {
            opts->negative_prompt = argv[++i];
            continue;
        }

        /* Verbose */
        if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
            opts->verbose = 1;
            continue;
        }

        /* Unknown option */
        fprintf(stderr, "Error: Unknown option: %s\n", arg);
        return 1;
    }

    /* Determine mode */
    if (opts->input_image) {
        opts->mode = 2;  // I2V
    } else if (opts->prompt) {
        opts->mode = 1;  // T2V
    }

    /* Set default output path */
    if (!opts->output_path) {
        opts->output_path = (char*)"output.avi";
    }

    /* Set default backend */
    if (!opts->backend) {
        opts->backend = (char*)"cpu";
    }

    /* Set random seed if needed */
    if (opts->seed < 0) {
        opts->seed = (int)time(NULL);
    }

    return 0;
}

/* ============================================================================
 * Validate Options
 * ============================================================================ */

static int validate_options(const cli_options_t* opts) {
    if (!opts->model_path) {
        fprintf(stderr, "Error: Model path is required. Use -m or --model.\n");
        return 1;
    }

    if (!opts->prompt) {
        fprintf(stderr, "Error: Prompt is required. Use -p or --prompt.\n");
        return 1;
    }

    if (opts->width <= 0 || opts->width > 4096) {
        fprintf(stderr, "Error: Invalid width: %d. Must be 1-4096.\n", opts->width);
        return 1;
    }

    if (opts->height <= 0 || opts->height > 4096) {
        fprintf(stderr, "Error: Invalid height: %d. Must be 1-4096.\n", opts->height);
        return 1;
    }

    if (opts->num_frames <= 0 || opts->num_frames > 1000) {
        fprintf(stderr, "Error: Invalid frame count: %d. Must be 1-1000.\n", opts->num_frames);
        return 1;
    }

    if (opts->steps <= 0 || opts) {
        /* Allow any positive step count */
    }

    if (opts->fps <= 0 || opts->fps > 120) {
        fprintf(stderr, "Error: Invalid FPS: %d. Must be 1-120.\n", opts->fps);
        return 1;
    }

    return 0;
}

/* ============================================================================
 * Print Generation Info
 * ============================================================================ */

static void print_generation_info(const cli_options_t* opts) {
    printf("\n");
    printf("==================================================\n");
    printf("  wan-cpp Video Generation\n");
    printf("==================================================\n\n");

    printf("Mode: ");
    if (opts->mode == 2) {
        printf("Image-to-Video (I2V)\n");
        printf("Input: %s\n", opts->input_image);
    } else {
        printf("Text-to-Video (T2V)\n");
    }

    printf("\nModel: %s\n", opts->model_path);
    printf("Prompt: %s\n", opts->prompt);

    if (opts->negative_prompt) {
        printf("Negative: %s\n", opts->negative_prompt);
    }

    printf("\nOutput: %s\n", opts->output_path);
    printf("Backend: %s\n", opts->backend);
    printf("Threads: %d\n", opts->threads);

    printf("\nParameters:\n");
    printf("  Resolution: %dx%d\n", opts->width, opts->height);
    printf("  Frames: %d @ %d FPS\n", opts->num_frames, opts->fps);
    printf("  Steps: %d\n", opts->steps);
    printf("  CFG: %.2f\n", opts->cfg);
    printf("  Seed: %d\n", opts->seed);

    printf("\n==================================================\n\n");
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char** argv) {
    cli_options_t opts;

    /* Parse arguments */
    if (parse_args(&opts, argc, argv) != 0) {
        fprintf(stderr, "Use --help for usage information.\n");
        return 1;
    }

    /* Handle special options */
    if (opts.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (opts.show_version) {
        printf("wan-cpp version: %s\n", wan_version());
        return 0;
    }

    /* Validate required options */
    if (validate_options(&opts) != 0) {
        return 1;
    }

    /* Print generation info */
    print_generation_info(&opts);

    /* Load model */
    wan_context_t* ctx = NULL;
    wan_error_t err;

    printf("Loading model...\n");
    err = wan_load_model(opts.model_path, opts.threads, opts.backend, &ctx);

    if (err != WAN_SUCCESS) {
        fprintf(stderr, "Error loading model: %d\n", err);
        if (ctx) {
            fprintf(stderr, "Details: %s\n", wan_get_last_error(ctx));
            wan_free(ctx);
        }
        return 1;
    }

    printf("Model loaded successfully!\n\n");

    /* Prepare parameters */
    wan_params_t* params = wan_params_create();
    if (!params) {
        fprintf(stderr, "Error creating parameters\n");
        wan_free(ctx);
        return 1;
    }

    wan_params_set_seed(params, opts.seed);
    wan_params_set_steps(params, opts.steps);
    wan_params_set_cfg(params, opts.cfg);
    wan_params_set_size(params, opts.width, opts.height);
    wan_params_set_num_frames(params, opts.num_frames);
    wan_params_set_fps(params, opts.fps);
    wan_params_set_n_threads(params, opts.threads);
    wan_params_set_backend(params, opts.backend);
    wan_params_set_progress_callback(params, progress_callback, NULL);

    if (opts.negative_prompt) {
        wan_params_set_negative_prompt(params, opts.negative_prompt);
    }

    /* Generate video */
    printf("Generating video...\n\n");
    err = WAN_ERROR_UNKNOWN;

    if (opts.mode == 2) {
        /* I2V mode */
        wan_image_t* image = NULL;
        err = wan_load_image(opts.input_image, &image);

        if (err != WAN_SUCCESS) {
            fprintf(stderr, "Error loading input image: %d\n", err);
            wan_params_free(params);
            wan_free(ctx);
            return 1;
        }

        err = wan_generate_video_i2v_ex(ctx, image, opts.prompt, params, opts.output_path);
        wan_free_image(image);
    } else {
        /* T2V mode */
        err = wan_generate_video_t2v_ex(ctx, opts.prompt, params, opts.output_path);
    }

    printf("\n");

    /* Check result */
    if (err != WAN_SUCCESS) {
        fprintf(stderr, "Generation failed: %d\n", err);
        if (ctx) {
            fprintf(stderr, "Details: %s\n", wan_get_last_error(ctx));
        }
        wan_params_free(params);
        wan_free(ctx);
        return 1;
    }

    printf("\nVideo saved to: %s\n", opts.output_path);
    printf("Done!\n");

    /* Cleanup */
    wan_params_free(params);
    wan_free(ctx);

    return 0;
}
