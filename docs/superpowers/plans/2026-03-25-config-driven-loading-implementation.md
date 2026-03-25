# WanModel 配置文件加载功能 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 WanModel 加载逻辑改造为由 JSON 配置文件驱动的分布式加载方式，支持独立的 Transformer、VAE、Text Encoder、CLIP 权重文件路径。

**Architecture:** 通过新增 ConfigLoader 类解析 JSON 配置文件，提取运行环境参数（backend、threads、GPU）和各子模型路径。重写 `Wan::WanModel::load()` 方法，使其接收配置文件路径而非权重文件路径，分别初始化和加载各组件。更新 CLI 接口移除 `-m` 参数，改为位置参数接收配置文件路径。

**Tech Stack:** nlohmann/json（已有）, GGUF/Safetensors 权重加载

---

## 文件结构 (File Structure)

| 文件 | 变更 | 说明 |
|------|------|------|
| `src/config_loader.hpp` | 创建 | ConfigLoader 类声明 |
| `src/config_loader.cpp` | 创建 | ConfigLoader 类实现 |
| `src/api/wan-api.cpp` | 修改 | 重写 `WanModel::load()` 和 `wan_load_model()` |
| `examples/cli/main.cpp` | 修改 | 移除 `-m` 参数，改为位置参数 |
| `include/wan.h` | 修改 | 更新 API 注释/文档 |

---

## 任务分解 (Task Breakdown)

### Task 1: 创建 ConfigLoader 类

**Files:**
- Create: `src/config_loader.hpp`
- Create: `src/config_loader.cpp`
- Modify: `CMakeLists.txt`

**Description:** 定义并实现 ConfigLoader 类，用于解析主配置文件和 wan_config.json。

- [ ] **Step 1: 创建 config_loader.hpp 头文件**

```cpp
#ifndef __CONFIG_LOADER_HPP__
#define __CONFIG_LOADER_HPP__

#include <string>
#include <vector>
#include "nlohmann/json.hpp"  // Include nlohmann/json
#include "model.h"             // For SDVersion

struct WanLoadConfig {
    // Runtime environment
    std::string backend;
    int n_threads;
    std::vector<int> gpu_ids;

    // Model paths
    std::string transformer_path;
    std::string vae_path;
    std::string text_encoder_path;
    std::string clip_path;  // Optional

    // Architecture config file
    std::string wan_config_file;
};

struct WanArchConfig {
    // From wan_config.json
    std::string model_type;      // t2v, i2v, ti2v
    int dim;
    int num_heads;
    int num_layers;
    int in_dim;
    int out_dim;
    int text_len;
    float eps;
    int ffn_dim;
    int freq_dim;
};

class ConfigLoader {
public:
    /**
     * Load main configuration from JSON file
     * @param config_path Path to config.json
     * @return Parsed WanLoadConfig
     * @throws std::runtime_error if file not found or JSON is invalid
     */
    static WanLoadConfig load_config(const std::string& config_path);

    /**
     * Load architecture configuration from JSON file
     * @param config_path Path to wan_config.json
     * @return Parsed WanArchConfig
     * @throws std::runtime_error if file not found or JSON is invalid
     */
    static WanArchConfig load_arch_config(const std::string& config_path);

    /**
     * Validate that all required model paths exist
     * @param config WanLoadConfig to validate
     * @throws std::runtime_error if required files are missing
     */
    static void validate_required_files(const WanLoadConfig& config);
};

#endif // __CONFIG_LOADER_HPP__
```

- [ ] **Step 2: 实现 config_loader.cpp**

