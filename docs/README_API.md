# OnlyWan API 文档总览

欢迎使用 OnlyWan 项目的完整 API 文档。本文档集合提供了从架构设计到实际使用的全面指导。

## 📚 文档结构

### 1. **API_CALLS.md** - API 参考手册
快速查阅所有 API 函数的签名、参数和返回值。
- ModelRegistry API
- GGMLRunner API
- I/O 工具 API
- 块/组件 API
- 所有 9 个已注册模型的列表

**适合**: 需要快速查找 API 函数的开发者

### 2. **API_FLOW_ARCHITECTURE.md** - 系统架构与流程
详细的系统架构说明和 API 调用流程。
- 分层架构概览
- 模型加载流程
- T2V/I2V/TI2V 推理流程
- 模块关系图
- 张量形状变换链
- 数据流向说明

**适合**: 想要理解系统整体设计的开发者

### 3. **API_FLOW_DIAGRAMS.md** - 可视化流程图
12 个 Mermaid 流程图，展示不同的 API 调用流程。
- 系统整体架构流程图
- 模型加载流程图
- T2V 推理流程图
- I2V/TI2V 流程差异
- WanAttentionBlock 内部流程
- VAE 解码流程
- 模型注册机制流程
- 张量形状变换链
- 计算图构建与执行流程
- 错误处理流程
- 内存管理流程
- 完整推理流程 (端到端)

**适合**: 需要可视化理解流程的开发者

### 4. **API_COMPLETE_GUIDE.md** - 完整使用指南
从快速开始到最佳实践的完整指南。
- 快速开始示例
- 核心概念详解
- 详细 API 参考
- 完整推理示例 (T2V 和 I2V)
- 最佳实践
- 常见问题解答

**适合**: 想要学习如何使用 API 的开发者

### 5. **examples/** - 代码示例
可运行的代码示例。
- `model_registry_usage.cpp` - 模型注册表使用示例
- `io_utils_usage.cpp` - I/O 工具使用示例

**适合**: 需要参考代码的开发者

---

## 🚀 快速开始

### 最小化示例

```cpp
#include "src/model_registry.hpp"
#include "src/wan.hpp"

int main() {
    // 初始化后端
    BackendRAII backend_guard(GGML_BACKEND_CPU);

    // 加载模型
    auto runner = ModelRegistry::instance()->create<WanRunner>(
        "wan-runner-t2v",
        GGML_BACKEND_CPU,
        false,
        {},
        "models/"
    );
    runner->alloc_params_buffer();

    // 执行推理
    bool success = runner->compute(inputs, 1, outputs, 1);

    return success ? 0 : 1;
}
```

