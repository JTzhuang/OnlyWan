# OnlyWan API 调用流程图与架构文档

## 目录
1. [系统架构概览](#系统架构概览)
2. [API 调用流程](#api-调用流程)
3. [模块关系图](#模块关系图)
4. [数据流向](#数据流向)
5. [核心 API 接口](#核心-api-接口)
6. [使用示例](#使用示例)

---

## 系统架构概览

### 分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                    应用层 (Application)                      │
│              用户代码 / 推理引擎 / 服务                       │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│              模型注册与工厂层 (Registry & Factory)            │
│  ModelRegistry (单例) → 模型工厂 → 模型实例创建              │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│              模型运行器层 (Model Runner Layer)               │
│  ┌──────────────┬──────────────┬──────────────┐             │
│  │ WanRunner    │ WanVAERunner │ CLIPRunner   │ T5Runner    │
│  │ (Transformer)│ (VAE Codec)  │ (Encoders)   │ (Encoder)   │
│  └──────────────┴──────────────┴──────────────┘             │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│           组件/块层 (Block/Component Layer)                  │
│  ┌─────────────┬──────────────┬──────────────┐              │
│  │ Attention   │ Conv/Linear  │ Normalization│ Activation   │
│  │ Blocks      │ Layers       │ Layers       │ Functions    │
│  └─────────────┴──────────────┴──────────────┘              │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│         GGML 扩展层 (GGML Extension Layer)                   │
│  ggml_extend.hpp - GGML 操作包装与优化                       │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│              底层计算库 (GGML Backend)                       │
│  CPU / GPU (CUDA/Metal) 张量计算                            │
└─────────────────────────────────────────────────────────────┘
```

### 核心模块

| 模块 | 文件 | 职责 |
|------|------|------|
| **模型注册表** | `src/model_registry.hpp/cpp` | 管理所有模型工厂，提供单例访问 |
| **模型工厂** | `src/model_factory.hpp/cpp` | 注册 12 个模型变体，创建模型实例 |
| **WAN Transformer** | `src/wan.hpp` | T2V/I2V/TI2V 推理核心 (DiT) |
| **WAN VAE** | `src/wan.hpp` | 视频编码/解码 |
| **CLIP** | `src/clip.hpp` | 文本/图像编码器 |
| **T5** | `src/t5.hpp` | 文本编码器 |
| **通用块** | `src/common_block.hpp` | Conv2d, Linear, LayerNorm 等 |
| **GGML 扩展** | `src/ggml_extend.hpp` | GGML 操作包装 |

---

## API 调用流程

### 1. 模型加载流程

```
┌─────────────────────────────────────────────────────────────┐
│ 应用代码                                                     │
│ ModelRegistry::instance()->create<WanRunner>(                │
│   "wan-runner-t2v", backend, offload, tensor_map, prefix)   │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────┐
│ ModelRegistry::create<T>()                                   │
│ - 查找注册的工厂函数                                         │
│ - 验证版本字符串存在                                         │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────┐
│ 工厂 Lambda 函数 (model_factory.cpp)                         │
│ - 创建 std::make_unique<WanRunner>(...)                      │
│ - 调用 runner->init()                                        │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────┐
│ WanRunner::init()                                            │
│ - 初始化 Wan 模型实例                                        │
│ - 调用 GGMLBlock::init_params()                              │
│ - 为所有参数分配张量                                         │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────┐
│ 返回 std::unique_ptr<WanRunner>                              │
│                                                              │
│ 应用代码:                                                    │
│ runner->alloc_params_buffer()  // 分配参数缓冲区             │
│ runner->get_param_tensors()    // 获取参数张量映射           │
│ ModelLoader::load(...)         // 加载权重                   │
└─────────────────────────────────────────────────────────────┘
```

### 2. T2V (文本到视频) 推理流程

```
┌──────────────────────────────────────────────────────────────┐
│ 输入准备                                                      │
│ - 文本提示: "a cat running"                                   │
│ - 生成参数: num_frames=25, height=512, width=512             │
└────────────────┬─────────────────────────────────────────────┘
                 │
    ┌────────────┴────────────┐
    │                         │
    ▼                         ▼
┌──────────────────┐  ┌──────────────────┐
│ 文本编码阶段      │  │ 噪声初始化        │
│                  │  │                  │
│ 文本提示          │  │ x ~ N(0, 1)      │
│ ↓                │  │ shape: [N*16,    │
│ Tokenize         │  │        T, H, W]  │
│ ↓                │  │                  │
│ Token IDs        │  │ N=batch_size     │
│ [1, 77]          │  │ T=时间帧数       │
│ ↓                │  │ H,W=空间分辨率   │
│ CLIPTextModel    │  │                  │
│ 或 T5Model       │  │                  │
│ ↓                │  │                  │
│ 文本嵌入          │  │                  │
│ [N, 512, 4096]   │  │                  │
└────────┬─────────┘  └────────┬─────────┘
         │                     │
         └──────────┬──────────┘
                    │
                    ▼
┌──────────────────────────────────────────────────────────────┐
│ Transformer DiT 推理 (WanRunner::compute)                    │
│                                                              │
│ 1. 输入准备:                                                 │
│    - Patch embedding: x → [N, t_len*h_len*w_len, dim]       │
│    - Time embedding: timestep → [N, dim]                    │
│    - Text context: [N, 512, 4096]                           │
│    - Position encoding (RoPE)                               │
│                                                              │
│ 2. 30-40 层 Transformer Blocks:                              │
│    ┌─────────────────────────────────────────┐              │
│    │ WanAttentionBlock::forward()             │              │
│    │ ├─ Self-Attention (with RoPE)           │              │
│    │ ├─ Cross-Attention (with text context)  │              │
│    │ └─ FFN (Linear + GELU + Linear)         │              │
│    └─────────────────────────────────────────┘              │
│                                                              │
│ 3. Head 输出:                                                │
│    - [N, t_len*h_len*w_len, 16*1*2*2]                       │
│                                                              │
│ 4. Unpatchify:                                               │
│    - [N*16, T, H, W]                                         │
└────────────────┬─────────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────────┐
│ VAE 解码阶段 (WanVAERunner::compute)                         │
│                                                              │
│ 潜在表示 z: [N*16, T, H, W]                                  │
│ ↓                                                            │
│ Conv2d (z → [N*16, T, H, W])                                │
│ ↓                                                            │
│ Decoder3d::forward() (逐帧处理):                             │
│ ├─ CausalConv3d                                             │
│ ├─ Middle Blocks (ResidualBlock + AttentionBlock)           │
│ ├─ Upsamples (Up_ResidualBlock with Resample)               │
│ └─ Head (RMSNorm + SiLU + CausalConv3d)                     │
│ ↓                                                            │
│ 视频帧: [N*3, T*4, H*8, W*8]                                │
│ (RGB 格式，分辨率提升 8 倍)                                  │
└────────────────┬─────────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────────┐
│ 输出处理                                                      │
│ - 视频帧保存为 AVI/MP4                                        │
│ - 或返回张量供后续处理                                        │
└──────────────────────────────────────────────────────────────┘
```

### 3. I2V/TI2V (图像到视频) 流程差异

```
┌──────────────────────────────────────────────────────────────┐
│ 额外输入: 参考图像                                            │
│ [N, 3, 224, 224]                                             │
└────────────────┬─────────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────────┐
│ CLIP Vision 编码                                              │
│ CLIPVisionModelProjectionRunner::forward()                   │
│ ↓                                                            │
│ 图像特征: [N, 257, 1280]                                     │
│ (257 = 1 CLS token + 256 patch tokens)                       │
└────────────────┬─────────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────────┐
│ 特征投影                                                      │
│ MLPProj::forward()                                            │
│ ↓                                                            │
│ [N, 257, dim]                                                │
└────────────────┬─────────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────────┐
│ 与文本上下文拼接                                              │
│ context = [image_features, text_features]                   │
│ shape: [N, 257+512, dim]                                     │
└────────────────┬─────────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────────┐
│ Transformer DiT 推理 (使用 I2V/TI2V Cross-Attention)        │
│ - WanI2VCrossAttention (双路 attention)                      │
│ - 同时处理图像和文本上下文                                    │
└──────────────────────────────────────────────────────────────┘
```

---

## 模块关系图

### 类继承关系

```
GGMLRunner (抽象基类)
├─ WanRunner
│  └─ Wan (包含 30-40 个 WanAttentionBlock)
├─ WanVAERunner
│  └─ WanVAE (包含 Encoder3d/Decoder3d)
├─ CLIPTextModelRunner
│  └─ CLIPTextModel
├─ CLIPVisionModelProjectionRunner
│  └─ CLIPVisionModel
└─ T5Runner
   └─ T5

GGMLBlock (组件基类)
├─ Conv2d, Conv3d, CausalConv3d
├─ Linear, LayerNorm, RMSNorm, GroupNorm32
├─ WanAttentionBlock
│  ├─ WanSelfAttention (RoPE)
│  ├─ WanT2VCrossAttention 或 WanI2VCrossAttention
│  └─ FFN (Linear + GELU + Linear)
├─ ResidualBlock, Down_ResidualBlock, Up_ResidualBlock
├─ Encoder3d, Decoder3d
├─ Head, MLPProj
└─ ...其他块
```

### 模型注册关系

```
ModelRegistry (单例)
├─ CLIP Vision (3 个变体)
│  ├─ "clip-vision-vit-l-14"
│  ├─ "clip-vision-vit-h-14"
│  └─ "clip-vision-vit-bigg-14"
├─ T5 (2 个变体)
│  ├─ "t5-standard"
│  └─ "t5-umt5"
├─ WAN VAE (4 个变体)
│  ├─ "wan-vae-t2v"
│  ├─ "wan-vae-t2v-decode"
│  ├─ "wan-vae-i2v"
│  └─ "wan-vae-ti2v"
└─ WAN Transformer (3 个变体)
   ├─ "wan-runner-t2v"
   ├─ "wan-runner-i2v"
   └─ "wan-runner-ti2v"
```

---

## 数据流向

### 张量形状变换链

#### T2V 路径

```
文本输入:
"a cat running"
  → Tokenize → [1, 77] token IDs
  → Embedding → [1, 77, 768]
  → CLIPTextModel/T5 → [1, 512, 4096] (投影后)

噪声初始化:
x ~ N(0, 1) → [N*16, T, H, W]
  (N=batch_size, T=时间帧数, H=高度, W=宽度)

Transformer 处理:
[N*16, T, H, W]
  → Patch embedding → [N, t_len*h_len*w_len, 1536/3072/5120]
  → 30-40 层 Transformer blocks
  → Head output → [N, t_len*h_len*w_len, 16*1*2*2]
  → Unpatchify → [N*16, T, H, W]

VAE 解码:
[N*16, T, H, W]
  → Conv2d → [N*16, T, H, W]
  → Decoder3d → [N*3, T*4, H*8, W*8]
  (RGB 视频帧，分辨率提升 8 倍)
```

#### I2V/TI2V 路径

```
图像输入:
[N, 3, 224, 224]
  → CLIPVisionModel → [N, 257, 1280]
  → MLPProj → [N, 257, dim]

文本输入:
(同 T2V)
  → [1, 512, 4096]

上下文拼接:
[image_features, text_features]
  → [N, 257+512, dim]

Transformer 处理:
(使用 I2V/TI2V Cross-Attention)
  → [N*16, T, H, W]

VAE 解码:
(同 T2V)
  → [N*3, T*4, H*8, W*8]
```

### 状态管理

| 状态 | 存储位置 | 用途 |
|------|---------|------|
| **计算图** | `GGMLRunner::graph` | 每次推理步骤重新构建 |
| **位置编码缓存** | `WanRunner::pe_cached` | OP-02 优化，避免重复计算 |
| **特征缓存** | `WanVAE::_feat_map` | VAE 因果卷积的中间特征 |
| **后端管理** | `GGMLRunnerContext` | CPU/GPU 张量自动 offload |

---

## 核心 API 接口

### 1. ModelRegistry API

```cpp
// 获取单例
ModelRegistry* registry = ModelRegistry::instance();

// 检查模型版本是否存在
bool exists = registry->has_version<WanRunner>("wan-runner-t2v");

// 创建模型实例
auto runner = registry->create<WanRunner>(
    "wan-runner-t2v",           // 版本字符串
    backend,                     // 后端 (CPU/GPU)
    offload,                     // 是否 offload
    tensor_map,                  // 张量映射 (可选)
    prefix                       // 模型前缀 (可选)
);

// 获取所有注册的版本
std::vector<std::string> versions = registry->get_versions<WanRunner>();
```

### 2. 模型运行器 API

```cpp
// 分配参数缓冲区
runner->alloc_params_buffer();

// 获取参数张量映射
auto param_tensors = runner->get_param_tensors();

// 获取模型描述
auto desc = runner->get_desc();

// 构建计算图
struct ggml_cgraph* graph = runner->build_graph(
    inputs,      // 输入张量
    num_inputs,  // 输入数量
    outputs,     // 输出张量
    num_outputs  // 输出数量
);

// 执行推理
bool success = runner->compute(
    inputs,
    num_inputs,
    outputs,
    num_outputs
);

// 获取运行上下文
GGMLRunnerContext ctx = runner->get_context();
```

### 3. I/O 工具 API

```cpp
// 加载 .npy 文件到 ggml 张量
struct ggml_tensor* tensor = load_npy(
    "path/to/file.npy",  // 文件路径
    ctx                  // ggml 上下文
);

// 保存 ggml 张量到 .npy 文件
bool success = save_npy(
    "path/to/file.npy",  // 文件路径
    tensor               // 张量
);

// 支持的数据类型
// - F32 (float32)
// - F16 (float16)
// - I32 (int32)
// - I64 (int64)
```

### 4. 块/组件 API

```cpp
// 初始化块参数
block->init_params(ctx, prefix);

// 前向传播
struct ggml_tensor* output = block->forward(
    ctx,      // ggml 上下文
    input,    // 输入张量
    ...       // 其他参数
);

// 获取参数张量
auto params = block->get_param_tensors();
```

---

## 使用示例

### 示例 1: 加载模型并执行 T2V 推理

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

    // 3. 加载 Transformer DiT
    auto transformer = ModelRegistry::instance()->create<WanRunner>(
        "wan-runner-t2v",
        GGML_BACKEND_CPU,
        false,
        {},
        "models/"
    );
    transformer->alloc_params_buffer();

    // 4. 加载 VAE 解码器
    auto vae_decoder = ModelRegistry::instance()->create<WanVAERunner>(
        "wan-vae-t2v-decode",
        GGML_BACKEND_CPU,
        false,
        {},
        "models/"
    );
    vae_decoder->alloc_params_buffer();

    // 5. 执行推理
    // ... (详见 docs/examples/model_registry_usage.cpp)

    return 0;
}
```

### 示例 2: 加载和保存张量

```cpp
#include "src/test_io_utils.hpp"

int main() {
    // 创建 GGML 上下文
    GGMLCtxRAII ctx_guard(256 * 1024 * 1024);  // 256MB
    struct ggml_context* ctx = ctx_guard.get();

    // 加载 .npy 文件
    struct ggml_tensor* tensor = load_npy("data/input.npy", ctx);

    // 处理张量
    // ...

    // 保存结果
    save_npy("data/output.npy", tensor);

    return 0;
}
```

---

## 关键设计模式

### 1. 工厂模式 (Factory Pattern)

- **ModelRegistry**: 单例工厂，管理所有模型创建
- **REGISTER_MODEL_FACTORY 宏**: 编译时注册模型工厂

### 2. 组合模式 (Composite Pattern)

- **GGMLBlock**: 递归组合，支持嵌套块结构
- **Wan**: 包含 30-40 个 WanAttentionBlock

### 3. 模板方法模式 (Template Method Pattern)

- **GGMLRunner**: 定义推理流程框架
- **子类**: 实现具体的 build_graph 和 forward 方法

### 4. RAII 模式 (Resource Acquisition Is Initialization)

- **BackendRAII**: 自动管理后端生命周期
- **GGMLCtxRAII**: 自动管理 GGML 上下文
- **TempFile**: 自动清理临时文件

---

## 性能优化

### 1. 位置编码缓存 (OP-02)

- 缓存 RoPE 位置编码，避免重复计算
- 存储在 `WanRunner::pe_cached`

### 2. 计算图重用

- 相同输入形状的计算图可以重用
- 减少图构建开销

### 3. 张量 Offload

- 自动将不活跃张量 offload 到 CPU
- 减少 GPU 内存占用

### 4. 融合操作

- GELU 融合到 FFN
- 减少内存访问和计算开销

---

## 错误处理

### 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| `Unknown model version` | 版本字符串不存在 | 检查 model_factory.cpp 中的注册 |
| `Tensor shape mismatch` | 输入形状不匹配 | 检查输入张量的维度 |
| `Out of memory` | 内存不足 | 增加 ggml_init_params 的大小 |
| `Invalid dtype` | 不支持的数据类型 | 使用 F32/F16/I32/I64 |

---

## 参考资源

- [API 调用文档](./API_CALLS.md)
- [模型注册表使用示例](./examples/model_registry_usage.cpp)
- [I/O 工具使用示例](./examples/io_utils_usage.cpp)
- [测试文件](../tests/cpp/)