```cpp
#include "config_loader.hpp"
#include "util.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

WanLoadConfig ConfigLoader::load_config(const std::string& config_path) {
    LOG_DEBUG("ConfigLoader::load_config: parsing %s", config_path.c_str());

    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Cannot open config file: " + config_path);
    }

    nlohmann::json j;
    try {
        ifs >> j;
    } catch (const std::exception& e) {
        throw std::runtime_error("JSON parse error in " + config_path + ": " + e.what());
    }

    WanLoadConfig cfg;

    // Parse fields with defaults (backend, n_threads, gpu_ids)
    cfg.backend = j.value("backend", "cpu");
    cfg.n_threads = j.value("n_threads", 0);

    if (j.contains("gpu_ids") && j["gpu_ids"].is_array()) {
        cfg.gpu_ids = j["gpu_ids"].get<std::vector<int>>();
    }

    // Parse required model paths
    try {
        cfg.transformer_path = j.at("models").at("transformer_path").get<std::string>();
        cfg.vae_path = j.at("models").at("vae_path").get<std::string>();
        cfg.text_encoder_path = j.at("models").at("text_encoder_path").get<std::string>();
        cfg.wan_config_file = j.at("wan_config_file").get<std::string>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Missing required field in config: " + std::string(e.what()));
    }

    // Parse optional clip_path
    if (j.contains("models") && j["models"].contains("clip_path")) {
        cfg.clip_path = j["models"]["clip_path"].get<std::string>();
    }

    LOG_DEBUG("ConfigLoader::load_config: parsed successfully");
    return cfg;
}

WanArchConfig ConfigLoader::load_arch_config(const std::string& config_path) {
    LOG_DEBUG("ConfigLoader::load_arch_config: parsing %s", config_path.c_str());

    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Cannot open arch config file: " + config_path);
    }

    nlohmann::json j;
    try {
        ifs >> j;
    } catch (const std::exception& e) {
        throw std::runtime_error("JSON parse error in " + config_path + ": " + e.what());
    }

    WanArchConfig cfg;

    try {
        cfg.model_type = j.at("model_type").get<std::string>();
        cfg.dim = j.at("dim").get<int>();
        cfg.num_heads = j.at("num_heads").get<int>();
        cfg.num_layers = j.at("num_layers").get<int>();
        cfg.in_dim = j.at("in_dim").get<int>();
        cfg.out_dim = j.at("out_dim").get<int>();
        cfg.text_len = j.at("text_len").get<int>();
        cfg.eps = j.value("eps", 1e-6f);
        cfg.ffn_dim = j.at("ffn_dim").get<int>();
        cfg.freq_dim = j.at("freq_dim").get<int>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Missing required arch field: " + std::string(e.what()));
    }

    LOG_DEBUG("ConfigLoader::load_arch_config: parsed successfully, model_type=%s", cfg.model_type.c_str());
    return cfg;
}

void ConfigLoader::validate_required_files(const WanLoadConfig& config) {
    LOG_DEBUG("ConfigLoader::validate_required_files: validating required files");

    // Check required files
    std::vector<std::pair<std::string, std::string>> required = {
        {"transformer_path", config.transformer_path},
        {"vae_path", config.vae_path},
        {"text_encoder_path", config.text_encoder_path}
    };

    for (const auto& [name, path] : required) {
        if (!file_exists(path)) {
            throw std::runtime_error("Required model file not found: " + name + " = " + path);
        }
        LOG_DEBUG("ConfigLoader::validate_required_files: ✓ %s exists", name.c_str());
    }

    // Check wan_config_file
    if (!file_exists(config.wan_config_file)) {
        throw std::runtime_error("Architecture config file not found: " + config.wan_config_file);
    }
    LOG_DEBUG("ConfigLoader::validate_required_files: ✓ wan_config_file exists");

    // Optional: warn if clip_path is empty but might be needed for i2v/ti2v
    // (actual check is done in WanModel::load() after reading model_type)
}
```

- [ ] **Step 3: 在 CMakeLists.txt 中添加源文件**

修改 `CMakeLists.txt`，在 `add_library(wan-api ...)` 目标中添加 `src/config_loader.cpp`：

**Before:** 找到包含 wan-api 库定义的行（大约在第 100-150 行）
```bash
grep -n "add_library(wan-api" CMakeLists.txt
```

**After:** 在源文件列表中添加 `src/config_loader.cpp`，例如：
```cmake
add_library(wan-api SHARED
    src/api/wan-api.cpp
    src/api/wan_loader.cpp
    src/config_loader.cpp    # ← 添加此行
    ... other sources ...
)
```

- [ ] **Step 4: 提交**

```bash
git add src/config_loader.hpp src/config_loader.cpp CMakeLists.txt
git commit -m "feat: add ConfigLoader class for JSON-based model configuration"
```

---

### Task 1.5: 验证编译（ConfigLoader）

**Files:**
- None (verification only)

**Description:** 确保 ConfigLoader 类能够独立编译。

- [ ] **Step 1: 编译验证**

```bash
cd /data/zhongwang2/jtzhuang/projects/OnlyWan/build
cmake .. && make -j$(nproc) 2>&1 | head -50
```

预期输出：编译成功，或仅显示与尚未修改的 wan-api.cpp 有关的错误（这在 Task 2 中修复）。

如果有 ConfigLoader 相关的编译错误，立即修复。

---

### Task 2: 修改 WanModel::load() 使用 ConfigLoader

**Files:**
- Modify: `src/api/wan-api.cpp` (主要改造此文件中的 `Wan::WanModel::load()` 方法)
- Modify: `src/api/wan-api.h` (如有需要更新函数签名)

**Description:** 重写 `WanModel::load()` 方法，使其接收配置文件路径而非权重文件路径，并使用 ConfigLoader 解析配置。

**Includes needed:** 在 `src/api/wan-api.cpp` 顶部，需要确保已有以下 include：
```cpp
#include "config_loader.hpp"
#include "model.h"           // For SDVersion, ModelLoader
#include "util.h"            // For file_exists, LOG_* macros
#include "ggml.h"
// ... 其他现有 include ...
```

- [ ] **Step 1: 备份原有的 WanModel::load() 逻辑**

在 `src/api/wan-api.cpp` 中，当前 `WanModel::load()` 接收 GGUF/safetensors 文件路径（行号约 131-339）。我们需要：
1. 将当前实现复制备份（可选），然后整体替换
2. 重写 `WanModel::load()` 为配置驱动版本

