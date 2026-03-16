# wan-cpp

## What This Is

一个独立的 WAN 视频生成模型 C++ 推理库，从 stable-diffusion.cpp 中抽离出来。支持 WAN2.1 和 WAN2.2 系列模型的各种生成模式（T2V、I2V）。专注于高效的视频生成推理，使用 ggml 作为底层计算框架。

v1.0 已发布：完整的 T2V 和 I2V 生成流水线，包含 Euler 采样器、T5/CLIP 编码器、VAE 解码和 AVI 输出。

## Core Value

提供独立、轻量、跨平台的 WAN 视频生成推理能力，让其他项目无需依赖完整的 stable-diffusion.cpp 即可使用 WAN 模型。

## Requirements

### Validated

- ✓ 提取 WAN 核心代码（wan.hpp 及其依赖） — v1.0
- ✓ 提取图像预处理功能（ggml_extend.hpp sd_image_to_ggml_tensor） — v1.0
- ✓ 保留完整的 ggml 子模块（符号链接方式） — v1.0
- ✓ 创建独立的 CMake 构建系统（多后端支持） — v1.0
- ✓ 创建视频生成示例程序（wan-cli T2V/I2V） — v1.0
- ✓ 跨平台支持（Linux 验证，CUDA/Metal/Vulkan 选项就绪） — v1.0
- ✓ C 风格公共 API（wan.h，支持语言绑定） — v1.0
- ✓ T5 文本编码器 + CLIP 图像编码器集成 — v1.0
- ✓ 完整 Euler flow-matching 去噪循环 + CFG — v1.0
- ✓ AVI 视频输出（DIB/BI_RGB，无压缩） — v1.0

### Active

- [ ] 进度回调在生成循环中实际触发（progress_cb 已接入但未调用）
- [ ] negative_prompt 在 CFG 无条件 pass 中生效（当前硬编码空字符串）
- [ ] 移除公共 API 中的遗留 stub（wan_generate_video_t2v/i2v 永远返回 UNSUPPORTED）
- [ ] 词汇表文件改为内存映射 I/O（当前 ~85MB 头文件嵌入）

### Out of Scope

- 其他 Stable Diffusion 模型（SD1、SD2、SDXL、Flux 等） — 只保留 WAN
- 图像生成任务 — 仅专注于视频生成
- 网络服务 — 可以通过库函数集成
- 训练功能 — 这是推理库
- 权重转换工具 — 一次性操作，不是核心推理功能

## Context

**当前状态：** v1.0 已发布（2026-03-16）
**代码规模：** ~44,500 LOC（不含词汇表头文件）
**技术栈：** C++17, CMake 3.20+, ggml（符号链接）
**已验证平台：** Linux CPU

**架构决策（v1.0）：**
- `wan-api.cpp` 是唯一包含 wan.hpp/t5.hpp/clip.hpp 的翻译单元（ODR 修复）
- `wan_t2v.cpp`/`wan_i2v.cpp` 保留为 stub，真实实现在 `wan-api.cpp`
- CMake 使用显式源文件列表（非 GLOB_RECURSE）防止重复符号
- GGML 通过符号链接引用父仓库（git 子模块限制的变通方案）

**已知技术债务：**
- WanBackend ctx 在 wan_load_model 中分配 256MB 但从未使用
- WanImage/WanVideo 方法在 wan-internal.hpp 中声明但从未实现
- avi_writer.h 注释仍说"stub"（实现已完整）

## Constraints

- **技术栈**: C++17, CMake, ggml
- **构建**: 必须支持多种后端（CUDA、Metal、Vulkan 等）
- **依赖**: 使用符号链接方式引用 ggml，避免代码重复

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| 符号链接引用 ggml | git 不允许子模块共享父仓库目录 | ✓ 正常工作，但依赖父仓库路径 |
| 独立的 CMakeLists.txt | 解耦原项目构建系统 | ✓ 多后端支持就绪 |
| 只保留 WAN 模型代码 | 减小体积，聚焦核心功能 | ✓ 库聚焦，无 SD 膨胀 |
| wan-api.cpp 单 TU 架构 | 避免 wan.hpp/t5.hpp/clip.hpp ODR 违规 | ✓ 链接干净，0 重复符号 |
| 显式 CMake 源文件列表 | GLOB_RECURSE 导致重复符号链接失败 | ✓ 构建稳定 |
| Euler flow-matching 采样器 | 与 stable-diffusion.cpp 原始实现保持一致 | ✓ 正确的 WAN 推理 |
| 词汇表嵌入为头文件 | 简化分发，无外部文件依赖 | ⚠️ ~85MB 源码膨胀，考虑 v2 改为 mmap |

---
*Last updated: 2026-03-16 after v1.0 milestone*
