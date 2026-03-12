/**
 * @file main.cpp
 * @brief CLI example for wan-cpp video generation library
 *
 * This example demonstrates the structure of a CLI application for wan-cpp.
 * It shows argument parsing, parameter configuration, and AVI output.
 *
 * Note: This is a structural example. To use the full wan-cpp API,
 * you would integrate with the actual model loading and generation calls.
 */

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// Stub types for compilation demonstration
typedef struct wan_context wan_context_t;
typedef struct wan_params wan_params_t;
typedef struct wan_image wan_image_t;
typedef int (*wan_progress_cb_t)(int step, int total_steps, float progress, void* user_data);

/* ============================================================================
 * Command-line Options
 * ============================================================================ */

struct WanCliOptions {
    std::string model_path;
    std::string prompt;
    std::string negative_prompt;
    std::string input_image;
    std::string output_path = "output.avi";
    std::string backend = "cpu";

    int width = 640;
    int height = 480;
    int num_frames = 16;
    int fps = 16;
    int steps = 30;
    int seed = -1;
    int n_threads = 0;
    float cfg = 7.0f;

    bool verbose = false;
    bool show_help = false;
};

/* ============================================================================
 * Argument Parsing
 * ============================================================================ */

void print_usage(const char* program_name) {
    printf("wan-cli - WAN Video Generation CLI\n");
    printf("Version: 1.0.0\n\n");
    printf("Usage: %s [options]\n\n", program_name);
    printf("Required options:\n");
    printf("  -m, --model <path>        Path to WAN model file (GGUF format)\n");
    printf("  -p, --prompt <text>        Text prompt for T2V or I2V\n\n");
    printf("Optional options:\n");
    printf("  -i, --input <path>         Input image for I2V mode\n");
    printf("  -o, --output <path>        Output video path (default: output.avi)\n");
    printf("  -b, --backend <type>       Backend type (default: cpu)\n");
    printf("                             Available: cpu, cuda, metal, vulkan\n");
    printf("  -t, --threads <num>        Number of threads (default: auto)\n");
    printf("  -W, --width <pixels>       Output width (default: 640)\n");
    printf("  -H, --height <pixels>      Output height (default: 480)\n");
    printf("  -f, --frames <num>        Number of frames (default: 16)\n");
    printf("  --fps <num>                Frames per second (default: 16)\n");
    printf("  -s, --steps <num>         Sampling steps (default: 30)\n");
    printf("  --seed <num>                Random seed (default: random)\n");
    printf("  -c, --cfg <value>          CFG scale (default: 7.0)\n");
    printf("  -n, --negative <text>      Negative prompt\n");
    printf("  -v, --verbose               Enable verbose output\n");
    printf("  -h, --help                 Show this help message\n");
}

bool parse_args(int argc, char** argv, WanCliOptions& opts) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-m" || arg == "--model") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.model_path = argv[i];
        } else if (arg == "-p" || arg == "--prompt") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.prompt = argv[i];
        } else if (arg == "-i" || arg == "--input") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.input_image = argv[i];
        } else if (arg == "-o" || arg == "--output") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.output_path = argv[i];
        } else if (arg == "-b" || arg == "--backend") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.backend = argv[i];
        } else if (arg == "-t" || arg == "--threads") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.n_threads = std::stoi(argv[i]);
        } else if (arg == "-W" || arg == "--width") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.width = std::stoi(argv[i]);
        } else if (arg == "-H" || arg == "--height") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.height = std::stoi(argv[i]);
        } else if (arg == "-f" || arg == "--frames") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.num_frames = std::stoi(argv[i]);
        } else if (arg == "--fps") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.fps = std::stoi(argv[i]);
        } else if (arg == "-s" || arg == "--steps") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.steps = std::stoi(argv[i]);
        } else if (arg == "--seed") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.seed = std::stoi(argv[i]);
        } else if (arg == "-c" || arg == "--cfg") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.cfg = std::stof(argv[i]);
        } else if (arg == "-n" || arg == "--negative") {
            if (++i >= argc) { fprintf(stderr, "Error: missing value for %s\n", arg.c_str()); return false; }
            opts.negative_prompt = argv[i];
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            opts.show_help = true;
            return true;
        } else {
            fprintf(stderr, "Error: unknown option: %s\n", arg.c_str());
            return false;
        }
    }

    // Validate required arguments
    if (opts.model_path.empty()) {
        fprintf(stderr, "Error: model path is required (-m or --model)\n");
        return false;
    }
    if (opts.prompt.empty()) {
        fprintf(stderr, "Error: prompt is required (-p or --prompt)\n");
        return false;
    }

    return true;
}

/* ============================================================================
 * Progress Callback
 * ============================================================================ */

int progress_callback(int step, int total_steps, float progress, void* user_data) {
    WanCliOptions* opts = (WanCliOptions*)user_data;
    if (opts && opts->verbose) {
        printf("Step %d/%d (%.1f%%)\n", step, total_steps, progress * 100);
    } else {
        printf("\rProgress: %d/%d (%.1f%%)", step, total_steps, progress * 100);
        fflush(stdout);
    }
    return 0;  // Continue
}

/* ============================================================================
 * Main Function
 * ============================================================================ */

int main(int argc, char** argv) {
    WanCliOptions opts;

    if (!parse_args(argc, argv, opts)) {
        print_usage(argv[0]);
        return 1;
    }

    if (opts.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    // Print configuration
    printf("Configuration:\n");
    printf("  Model: %s\n", opts.model_path.c_str());
    printf("  Mode: %s\n", opts.input_image.empty() ? "T2V" : "I2V");
    printf("  Prompt: %s\n", opts.prompt.c_str());
    if (!opts.negative_prompt.empty()) {
        printf("  Negative prompt: %s\n", opts.negative_prompt.c_str());
    }
    if (!opts.input_image.empty()) {
        printf("  Input image: %s\n", opts.input_image.c_str());
    }
    printf("  Output: %s\n", opts.output_path.c_str());
    printf("  Size: %dx%d\n", opts.width, opts.height);
    printf("  Frames: %d @ %d fps\n", opts.num_frames, opts.fps);
    printf("  Steps: %d\n", opts.steps);
    printf("  CFG: %.1f\n", opts.cfg);
    printf("  Seed: %d\n", opts.seed);
    printf("  Backend: %s\n", opts.backend.c_str());
    printf("  Threads: %d\n", opts.n_threads);
    printf("\n");

    printf("This is a structural example showing CLI pattern.\n");
    printf("For actual video generation, integrate with wan-cpp API.\n");
    printf("\n");

    printf("The following would be the actual flow:\n");
    printf("1. Load model: wan_load_model(...)\n");
    printf("2. Generate video: wan_generate_video_t2v(...) or wan_generate_video_i2v(...)\n");
    printf("3. Save output to: %s\n", opts.output_path.c_str());

    return 0;
}