**Before:** 检查当前 WanModel::load() 的位置
```bash
grep -n "WanModelLoadResult Wan::WanModel::load" src/api/wan-api.cpp
# 应该输出: 131:WanModelLoadResult Wan::WanModel::load(const std::string& file_path) {
```

- [ ] **Step 2: 在 WanModel::load() 顶部添加配置解析**

```cpp
WanModelLoadResult Wan::WanModel::load(const std::string& config_path) {
    LOG_DEBUG("WanModel::load: START, config_path='%s'", config_path.c_str());

    WanModelLoadResult result;
    result.success = false;

    // Step 1: 验证配置文件存在
    {
        std::ifstream test_file(config_path);
        if (!test_file.good()) {
            result.error_message = "Config file not found: " + config_path;
            LOG_ERROR("WanModel::load: config file not found: %s", config_path.c_str());
            return result;
        }
    }

    // Step 2: 解析主配置文件
    WanLoadConfig config;
    try {
        config = ConfigLoader::load_config(config_path);
        LOG_DEBUG("WanModel::load: parsed main config successfully");
    } catch (const std::exception& e) {
        result.error_message = std::string("Failed to parse config: ") + e.what();
        LOG_ERROR("WanModel::load: config parse error: %s", result.error_message.c_str());
        return result;
    }

    // Step 3: 验证必需文件存在
    try {
        ConfigLoader::validate_required_files(config);
        LOG_DEBUG("WanModel::load: all required files validated");
    } catch (const std::exception& e) {
        result.error_message = std::string("File validation failed: ") + e.what();
        LOG_ERROR("WanModel::load: %s", result.error_message.c_str());
        return result;
    }

    // Step 4: 加载架构配置
    WanArchConfig arch_config;
    try {
        arch_config = ConfigLoader::load_arch_config(config.wan_config_file);
        LOG_DEBUG("WanModel::load: loaded arch config, model_type='%s'", arch_config.model_type.c_str());
    } catch (const std::exception& e) {
        result.error_message = std::string("Failed to load arch config: ") + e.what();
        LOG_ERROR("WanModel::load: arch config error: %s", result.error_message.c_str());
        return result;
    }

    // ... 继续下一部分
}
```

- [ ] **Step 3: 添加 Backend 初始化逻辑**

在配置解析后，根据 `config.backend` 初始化后端：

```cpp
    // Step 5: 初始化 CPU backend 用于权重加载
    LOG_DEBUG("WanModel::load: initializing CPU backend");
    ggml_backend_t backend = ggml_backend_cpu_init();
    if (!backend) {
        result.error_message = "Failed to initialize CPU backend for model loading";
        LOG_ERROR("WanModel::load: CPU backend init failed");
        return result;
    }

    LOG_DEBUG("WanModel::load: creating ModelLoader");
    ModelLoader model_loader;

    // ... 继续加载各个模型文件
```

- [ ] **Step 4: 分别加载 Transformer, VAE, Text Encoder, CLIP**

对于每个模型组件，使用对应的路径从 `config` 中加载。可参考原有逻辑改造：

