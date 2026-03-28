# OnlyWan 完整 API 使用指南

## 目录
1. [快速开始](#快速开始)
2. [核心概念](#核心概念)
3. [详细 API 参考](#详细-api-参考)
4. [完整推理示例](#完整推理示例)
5. [最佳实践](#最佳实践)
6. [常见问题](#常见问题)

---

## 快速开始

### 最小化示例：T2V 推理

```cpp
#include "src/model_registry.hpp"
#include "src/wan.hpp"
#include "src/clip.hpp"

int main() {
    // 1. 初始化后端
    BackendRAII backend_guard(GGML_BACKEND_CPU);

    // 2. 加载文本编码器
    auto text_encoder = ModelRegistry::instance()->create<CLIPTextModelRunner>(
        "clip-vit-l-14",
        GGML_BACKEND_CPU,
        false,
        {},
        "models/"
    );
    text_encoder->alloc_params_buffer();

    // 3. 加载 Transformer
    auto transformer = ModelRegistry::instance()->create<WanRunner>(
        "wan-runner-t2v",
        GGML_BACKEND_CPU,
        false,
        {},
        "models/"
    );
    transformer->alloc_params_buffer();

    // 4. 加载 VAE 解码器
    auto vae = ModelRegistry::instance()->create<WanVAERunner>(
        "wan-vae-t2v-decode",
        GGML_BACKEND_CPU,
        false,
        {},
        "models/"
    );
    vae->alloc_params_buffer();

    // 5. 执行推理
    // ... (详见下面的完整示例)

    return 0;
}
```

---

## 核心概念

### 1. ModelRegistry (模型注册表)

**职责**: 管理所有模型工厂，提供单例访问

**关键方法**:
```cpp
// 获取单例
ModelRegistry* registry = ModelRegistry::instance();

// 检查版本是否存在
bool exists = registry->has_version<WanRunner>("wan-runner-t2v");

// 创建模型实例
auto runner = registry->create<WanRunner>(
    "wan-runner-t2v",      // 版本字符串
    backend,               // 后端类型
    offload,               // 是否 offload
    tensor_map,            // 张量映射 (可选)
    prefix                 // 模型前缀 (可选)
);

// 获取所有注册的版本
std::vector<std::string> versions = registry->get_versions<WanRunner>();
```

**已注册的模型**:
- CLIP Vision: `clip-vision-vit-l-14`, `clip-vision-vit-h-14`, `clip-vision-vit-bigg-14`
- T5: `t5-standard`, `t5-umt5`
- WAN VAE: `wan-vae-t2v`, `wan-vae-t2v-decode`, `wan-vae-i2v`, `wan-vae-ti2v`
- WAN Transformer: `wan-runner-t2v`, `wan-runner-i2v`, `wan-runner-ti2v`

### 2. GGMLRunner (模型运行器基类)

**职责**: 定义模型推理的通用接口

**关键方法**:
```cpp
class GGMLRunner {
    // 分配参数缓冲区
    virtual void alloc_params_buffer();

    // 获取参数张量映射
    virtual std::map<std::string, struct ggml_tensor*> get_param_tensors();

    // 获取模型描述
    virtual std::string get_desc();

    // 构建计算图
    virtual struct ggml_cgraph* build_graph(
        struct ggml_tensor** inputs,
        int num_inputs,
        struct ggml_tensor** outputs,
        int num_outputs
    );

    // 执行推理
    virtual bool compute(
        struct ggml_tensor** inputs,
        int num_inputs,
        struct ggml_tensor** outputs,
        int num_outputs
    );

    // 获取运行上下文
    GGMLRunnerContext get_context();
};
```

### 3. 张量形状约定

#### T2V 路径

| 阶段 | 张量 | 形状 | 说明 |
|------|------|------|------|
| 输入 | 文本 | `[1, 77]` | Token IDs |
| 输入 | 噪声 | `[N*16, T, H, W]` | 初始噪声 |
| 文本编码 | 嵌入 | `[N, 512, 4096]` | 文本特征 |
| Transformer | 输出 | `[N*16, T, H, W]` | 潜在表示 |
| VAE 解码 | 输出 | `[N*3, T*4, H*8, W*8]` | RGB 视频帧 |

#### I2V/TI2V 路径

| 阶段 | 张量 | 形状 | 说明 |
|------|------|------|------|
| 输入 | 图像 | `[N, 3, 224, 224]` | 参考图像 |
| 图像编码 | 特征 | `[N, 257, 1280]` | CLIP 特征 |
| 特征融合 | 上下文 | `[N, 257+512, dim]` | 图像+文本 |
| Transformer | 输出 | `[N*16, T, H, W]` | 潜在表示 |
| VAE 解码 | 输出 | `[N*3, T*4, H*8, W*8]` | RGB 视频帧 |

### 4. RAII 模式

**BackendRAII**: 自动管理后端生命周期
```cpp
{
    BackendRAII backend_guard(GGML_BACKEND_CPU);
    // 使用后端
} // 自动清理
```

**GGMLCtxRAII**: 自动管理 GGML 上下文
```cpp
{
    GGMLCtxRAII ctx_guard(256 * 1024 * 1024);  // 256MB
    struct ggml_context* ctx = ctx_guard.get();
    // 使用上下文
} // 自动释放
```

**TempFile**: 自动清理临时文件
```cpp
{
    TempFile temp("output.npy");
    save_npy(temp.path(), tensor);
    // 使用文件
} // 自动删除
```

---

## 详细 API 参考

### ModelRegistry API

#### 创建模型实例

```cpp
// 基本用法
auto runner = ModelRegistry::instance()->create<WanRunner>(
    "wan-runner-t2v",
    GGML_BACKEND_CPU,
    false,
    {},
    "models/"
);

// 参数说明
// - 第1个参数: 版本字符串 (必需)
// - 第2个参数: 后端类型 (GGML_BACKEND_CPU, GGML_BACKEND_CUDA, etc.)
// - 第3个参数: 是否 offload (true/false)
// - 第4个参数: 张量映射 (String2TensorStorage, 可选)
// - 第5个参数: 模型前缀 (文件路径前缀, 可选)

// 返回值: std::unique_ptr<WanRunner>
```

#### 检查版本存在性

```cpp
if (ModelRegistry::instance()->has_version<WanRunner>("wan-runner-t2v")) {
    // 版本存在，可以创建
} else {
    // 版本不存在，处理错误
}
```

#### 获取所有版本

```cpp
auto versions = ModelRegistry::instance()->get_versions<WanRunner>();
for (const auto& version : versions) {
    std::cout << "Available version: " << version << std::endl;
}
```

### GGMLRunner API

#### 初始化模型

```cpp
// 1. 创建模型实例
auto runner = ModelRegistry::instance()->create<WanRunner>(...);

// 2. 分配参数缓冲区
runner->alloc_params_buffer();

// 3. 获取参数张量映射
auto param_tensors = runner->get_param_tensors();

// 4. 加载权重到张量
ModelLoader loader("models/wan-runner-t2v.safetensors");
for (const auto& [name, tensor] : param_tensors) {
    loader.load_tensor(name, tensor);
}
```

#### 执行推理

```cpp
// 1. 准备输入张量
struct ggml_tensor* input = ...;
struct ggml_tensor* output = ...;

// 2. 执行推理
bool success = runner->compute(
    &input,   // 输入张量数组
    1,        // 输入数量
    &output,  // 输出张量数组
    1         // 输出数量
);

if (!success) {
    std::cerr << "Inference failed" << std::endl;
}
```

#### 获取模型描述

```cpp
std::string desc = runner->get_desc();
std::cout << "Model: " << desc << std::endl;
// 输出: Model: WanRunner (T2V, 30 layers, dim=1536)
```

### I/O 工具 API

#### 加载 .npy 文件

```cpp
#include "src/test_io_utils.hpp"

// 创建上下文
GGMLCtxRAII ctx_guard(256 * 1024 * 1024);
struct ggml_context* ctx = ctx_guard.get();

// 加载文件
struct ggml_tensor* tensor = load_npy("data/input.npy", ctx);

if (tensor == nullptr) {
    std::cerr << "Failed to load tensor" << std::endl;
}

// 获取张量信息
std::cout << "Shape: ";
for (int i = 0; i < tensor->n_dims; i++) {
    std::cout << tensor->ne[i] << " ";
}
std::cout << std::endl;
```

#### 保存 .npy 文件

```cpp
// 创建张量
struct ggml_tensor* tensor = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 10, 20);

// 保存文件
bool success = save_npy("data/output.npy", tensor);

if (!success) {
    std::cerr << "Failed to save tensor" << std::endl;
}
```

#### 支持的数据类型

```cpp
// F32 (float32)
struct ggml_tensor* f32_tensor = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 10, 20);

// F16 (float16)
struct ggml_tensor* f16_tensor = ggml_new_tensor_2d(ctx, GGML_TYPE_F16, 10, 20);

// I32 (int32)
struct ggml_tensor* i32_tensor = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 10, 20);

// I64 (int64)
struct ggml_tensor* i64_tensor = ggml_new_tensor_2d(ctx, GGML_TYPE_I64, 10, 20);
```

---

## 完整推理示例

### 示例 1: T2V 完整推理流程

```cpp
#include "src/model_registry.hpp"
#include "src/wan.hpp"
#include "src/clip.hpp"
#include "src/test_io_utils.hpp"
#include <iostream>

int main() {
    try {
        // ========== 初始化 ==========
        std::cout << "Initializing backend..." << std::endl;
        BackendRAII backend_guard(GGML_BACKEND_CPU);

        // ========== 加载模型 ==========
        std::cout << "Loading models..." << std::endl;

        // 加载文本编码器
        auto text_encoder = ModelRegistry::instance()->create<CLIPTextModelRunner>(
            "clip-vit-l-14",
            GGML_BACKEND_CPU,
            false,
            {},
            "models/"
        );
        text_encoder->alloc_params_buffer();
        std::cout << "Text encoder loaded: " << text_encoder->get_desc() << std::endl;

        // 加载 Transformer
        auto transformer = ModelRegistry::instance()->create<WanRunner>(
            "wan-runner-t2v",
            GGML_BACKEND_CPU,
            false,
            {},
            "models/"
        );
        transformer->alloc_params_buffer();
        std::cout << "Transformer loaded: " << transformer->get_desc() << std::endl;

        // 加载 VAE 解码器
        auto vae = ModelRegistry::instance()->create<WanVAERunner>(
            "wan-vae-t2v-decode",
            GGML_BACKEND_CPU,
            false,
            {},
            "models/"
        );
        vae->alloc_params_buffer();
        std::cout << "VAE decoder loaded: " << vae->get_desc() << std::endl;

        // ========== 准备输入 ==========
        std::cout << "Preparing inputs..." << std::endl;

        // 创建 GGML 上下文
        GGMLCtxRAII ctx_guard(512 * 1024 * 1024);  // 512MB
        struct ggml_context* ctx = ctx_guard.get();

        // 文本提示
        std::string prompt = "a cat running in the park";
        std::cout << "Prompt: " << prompt << std::endl;

        // 生成参数
        int batch_size = 1;
        int num_frames = 25;
        int height = 512;
        int width = 512;

        // ========== 文本编码 ==========
        std::cout << "Encoding text..." << std::endl;

        // 这里应该调用 tokenizer 和文本编码器
        // 输出: text_embedding [batch_size, 512, 4096]
        struct ggml_tensor* text_embedding = ggml_new_tensor_3d(
            ctx, GGML_TYPE_F32,
            4096,  // 特征维度
            512,   // 序列长度
            batch_size
        );

        // ========== 噪声初始化 ==========
        std::cout << "Initializing noise..." << std::endl;

        // 噪声张量: [batch_size*16, num_frames, height, width]
        struct ggml_tensor* noise = ggml_new_tensor_4d(
            ctx, GGML_TYPE_F32,
            width,
            height,
            num_frames,
            batch_size * 16
        );

        // ========== Transformer 推理 ==========
        std::cout << "Running Transformer inference..." << std::endl;

        struct ggml_tensor* latent = ggml_new_tensor_4d(
            ctx, GGML_TYPE_F32,
            width,
            height,
            num_frames,
            batch_size * 16
        );

        bool success = transformer->compute(
            &noise,
            1,
            &latent,
            1
        );

        if (!success) {
            std::cerr << "Transformer inference failed" << std::endl;
            return 1;
        }

        std::cout << "Transformer inference completed" << std::endl;

        // ========== VAE 解码 ==========
        std::cout << "Running VAE decoding..." << std::endl;

        // 输出视频帧: [batch_size*3, num_frames*4, height*8, width*8]
        struct ggml_tensor* video_frames = ggml_new_tensor_4d(
            ctx, GGML_TYPE_F32,
            width * 8,
            height * 8,
            num_frames * 4,
            batch_size * 3
        );

        success = vae->compute(
            &latent,
            1,
            &video_frames,
            1
        );

        if (!success) {
            std::cerr << "VAE decoding failed" << std::endl;
            return 1;
        }

        std::cout << "VAE decoding completed" << std::endl;

        // ========== 保存结果 ==========
        std::cout << "Saving results..." << std::endl;

        save_npy("output/video_frames.npy", video_frames);
        std::cout << "Video frames saved to output/video_frames.npy" << std::endl;

        std::cout << "Inference completed successfully!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

### 示例 2: I2V 推理流程

```cpp
#include "src/model_registry.hpp"
#include "src/wan.hpp"
#include "src/clip.hpp"
#include "src/test_io_utils.hpp"

int main() {
    try {
        BackendRAII backend_guard(GGML_BACKEND_CPU);

        // 加载模型
        auto image_encoder = ModelRegistry::instance()->create<CLIPVisionModelProjectionRunner>(
            "clip-vision-vit-l-14",
            GGML_BACKEND_CPU,
            false,
            {},
            "models/"
        );
        image_encoder->alloc_params_buffer();

        auto text_encoder = ModelRegistry::instance()->create<CLIPTextModelRunner>(
            "clip-vit-l-14",
            GGML_BACKEND_CPU,
            false,
            {},
            "models/"
        );
        text_encoder->alloc_params_buffer();

        auto transformer = ModelRegistry::instance()->create<WanRunner>(
            "wan-runner-i2v",  // I2V 版本
            GGML_BACKEND_CPU,
            false,
            {},
            "models/"
        );
        transformer->alloc_params_buffer();

        auto vae = ModelRegistry::instance()->create<WanVAERunner>(
            "wan-vae-i2v",  // I2V 版本
            GGML_BACKEND_CPU,
            false,
            {},
            "models/"
        );
        vae->alloc_params_buffer();

        // 创建上下文
        GGMLCtxRAII ctx_guard(512 * 1024 * 1024);
        struct ggml_context* ctx = ctx_guard.get();

        // 加载参考图像
        struct ggml_tensor* image = load_npy("input/reference_image.npy", ctx);

        // 编码图像
        struct ggml_tensor* image_features = ggml_new_tensor_3d(
            ctx, GGML_TYPE_F32,
            1280,  // CLIP 特征维度
            257,   // 257 tokens (1 CLS + 256 patches)
            1      // batch_size
        );

        image_encoder->compute(&image, 1, &image_features, 1);

        // 编码文本
        std::string prompt = "the cat starts running";
        struct ggml_tensor* text_features = ggml_new_tensor_3d(
            ctx, GGML_TYPE_F32,
            4096,  // 文本特征维度
            512,   // 序列长度
            1      // batch_size
        );

        text_encoder->compute(/* 文本输入 */, 1, &text_features, 1);

        // 融合特征
        struct ggml_tensor* fused_context = ggml_new_tensor_3d(
            ctx, GGML_TYPE_F32,
            4096,
            257 + 512,  // 图像 + 文本
            1
        );

        // 初始化噪声
        struct ggml_tensor* noise = ggml_new_tensor_4d(
            ctx, GGML_TYPE_F32,
            512, 512, 25, 16
        );

        // Transformer 推理
        struct ggml_tensor* latent = ggml_new_tensor_4d(
            ctx, GGML_TYPE_F32,
            512, 512, 25, 16
        );

        transformer->compute(&noise, 1, &latent, 1);

        // VAE 解码
        struct ggml_tensor* video = ggml_new_tensor_4d(
            ctx, GGML_TYPE_F32,
            4096, 4096, 100, 3
        );

        vae->compute(&latent, 1, &video, 1);

        // 保存结果
        save_npy("output/i2v_video.npy", video);

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

---

## 最佳实践

### 1. 内存管理

```cpp
// ✅ 好的做法: 使用 RAII
{
    GGMLCtxRAII ctx_guard(256 * 1024 * 1024);
    struct ggml_context* ctx = ctx_guard.get();
    // 使用上下文
} // 自动释放

// ❌ 不好的做法: 手动管理
struct ggml_context* ctx = ggml_init(params);
// ... 使用
ggml_free(ctx);  // 容易忘记或异常时泄漏
```

### 2. 错误处理

```cpp
// ✅ 好的做法: 检查返回值
auto runner = ModelRegistry::instance()->create<WanRunner>(...);
if (!runner) {
    std::cerr << "Failed to create runner" << std::endl;
    return 1;
}

bool success = runner->compute(...);
if (!success) {
    std::cerr << "Inference failed" << std::endl;
    return 1;
}

// ❌ 不好的做法: 忽略错误
auto runner = ModelRegistry::instance()->create<WanRunner>(...);
runner->compute(...);  // 可能失败但不检查
```

### 3. 张量形状验证

```cpp
// ✅ 好的做法: 验证张量形状
if (tensor->n_dims != 4) {
    std::cerr << "Expected 4D tensor, got " << tensor->n_dims << "D" << std::endl;
    return 1;
}

if (tensor->ne[0] != expected_width || tensor->ne[1] != expected_height) {
    std::cerr << "Shape mismatch" << std::endl;
    return 1;
}

// ❌ 不好的做法: 假设形状正确
struct ggml_tensor* output = ggml_new_tensor_4d(ctx, GGML_TYPE_F32, ...);
// 直接使用，可能导致内存访问错误
```

### 4. 模型版本检查

```cpp
// ✅ 好的做法: 检查版本存在性
if (!ModelRegistry::instance()->has_version<WanRunner>("wan-runner-t2v")) {
    std::cerr << "Model version not found" << std::endl;
    return 1;
}

auto runner = ModelRegistry::instance()->create<WanRunner>(...);

// ❌ 不好的做法: 直接创建，可能返回 nullptr
auto runner = ModelRegistry::instance()->create<WanRunner>(...);
runner->alloc_params_buffer();  // 可能崩溃
```

### 5. 资源清理

```cpp
// ✅ 好的做法: 使用 unique_ptr 自动管理
{
    auto runner = ModelRegistry::instance()->create<WanRunner>(...);
    runner->alloc_params_buffer();
    // 使用
} // 自动释放

// ❌ 不好的做法: 手动管理
WanRunner* runner = new WanRunner(...);
runner->alloc_params_buffer();
// ... 使用
delete runner;  // 容易忘记
```

---

## 常见问题

### Q1: 如何选择合适的后端？

**A**: 根据硬件选择：
- **CPU**: `GGML_BACKEND_CPU` - 通用，但速度慢
- **CUDA**: `GGML_BACKEND_CUDA` - NVIDIA GPU，速度快
- **Metal**: `GGML_BACKEND_METAL` - Apple GPU
- **Vulkan**: `GGML_BACKEND_VULKAN` - 跨平台 GPU

```cpp
// 检测可用的后端
ggml_backend_t backend = ggml_backend_cuda_init();
if (backend == nullptr) {
    backend = ggml_backend_cpu_init();
}
```

### Q2: 如何处理内存不足？

**A**: 几种解决方案：
1. 增加 `ggml_init_params` 的大小
2. 使用 offload 功能
3. 分批处理
4. 使用更小的模型

```cpp
// 增加内存
struct ggml_init_params params = {
    .mem_size = 1024 * 1024 * 1024,  // 1GB
    .mem_buffer = nullptr,
    .no_alloc = false,
};

// 使用 offload
auto runner = ModelRegistry::instance()->create<WanRunner>(
    "wan-runner-t2v",
    backend,
    true,  // 启用 offload
    {},
    "models/"
);
```

### Q3: 如何加载自定义权重？

**A**: 使用 `ModelLoader` 或直接操作张量：

```cpp
// 方法 1: 使用 ModelLoader
ModelLoader loader("path/to/weights.safetensors");
auto param_tensors = runner->get_param_tensors();
for (const auto& [name, tensor] : param_tensors) {
    loader.load_tensor(name, tensor);
}

// 方法 2: 直接加载 .npy 文件
struct ggml_tensor* weights = load_npy("weights.npy", ctx);
// 复制到参数张量
ggml_backend_tensor_set(param_tensor, weights->data, 0, ggml_nbytes(weights));
```

### Q4: 如何调试推理过程？

**A**: 使用日志和中间张量保存：

```cpp
// 保存中间结果
struct ggml_tensor* intermediate = ...;
save_npy("debug/intermediate.npy", intermediate);

// 检查张量统计
float min_val = FLT_MAX, max_val = FLT_MIN;
float* data = (float*)intermediate->data;
for (int i = 0; i < ggml_nelements(intermediate); i++) {
    min_val = std::min(min_val, data[i]);
    max_val = std::max(max_val, data[i]);
}
std::cout << "Min: " << min_val << ", Max: " << max_val << std::endl;
```

### Q5: 支持哪些数据类型？

**A**: 支持以下数据类型：
- `GGML_TYPE_F32` - 32 位浮点数
- `GGML_TYPE_F16` - 16 位浮点数
- `GGML_TYPE_I32` - 32 位整数
- `GGML_TYPE_I64` - 64 位整数

```cpp
// 创建不同类型的张量
struct ggml_tensor* f32_tensor = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 10, 20);
struct ggml_tensor* f16_tensor = ggml_new_tensor_2d(ctx, GGML_TYPE_F16, 10, 20);
struct ggml_tensor* i32_tensor = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 10, 20);
```

### Q6: 如何优化推理速度？

**A**: 几种优化策略：
1. 使用 GPU 后端
2. 启用位置编码缓存
3. 使用融合操作
4. 减少张量复制

```cpp
// 使用 GPU
auto runner = ModelRegistry::instance()->create<WanRunner>(
    "wan-runner-t2v",
    GGML_BACKEND_CUDA,  // 使用 CUDA
    false,
    {},
    "models/"
);

// 位置编码缓存已自动启用 (OP-02 优化)
```

---

## 参考资源

- [API 调用文档](./API_CALLS.md)
- [API 流程架构](./API_FLOW_ARCHITECTURE.md)
- [API 流程图](./API_FLOW_DIAGRAMS.md)
- [模型注册表使用示例](./examples/model_registry_usage.cpp)
- [I/O 工具使用示例](./examples/io_utils_usage.cpp)
- [测试文件](../tests/cpp/)