详见 [API_COMPLETE_GUIDE.md](./API_COMPLETE_GUIDE.md#快速开始)

---

## 📖 学习路径

### 初学者
1. 阅读 [API_FLOW_ARCHITECTURE.md](./API_FLOW_ARCHITECTURE.md#系统架构概览) 的系统架构部分
2. 查看 [API_FLOW_DIAGRAMS.md](./API_FLOW_DIAGRAMS.md#1-系统整体架构流程图) 的架构流程图
3. 跟随 [API_COMPLETE_GUIDE.md](./API_COMPLETE_GUIDE.md#快速开始) 的快速开始示例

### 中级开发者
1. 学习 [API_COMPLETE_GUIDE.md](./API_COMPLETE_GUIDE.md#核心概念) 的核心概念
2. 研究 [API_FLOW_DIAGRAMS.md](./API_FLOW_DIAGRAMS.md#3-t2v-文本到视频-推理流程图) 的 T2V 推理流程
3. 参考 [examples/](./examples/) 中的完整代码示例

### 高级开发者
1. 深入研究 [API_FLOW_ARCHITECTURE.md](./API_FLOW_ARCHITECTURE.md#数据流向) 的数据流向
2. 查看 [API_FLOW_DIAGRAMS.md](./API_FLOW_DIAGRAMS.md#5-wanattentionblock-内部流程) 的内部块结构
3. 学习 [API_COMPLETE_GUIDE.md](./API_COMPLETE_GUIDE.md#最佳实践) 的最佳实践和优化策略

---

## 🔍 按用途查找

### 我想...

#### 快速查找 API 函数
→ 查看 [API_CALLS.md](./API_CALLS.md)

#### 理解系统架构
→ 查看 [API_FLOW_ARCHITECTURE.md](./API_FLOW_ARCHITECTURE.md)

#### 看可视化流程图
→ 查看 [API_FLOW_DIAGRAMS.md](./API_FLOW_DIAGRAMS.md)

#### 学习如何使用 API
→ 查看 [API_COMPLETE_GUIDE.md](./API_COMPLETE_GUIDE.md)

#### 查看代码示例
→ 查看 [examples/](./examples/)

#### 了解模型加载流程
→ 查看 [API_FLOW_DIAGRAMS.md#2-模型加载流程图](./API_FLOW_DIAGRAMS.md#2-模型加载流程图)

#### 了解 T2V 推理流程
→ 查看 [API_FLOW_DIAGRAMS.md#3-t2v-文本到视频-推理流程图](./API_FLOW_DIAGRAMS.md#3-t2v-文本到视频-推理流程图)

#### 了解 I2V/TI2V 推理流程
→ 查看 [API_FLOW_DIAGRAMS.md#4-i2vti2v-图像到视频-流程差异](./API_FLOW_DIAGRAMS.md#4-i2vti2v-图像到视频-流程差异)

#### 学习最佳实践
→ 查看 [API_COMPLETE_GUIDE.md#最佳实践](./API_COMPLETE_GUIDE.md#最佳实践)

#### 解决常见问题
→ 查看 [API_COMPLETE_GUIDE.md#常见问题](./API_COMPLETE_GUIDE.md#常见问题)

---

## 🎯 核心概念速览

### ModelRegistry (模型注册表)
单例工厂，管理所有模型的创建。支持 12 个已注册的模型变体。

```cpp
auto runner = ModelRegistry::instance()->create<WanRunner>(
    "wan-runner-t2v",
    GGML_BACKEND_CPU,
    false,
    {},
    "models/"
);
```

### GGMLRunner (模型运行器)
定义模型推理的通用接口。所有模型都继承自此基类。

```cpp
runner->alloc_params_buffer();
runner->compute(inputs, num_inputs, outputs, num_outputs);
```

### 张量形状约定

**T2V 路径**:
- 输入: `[N*16, T, H, W]` (噪声)
- 输出: `[N*3, T*4, H*8, W*8]` (RGB 视频帧)

**I2V/TI2V 路径**:
- 额外输入: `[N, 3, 224, 224]` (参考图像)
- 输出: `[N*3, T*4, H*8, W*8]` (RGB 视频帧)

### RAII 模式
自动管理资源生命周期。

```cpp
BackendRAII backend_guard(GGML_BACKEND_CPU);
GGMLCtxRAII ctx_guard(256 * 1024 * 1024);
```

---

## 📊 已注册的模型

### CLIP Vision (3 个)
- `clip-vision-vit-l-14`
- `clip-vision-vit-h-14`
- `clip-vision-vit-bigg-14`

### T5 (2 个)
- `t5-standard`
- `t5-umt5`

### WAN VAE (4 个)
- `wan-vae-t2v`
- `wan-vae-t2v-decode`
- `wan-vae-i2v`
- `wan-vae-ti2v`

### WAN Transformer (3 个)
- `wan-runner-t2v`
- `wan-runner-i2v`
- `wan-runner-ti2v`

---

## 🛠️ 常用操作

### 创建模型实例
```cpp
auto runner = ModelRegistry::instance()->create<WanRunner>(
    "wan-runner-t2v",
    GGML_BACKEND_CPU,
    false,
    {},
    "models/"
);
```

### 检查模型版本
```cpp
if (ModelRegistry::instance()->has_version<WanRunner>("wan-runner-t2v")) {
    // 版本存在
}
```

### 加载 .npy 文件
```cpp
struct ggml_tensor* tensor = load_npy("data/input.npy", ctx);
```

### 保存 .npy 文件
```cpp
save_npy("data/output.npy", tensor);
```

### 执行推理
```cpp
bool success = runner->compute(inputs, 1, outputs, 1);
```

---

## 📝 文档维护

这些文档由以下快速任务生成：
- **260328-m62**: 结合相关的测试工具，生成 API 调用文档

最后更新: 2026-03-28

---

## 🔗 相关资源

- [项目主页](../)
- [测试文件](../tests/cpp/)
- [源代码](../src/)
- [构建指南](../README.md)

---

## 💡 提示

- 所有文档都支持 Markdown 格式，可以在任何 Markdown 查看器中查看
- API_FLOW_DIAGRAMS.md 中的 Mermaid 图表可以在支持 Mermaid 的平台上渲染
- 建议按照学习路径顺序阅读文档，以获得最佳学习效果

---

**需要帮助?** 查看 [API_COMPLETE_GUIDE.md#常见问题](./API_COMPLETE_GUIDE.md#常见问题) 获取常见问题的答案。