```cpp
    // --- Transformer (WanRunner) ---
    LOG_DEBUG("WanModel::load: loading Transformer from %s", config.transformer_path.c_str());
    if (!model_loader.init_from_file_and_convert_name(config.transformer_path, "model.diffusion_model.")) {
        result.error_message = "Failed to load Transformer from: " + config.transformer_path;
        ggml_backend_free(backend);
        LOG_ERROR("WanModel::load: Transformer load failed");
        return result;
    }

    auto& tensor_storage_map = model_loader.get_tensor_storage_map();

    // Determine SD version from arch_config
    SDVersion sd_version = VERSION_WAN2;
    if (arch_config.model_type == "i2v")  sd_version = VERSION_WAN2_2_I2V;
    if (arch_config.model_type == "ti2v") sd_version = VERSION_WAN2_2_TI2V;

    auto wan_runner = std::make_shared<WAN::WanRunner>(
        backend, /*offload_params_to_cpu=*/false,
        tensor_storage_map, "", sd_version);

    wan_runner->alloc_params_buffer();

    {
        LOG_DEBUG("WanModel::load: loading Transformer tensors");
        std::map<std::string, ggml_tensor*> tensors;
        wan_runner->get_param_tensors(tensors, "");
        if (!model_loader.load_tensors(tensors)) {
            result.error_message = "Failed to load Transformer tensors";
            ggml_backend_free(backend);
            LOG_ERROR("WanModel::load: Transformer tensor load failed");
            return result;
        }
    }

    // --- VAE ---
    LOG_DEBUG("WanModel::load: loading VAE from %s", config.vae_path.c_str());
    ModelLoader vae_loader;
    if (!vae_loader.init_from_file_and_convert_name(config.vae_path, "")) {
        result.error_message = "Failed to load VAE from: " + config.vae_path;
        ggml_backend_free(backend);
        LOG_ERROR("WanModel::load: VAE load failed");
        return result;
    }

    auto& vae_tensor_map = vae_loader.get_tensor_storage_map();
    auto vae_runner = std::make_shared<WAN::WanVAERunner>(
        backend, /*offload_params_to_cpu=*/false,
        vae_tensor_map, "", /*decode_only=*/false, sd_version);

    vae_runner->alloc_params_buffer();

    {
        LOG_DEBUG("WanModel::load: loading VAE tensors");
        std::map<std::string, ggml_tensor*> vae_tensors;
        vae_runner->get_param_tensors(vae_tensors, "first_stage_model");
        vae_loader.load_tensors(vae_tensors);
    }

    // --- Text Encoder (T5) ---
    LOG_DEBUG("WanModel::load: loading Text Encoder from %s", config.text_encoder_path.c_str());
    ModelLoader t5_loader;
    if (!t5_loader.init_from_file(config.text_encoder_path)) {
        result.error_message = "Failed to load Text Encoder from: " + config.text_encoder_path;
        ggml_backend_free(backend);
        LOG_ERROR("WanModel::load: Text Encoder load failed");
        return result;
    }

    auto& t5_tensor_map = t5_loader.get_tensor_storage_map();
    auto t5_embedder = std::make_shared<T5Embedder>(
        backend, false, t5_tensor_map, "", /*is_umt5=*/true);

    t5_embedder->alloc_params_buffer();

    {
        LOG_DEBUG("WanModel::load: loading T5 tensors");
        std::map<std::string, ggml_tensor*> t5_tensors;
        t5_embedder->get_param_tensors(t5_tensors, "");
        t5_loader.load_tensors(t5_tensors);
    }

    // --- CLIP (Optional) ---
    std::shared_ptr<CLIPVisionModelProjectionRunner> clip_runner;
    if (!config.clip_path.empty() && (arch_config.model_type == "i2v" || arch_config.model_type == "ti2v")) {
        LOG_DEBUG("WanModel::load: loading CLIP from %s", config.clip_path.c_str());
        ModelLoader clip_loader;
        if (!clip_loader.init_from_file(config.clip_path)) {
            LOG_WARN("WanModel::load: failed to load CLIP, continuing without it");
        } else {
            auto& clip_tensor_map = clip_loader.get_tensor_storage_map();
            clip_runner = std::make_shared<CLIPVisionModelProjectionRunner>(
                backend, false, clip_tensor_map, "", OPEN_CLIP_VIT_H_14);

            clip_runner->alloc_params_buffer();

            {
                LOG_DEBUG("WanModel::load: loading CLIP tensors");
                std::map<std::string, ggml_tensor*> clip_tensors;
                clip_runner->get_param_tensors(clip_tensors, "");
                clip_loader.load_tensors(clip_tensors);
            }
        }
    }

    // Set result and return
    result.success = true;
    result.wan_runner = wan_runner;
    result.vae_runner = vae_runner;
    result.t5_embedder = t5_embedder;
    result.clip_runner = clip_runner;
    result.model_type = arch_config.model_type;
    result.model_version = "WAN2.1";  // Can be extended if needed
    return result;
}
```

- [ ] **Step 5: 编译并运行单元测试**

```bash
cd /data/zhongwang2/jtzhuang/projects/OnlyWan
mkdir -p build && cd build
cmake .. && make
```

预期输出：编译成功，无新错误。

- [ ] **Step 6: 提交**

```bash
git add src/api/wan-api.cpp
git commit -m "feat: rewrite WanModel::load() to use config-driven approach"
```

---

### Task 3: 更新 wan_load_model() API

**Files:**
- Modify: `src/api/wan-api.cpp`

**Description:** 更新公共 API `wan_load_model()` 使其调用新的 `WanModel::load()`。

- [ ] **Step 1: 修改 wan_load_model() 函数签名和实现**

```cpp
WAN_API wan_error_t wan_load_model(const char* config_path,
                                   int n_threads,
                                   const char* backend_type,
                                   wan_context_t** out_ctx) {
    if (!config_path || !out_ctx) {
        LOG_ERROR("wan_load_model: invalid arguments");
        return WAN_ERROR_INVALID_ARGUMENT;
    }

#ifndef WAN_EMBED_VOCAB
    LOG_DEBUG("wan_load_model: WAN_EMBED_VOCAB not defined, checking vocab dir");
    if (!wan_vocab_dir_is_set()) {
        LOG_ERROR("wan_load_model: vocab dir not set");
        return WAN_ERROR_INVALID_ARGUMENT;
    }
    // ... 原有的 vocab 目录检查逻辑保持不变
#endif

    // Create context
    LOG_DEBUG("wan_load_model: creating context");
    std::unique_ptr<wan_context> ctx(new wan_context());
    if (!ctx) {
        LOG_ERROR("wan_load_model: failed to create context (out of memory)");
        return WAN_ERROR_OUT_OF_MEMORY;
    }

    // 注意：ctx->model_path 现在存储的是配置文件路径，不是权重文件路径
    ctx->model_path = config_path;
    ctx->n_threads = n_threads;
    ctx->backend_type = backend_type ? backend_type : "cpu";

    // Load model via config-driven approach
    LOG_INFO("Loading model from config: %s", ctx->model_path.c_str());
    WanModelLoadResult result = Wan::WanModel::load(ctx->model_path);
    if (result.success) {
        LOG_INFO("Model loaded successfully: %s %s", result.model_version.c_str(), result.model_type.c_str());
    } else {
        LOG_ERROR("Model load failed: %s", result.error_message.c_str());
        set_last_error(ctx.get(), result.error_message.c_str());
        return WAN_ERROR_MODEL_LOAD_FAILED;
    }

    ctx->wan_runner  = result.wan_runner;
    ctx->vae_runner  = result.vae_runner;
    ctx->t5_embedder = result.t5_embedder;
    ctx->clip_runner = result.clip_runner;
    ctx->model_type  = result.model_type;
    ctx->model_version = result.model_version;

    // 后续初始化逻辑...
    *out_ctx = ctx.release();
    return WAN_SUCCESS;
}
```

