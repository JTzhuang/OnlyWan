# wan-cpp

## What This Is

一个独立的 WAN 视频生成模型 C++ 推理库，从 stable-diffusion.cpp 中抽离出来。支持 WAN2.1 和 WAN2.2 系列模型的各种生成模式（T2V、I2V、FLF2V、VACE、TI2V）。专注于高效的视频生成推理，使用 ggml 作为底层计算框架。

## Core Value

提供独立、轻量、跨平台的 WAN 视频生成推理能力，让其他项目无需依赖完整的 stable-diffusion.cpp 即可使用 WAN 模型。

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] 提取 WAN 核心代码（wan.hpp 及其依赖）
- [ ] 提取图像预处理功能
- [ ] 提取权重转换功能
- [ ] 保留完整的 ggml 子模块
- [ ] 创建独立的 CMake 构建系统
- [ ] 创建视频生成示例程序
- [ ] 支持权重 offloading 到 CPU
- [ ] 跨平台支持（Linux、Windows、macOS）

### Out of Scope

- [ ] 其他 Stable Diffusion 模型（SD1、SD2、SDXL、Flux 等） — 只保留 WAN
- [ ] 图像生成任务（仅专注于视频生成）
- [ ] 网络服务 — 可以通过库函数集成

## Context

**源代码位置：** /home/jtzhuang/projects/stable-diffusion.cpp/

**核心文件：**
- src/wan.hpp — WAN 模型核心实现（约 2300 行）
- src/common_block.hpp — 通用模块基类
- src/rope.hpp — 旋转位置编码
- src/vae.hpp — VAE 编解码器
- src/flux.hpp — Flow 匹配相关功能
- src/preprocessing.hpp — 图像预处理
- src/util.h/cpp — 工具函数
- src/model.h/cpp — 模型加载和版本管理
- examples/cli/main.cpp — 命令行示例

**依赖的 GGML 扩展功能：**
- ggml_extend.hpp — 自定义 GGML 操作

（通过 .gitmodules 子模块方式集成）
- ggml/ — GGML 计算库
- thirdparty/ — 第三方依赖（json.hpp、zip.h 等）

## Constraints

- **技术栈**: C++17, CMake, ggml
- **兼容性**: 需要保持与原 stable-diffusion.cpp 的接口兼容性
- **构建**: 必须支持多种后端（CUDA、Metal、Vulkan 等）
- **依赖**: 使用子模块方式引用 ggml，避免代码重复

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| 使用子模块方式引用 ggml | 避免代码重复，保持同步 | — Pending |
| 独立的 CMakeLists.txt | 解耦原项目构建系统 | — Pending |
| 只保留 WAN 模型代码 | 减小体积，聚焦核心功能 | — Pending |

---
*Last updated: 2025-03-12 after initialization*