- [ ] **Step 2: 提交**

```bash
git add src/api/wan-api.cpp
git commit -m "feat: update wan_load_model() to accept config file path"
```

---

### Task 4: 更新 CLI 参数处理

**Files:**
- Modify: `examples/cli/main.cpp`

**Description:** 移除 `-m` 参数，改为位置参数接收配置文件路径。

- [ ] **Step 1: 修改 cli_options_t 结构体**

在 `examples/cli/main.cpp` 中找到 `cli_options_t` 结构体，修改：

```cpp
typedef struct {
    /* Required options */
    char* config_path;             // Path to config.json file (changed from model_path)
    char* prompt;                  // Text prompt for generation

    /* Optional options */
    char* input_image;             // Input image for I2V mode
    char* output_path;             // Output video path
    char* backend;                 // Backend type (cpu, cuda, metal, vulkan)
    char* negative_prompt;         // Negative prompt
    char* vocab_dir;               // Vocab directory (required for WAN_EMBED_VOCAB=OFF builds)
    char* gpu_ids;                 // Comma-separated GPU IDs (e.g., "0,1,2")

    /* Parameters */
    int threads;                   // Number of threads (0 = auto)
    int width;                    // Output width
    int height;                   // Output height
    int num_frames;               // Number of frames
    int fps;                      // Frames per second
    int steps;                    // Sampling steps
    int seed;                     // Random seed
    int num_gpus;                 // Number of GPUs to use (0 = auto-detect)
    float cfg;                    // Classifier-free guidance scale

    /* Flags */
    int verbose;                   // Verbose output
    int show_help;                // Show help message
    int show_version;             // Show version info

    /* Status */
    int mode;                     // 0 = none, 1 = T2V, 2 = I2V
} cli_options_t;
```

- [ ] **Step 2: 修改 print_usage() 函数**

```cpp
static void print_usage(const char* program_name) {
    printf("wan-cpp CLI - Video generation using Wan models (WAN2.1, WAN2.2)\n\n");
    printf("Usage: %s <config_json> [options]\n\n", program_name);
    printf("Required Arguments:\n");
    printf("  <config_json>              Path to config.json file\n\n");
    printf("Required Options:\n");
    printf("  -p, --prompt <text>        Text prompt for generation\n\n");
    printf("Optional Options:\n");
    printf("  -i, --input <path>         Input image for I2V mode\n");
    printf("  -o, --output <path>        Output video path (default: output.avi)\n");
    printf("  -b, --backend <type>       Backend: cpu, cuda, metal, vulkan (overrides config)\n");
    printf("  --vocab-dir <path>         Vocab files directory (required for WAN_EMBED_VOCAB=OFF builds)\n");
    printf("  -t, --threads <num>        Number of threads (overrides config, 0 = auto)\n");
    printf("  --gpu-ids <ids>            Comma-separated GPU IDs (overrides config)\n");
    printf("  --num-gpus <num>           Number of GPUs to use (overrides config)\n");
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
    printf("  %s config.json -p \"A cat playing\" -o video.avi\n\n", program_name);
    printf("  # Image-to-Video generation\n");
    printf("  %s config.json -i frame.jpg -p \"Make it move\" -o output.avi\n\n", program_name);
    printf("  # High quality generation\n");
    printf("  %s config.json -p \"A sunset beach\" -s 50 --cfg 12.0 -W 1024 -H 576\n\n", program_name);
}
```

- [ ] **Step 3: 修改 init_options() 函数**

```cpp
static void init_options(cli_options_t* opts) {
    opts->config_path = NULL;      // 改为 config_path
    opts->prompt = NULL;
    opts->input_image = NULL;
    opts->output_path = NULL;
    opts->backend = NULL;
    opts->negative_prompt = NULL;
    opts->vocab_dir = NULL;
    opts->gpu_ids = NULL;

    opts->threads = DEFAULT_THREADS;
    opts->width = DEFAULT_WIDTH;
    opts->height = DEFAULT_HEIGHT;
    opts->num_frames = DEFAULT_FRAMES;
    opts->fps = DEFAULT_FPS;
    opts->steps = DEFAULT_STEPS;
    opts->seed = DEFAULT_SEED;
    opts->num_gpus = 0;
    opts->cfg = DEFAULT_CFG;

    opts->verbose = 0;
    opts->show_help = 0;
    opts->show_version = 0;

    opts->mode = 0;
}
```

- [ ] **Step 4: 修改 parse_args() 函数处理位置参数**

在 `parse_args()` 函数中，需要：
1. 遍历 argc/argv
2. 检测非选项参数（不以 `-` 开头的）作为位置参数
3. 首个位置参数作为 `config_path`
4. 移除对 `-m` / `--model` 的处理

```cpp
static int parse_args(cli_options_t* opts, int argc, char** argv) {
    int positional_count = 0;

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        // Check if this is a positional argument (doesn't start with -)
        if (arg[0] != '-') {
            if (positional_count == 0) {
                opts->config_path = (char*)arg;
                positional_count++;
            } else {
                // Extra positional argument (ignore or warn?)
                fprintf(stderr, "Warning: unexpected positional argument '%s'\n", arg);
            }
            continue;
        }

        // Handle options
        if (strcmp(arg, "-p") == 0 || strcmp(arg, "--prompt") == 0) {
            if (i + 1 >= argc) return 1;
            opts->prompt = argv[++i];
        } else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--input") == 0) {
            if (i + 1 >= argc) return 1;
            opts->input_image = argv[++i];
        } else if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) {
            if (i + 1 >= argc) return 1;
            opts->output_path = argv[++i];
        } else if (strcmp(arg, "-b") == 0 || strcmp(arg, "--backend") == 0) {
            if (i + 1 >= argc) return 1;
            opts->backend = argv[++i];
        } else if (strcmp(arg, "--vocab-dir") == 0) {
            if (i + 1 >= argc) return 1;
            opts->vocab_dir = argv[++i];
        } else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--threads") == 0) {
            if (i + 1 >= argc) return 1;
            opts->threads = atoi(argv[++i]);
        } else if (strcmp(arg, "--gpu-ids") == 0) {
            if (i + 1 >= argc) return 1;
            opts->gpu_ids = argv[++i];
        } else if (strcmp(arg, "--num-gpus") == 0) {
            if (i + 1 >= argc) return 1;
            opts->num_gpus = atoi(argv[++i]);
        } else if (strcmp(arg, "-W") == 0 || strcmp(arg, "--width") == 0) {
            if (i + 1 >= argc) return 1;
            opts->width = atoi(argv[++i]);
        } else if (strcmp(arg, "-H") == 0 || strcmp(arg, "--height") == 0) {
            if (i + 1 >= argc) return 1;
            opts->height = atoi(argv[++i]);
        } else if (strcmp(arg, "-f") == 0 || strcmp(arg, "--frames") == 0) {
            if (i + 1 >= argc) return 1;
            opts->num_frames = atoi(argv[++i]);
        } else if (strcmp(arg, "--fps") == 0) {
            if (i + 1 >= argc) return 1;
            opts->fps = atoi(argv[++i]);
        } else if (strcmp(arg, "-s") == 0 || strcmp(arg, "--steps") == 0) {
            if (i + 1 >= argc) return 1;
            opts->steps = atoi(argv[++i]);
        } else if (strcmp(arg, "--seed") == 0) {
            if (i + 1 >= argc) return 1;
            opts->seed = atoi(argv[++i]);
        } else if (strcmp(arg, "--cfg") == 0) {
            if (i + 1 >= argc) return 1;
            opts->cfg = (float)atof(argv[++i]);
        } else if (strcmp(arg, "-n") == 0 || strcmp(arg, "--negative") == 0) {
            if (i + 1 >= argc) return 1;
            opts->negative_prompt = argv[++i];
        } else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
            opts->verbose = 1;
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            opts->show_help = 1;
        } else if (strcmp(arg, "--version") == 0) {
            opts->show_version = 1;
        } else {
            fprintf(stderr, "Unknown option: %s\n", arg);
            return 1;
        }
    }

    return 0;
}
```

- [ ] **Step 5: 修改 main() 函数的验证逻辑**

在 `main()` 函数中，改为检查 `config_path` 而非 `model_path`：

```cpp
int main(int argc, char** argv) {
    cli_options_t opts;

    /* Parse arguments */
    if (parse_args(&opts, argc, argv) != 0) {
        fprintf(stderr, "Use --help for usage information.\n");
        return 1;
    }

    /* Show help and exit */
    if (opts.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    /* Show version and exit */
    if (opts.show_version) {
        printf("wan-cpp CLI v1.0.0\n");
        return 0;
    }

    /* Validate required arguments */
    if (!opts.config_path) {
        fprintf(stderr, "Error: config.json path is required\n");
        fprintf(stderr, "Use --help for usage information.\n");
        return 1;
    }

    if (!opts.prompt) {
        fprintf(stderr, "Error: -p/--prompt is required\n");
        fprintf(stderr, "Use --help for usage information.\n");
        return 1;
    }

    // 后续逻辑改为使用 opts.config_path 而非 opts.model_path
    // wan_error_t err = wan_load_model(opts.config_path, opts.threads, opts.backend, &ctx);
    // ...
}
```

- [ ] **Step 6: 编译并测试**

```bash
cd /data/zhongwang2/jtzhuang/projects/OnlyWan/build
cmake .. && make
# 测试帮助信息
./examples/cli/wan-cli --help
```

预期输出：显示更新后的帮助信息，不包含 `-m` 参数。

- [ ] **Step 7: 提交**

```bash
git add examples/cli/main.cpp
git commit -m "feat: update CLI to use config file as positional argument, remove -m"
```

---

### Task 5: 编译并验证

**Files:**
- None (integration test)

**Description:** 完整编译并运行集成测试。

- [ ] **Step 1: 清除并重新构建**

```bash
cd /data/zhongwang2/jtzhuang/projects/OnlyWan
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

预期输出：编译成功，无错误。

- [ ] **Step 2: 创建测试配置文件**

创建 `tests/fixtures/test_config.json`：
```json
{
  "backend": "cpu",
  "n_threads": 1,
  "models": {
    "transformer_path": "tests/fixtures/transformer_dummy.gguf",
    "vae_path": "tests/fixtures/vae_dummy.gguf",
    "text_encoder_path": "tests/fixtures/text_encoder_dummy.gguf"
  },
  "wan_config_file": "tests/fixtures/wan_config_dummy.json"
}
```

创建 `tests/fixtures/wan_config_dummy.json`：
```json
{
  "model_type": "t2v",
  "dim": 3072,
  "num_heads": 24,
  "num_layers": 30,
  "in_dim": 48,
  "out_dim": 48,
  "text_len": 512,
  "eps": 1e-06,
  "ffn_dim": 14336,
  "freq_dim": 256
}
```

- [ ] **Step 3: 运行单元测试（如有）**

```bash
cd /data/zhongwang2/jtzhuang/projects/OnlyWan/build
ctest --verbose
```

- [ ] **Step 4: 手动验证 CLI 帮助**

```bash
./examples/cli/wan-cli --help
```

预期输出包含：
- `Usage: ./wan-cli <config_json> [options]`
- 不包含 `-m` 或 `--model` 参数

- [ ] **Step 5: 提交**

```bash
git add tests/fixtures/test_config.json tests/fixtures/wan_config_dummy.json
git commit -m "test: add test configuration files"
```

---

### Task 6: 文档更新与验收

**Files:**
- Modify: `include/wan-cpp/wan.h`
- Create: `docs/CONFIG.md`

**Description:** 更新 API 文档，编写配置文件使用指南。

- [ ] **Step 1: 更新 include/wan-cpp/wan.h 中的函数文档**

在该文件中找到 `wan_load_model()` 函数声明（使用 `grep "wan_load_model" include/wan-cpp/wan.h`），修改其上方的注释为：

```cpp
/**
 * Load a Wan model from a configuration file
 *
 * The config file is a JSON file with the following structure:
 * {
 *   "backend": "cpu|cuda|metal|vulkan",
 *   "n_threads": <int>,
 *   "gpu_ids": [<int>, ...],
 *   "models": {
 *     "transformer_path": "<path>",
 *     "vae_path": "<path>",
 *     "text_encoder_path": "<path>",
 *     "clip_path": "<path>"  // optional
 *   },
 *   "wan_config_file": "<path to wan_config.json>"
 * }
 *
 * The wan_config.json file contains model architecture parameters:
 * {
 *   "model_type": "t2v|i2v|ti2v",
 *   "dim": <int>,
 *   "num_heads": <int>,
 *   ...
 * }
 *
 * @param config_path Path to config.json file
 * @param n_threads Number of threads (0 = auto, overrides config file)
 * @param backend_type Backend type (overrides config file)
 * @param out_ctx Output context pointer
 * @return WAN_SUCCESS on success, WAN_ERROR_* on failure
 */
WAN_API wan_error_t wan_load_model(const char* config_path,
                                   int n_threads,
                                   const char* backend_type,
                                   wan_context_t** out_ctx);
```

- [ ] **Step 2: 创建 CONFIG.md 文档**

```markdown
# WanModel 配置文件指南

## 概述
从 v1.x 开始，WanModel 采用基于配置文件的加载方式，支持灵活的模型路径配置和运行环境设定。

## 配置文件结构

### 主配置文件 (config.json)
指定运行环境和各子模型的路径。

#### 完整示例
\`\`\`json
{
  "backend": "cuda",
  "n_threads": 4,
  "gpu_ids": [0, 1],
  "models": {
    "transformer_path": "models/wan-transformer.gguf",
    "vae_path": "models/wan-vae.gguf",
    "text_encoder_path": "models/wan-text-encoder.gguf",
    "clip_path": "models/wan-clip.gguf"
  },
  "wan_config_file": "models/wan_config.json"
}
\`\`\`

#### 字段说明
- **backend** (可选，默认: "cpu")
  - 计算后端: cpu, cuda, metal, vulkan
- **n_threads** (可选，默认: 0)
  - 线程数，0 表示自动检测
- **gpu_ids** (可选，仅用于 CUDA)
  - GPU 设备 ID 列表
- **models** (必须)
  - **transformer_path** (必须): Transformer/UNet 权重文件路径
  - **vae_path** (必须): VAE 权重文件路径
  - **text_encoder_path** (必须): Text Encoder (T5) 权重文件路径
  - **clip_path** (可选): CLIP Vision 权重文件路径，仅在 i2v/ti2v 模式下使用
- **wan_config_file** (必须)
  - 模型架构配置文件路径

### 模型架构配置 (wan_config.json)
定义 Wan 模型的超参数。

#### 完整示例
\`\`\`json
{
  "_class_name": "WanModel",
  "_diffusers_version": "0.33.0",
  "model_type": "ti2v",
  "dim": 3072,
  "num_heads": 24,
  "num_layers": 30,
  "in_dim": 48,
  "out_dim": 48,
  "text_len": 512,
  "eps": 1e-06,
  "ffn_dim": 14336,
  "freq_dim": 256
}
\`\`\`

#### 字段说明
- **model_type** (必须)
  - t2v: Text-to-Video
  - i2v: Image-to-Video
  - ti2v: Text/Image-to-Video
- **dim**, **num_heads**, **num_layers**, etc.
  - 模型架构参数，直接传入相应组件初始化

## CLI 使用

### 新格式
\`\`\`bash
./wan-cli config.json -p "A cat playing" -o output.avi
\`\`\`

### 覆盖配置文件参数
CLI 参数可覆盖配置文件中的设定：
\`\`\`bash
./wan-cli config.json -p "..." -b cuda -t 8 --gpu-ids 0,1,2
\`\`\`

## API 使用

\`\`\`c
wan_context_t* ctx;
wan_error_t err = wan_load_model("config.json", 4, "cuda", &ctx);
if (err != WAN_SUCCESS) {
    fprintf(stderr, "Failed to load model\n");
    return 1;
}
\`\`\`

## 错误处理

- 配置文件缺失或格式错误 → `WAN_ERROR_INVALID_ARGUMENT`
- 必需的模型文件缺失 → `WAN_ERROR_MODEL_LOAD_FAILED`
- CLIP 文件缺失 → 记录警告，继续加载（可选）
```

- [ ] **Step 3: 提交**

```bash
git add include/wan.h docs/CONFIG.md
git commit -m "docs: update API documentation and add config file guide"
```

---

## 验收标准检查清单

**核心功能：**
- [ ] `./wan-cli config.json -p "A cat"` 能成功加载模型
- [ ] 模型类型（t2v/i2v/ti2v）由 `wan_config.json` 中的 model_type 正确决定
- [ ] 支持分别指定不同路径的 Transformer、VAE、Text Encoder、CLIP
- [ ] 多 GPU 配置通过 `config.json` 中的 gpu_ids 正确传递到 context
- [ ] `-m` 参数已移除，尝试使用 `-m` 会显示 "Unknown option" 错误

**错误处理：**
- [ ] 配置文件缺失：清晰的错误消息 "Config file not found: ..."
- [ ] JSON 格式错误：清晰的错误消息 "JSON parse error in ..."
- [ ] 必需文件缺失（transformer/vae/text_encoder）：加载失败并返回 WAN_ERROR_MODEL_LOAD_FAILED
- [ ] CLIP 文件缺失（i2v/ti2v）：记录 LOG_WARN "failed to load CLIP, continuing without it"

**配置验证：**
- [ ] 验证所有必需字段存在（transformer_path, vae_path, text_encoder_path, wan_config_file）
- [ ] 验证配置文件路径存在（通过 file_exists()）

**日志输出：**
- [ ] 日志显示配置解析步骤（"ConfigLoader::load_config: parsing ..."）
- [ ] 日志显示模型加载进度（"WanModel::load: loading Transformer from ..."）
- [ ] 日志显示最终加载结果（"Model loaded successfully: ..."）

**性能：**
- [ ] 配置解析耗时 < 100ms（JSON 解析较快）
- [ ] 整体加载时间与原有方式相近（不应增加显著开销）

---

## 关键提示

1. **分离的模型文件**：每个组件（Transformer, VAE, T5, CLIP）现在可以从不同文件加载，灵活性更高。
2. **从配置读取参数**：模型类型、架构参数等从 `wan_config.json` 读取，不再依赖权重文件 metadata。
3. **向后兼容**：旧式单一 GGUF 加载方式已弃用，用户需迁移至新配置方式。
4. **错误处理**：必需组件（VAE、T5、Transformer）缺失则加载失败；CLIP 缺失仅警告。
