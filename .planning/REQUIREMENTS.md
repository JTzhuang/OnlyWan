# Requirements: wan-cpp

**定义时间：** 2025-03-12
**核心价值：** 提供独立、轻量、跨平台的 WAN 视频生成推理能力

## v1 Requirements

初始发布的需求。每个对应路线图阶段。

### 项目结构

- [x] **STRUCT-01**: 创建独立的目录结构（include、src、examples、thirdparty、ggml 子模块）
- [x] **STRUCT-02**: 配置 .gitmodules 引用 ggml 子模块
- [x] **STRUCT-03**: 添加 .gitignore 和基础 README

### 核心代码提取

- [x] **CORE-01**: 提取 wan.hpp 及其依赖文件（common_block.hpp、rope.hpp、vae.hpp、flux.hpp）
- [x] **CORE-02**: 提取图像预处理功能（preprocessing.hpp）
- [x] **CORE-03**: 提取工具函数（util.h/cpp）
- [x] **CORE-04**: 提取模型加载相关代码（model.h/cpp 中的 WAN 部分）
- [x] **CORE-05**: 修正所有相对包含路径以适应新结构

### 构建系统

- [x] **BUILD-01**: 创建 CMakeLists.txt 支持多平台编译
- [x] **BUILD-02**: 配置 GGML 子模块编译
- [x] **BUILD-03**: 支持多种后端（CUDA、Metal、Vulkan、CPU）
- [x] **BUILD-04**: 配置第三方依赖（json.hpp、zip.h）

### 公共 API

- [x] **API-01**: 创建 C 风格公共头文件（wan.h）
- [x] **API-02**: 实现模型加载接口
- [x] **API-03**: 实现文本生成视频（T2V）接口
- [x] **API-04**: 实现图像生成视频（I2V）接口
- [x] **API-05**: 实现配置参数接口（seed、steps、guidance scale 等）

### 示例程序

- [x] **EX-01**: 提取并适配 CLI 示例程序
- [x] **EX-02**: 实现视频输出功能（AVI 格式）
- [x] **EX-03**: 添加基础命令行参数解析
- [x] **EX-04**: 创建使用示例文档

### 编码器集成

- [x] **ENCODER-01**: 集成 T5 文本编码器
- [x] **ENCODER-02**: 集成 CLIP 图像编码器

## v1.1 Requirements

**里程碑目标：** 支持 safetensors 格式模型，修复 v1.0 遗留问题

### Safetensors 支持

- [x] **SAFE-01**: 用户可直接加载 .safetensors 格式的 WAN 模型文件（无需预转换）
- [x] **SAFE-02**: 提供 safetensors → GGUF 转换工具（独立 CLI 可执行文件）
- [ ] **SAFE-03**: 转换工具支持 WAN2.1/2.2 所有子模型（DiT、VAE、T5、CLIP）（部分满足：dit-* 类型可加载，vae/t5/clip 待 Phase 13 文档说明）

### API 修复

- [x] **FIX-01**: 移除 `wan_generate_video_t2v` / `wan_generate_video_i2v` 遗留 stub，调用实际 `_ex` 实现
- [x] **FIX-02**: `progress_cb` 在 Euler 去噪循环每步实际触发，传入当前步数和总步数

### 性能优化

- [x] **PERF-01**: 词汇表文件（umt5/clip，~85MB）改为运行时 mmap 加载，消除编译时头文件嵌入（Phase 12 修复公共 API 暴露）

## v2 Requirements

延迟到未来发布。已追踪但不在当前路线图中。

### 增强功能

- **ENH-01**: 进度回调功能（支持 UI 集成）
- **ENH-02**: 预览生成功能（推理过程中实时查看）
- **ENH-03**: 批量生成支持
- **ENH-04**: C++17 风格 API 包装（Python/Rust 绑定）
- **ENH-05**: 流式输出（大视频生成不占用全部内存）
- **ENH-06**: 详细的日志系统

### 优化

- **OPT-01**: 内存池机制
- **ENH-07**: Spectrum 缓存功能
- **ENH-08**: RAII 资源管理优化

## 超出范围

明确排除。防止范围膨胀。

| 功能 | 原因 |
|---------|--------|
| 其他 Stable Diffusion 模型（SD1、SD2、SDXL、Flux 等） | 保持库聚焦，减少膨胀 |
| 训练功能 | 这是推理库，训练是独立关注点 |
| 权重转换工具 | 转换是一次性的，不是核心推理功能 |
| 网络服务器/API | 库使用者构建自己的服务器 |
| GUI 应用程序 | 库不是应用程序，允许灵活性 |
| 静态图像生成 | 库专注于视频生成 |
| 推理过程中动态切换模型 | 复杂且易错，很少需要 |
| 自动模型下载 | 安全考虑、网络依赖、复杂性 |
| 插件系统 | 超出此范围的设计 |

## 追踪性

哪些阶段覆盖哪些需求。路线图创建期间更新。

| 需求 | 阶段 | 状态 |
|-------------|-------|--------|
| STRUCT-01 | Phase 1 | Completed |
| STRUCT-02 | Phase 1 | Completed |
| STRUCT-03 | Phase 1 | Completed |
| CORE-01 | Phase 7 | Complete |
| CORE-02 | Phase 7 | Complete |
| CORE-03 | Phase 1 | Completed |
| CORE-04 | Phase 7 | Complete |
| CORE-05 | Phase 1 | Completed |
| BUILD-01 | Phase 6 | Complete |
| BUILD-02 | Phase 2 | Completed |
| BUILD-03 | Phase 2 | Completed |
| BUILD-04 | Phase 2 | Completed |
| API-01 | Phase 3 | Completed |
| API-02 | Phase 7 | Complete |
| API-03 | Phase 8 | Complete |
| API-04 | Phase 8 | Complete |
| API-05 | Phase 6 | Complete |
| EX-01 | Phase 4 | Completed |
| EX-02 | Phase 8 | Complete |
| EX-03 | Phase 4 | Completed |
| EX-04 | Phase 4 | Completed |
| ENCODER-01 | Phase 7 | Complete |
| ENCODER-02 | Phase 7 | Complete |
| SAFE-01 | Phase 10 | Complete |
| SAFE-02 | Phase 11 | Complete |
| SAFE-03 | Phase 13 | Pending |
| FIX-01 | Phase 9 | Complete |
| FIX-02 | Phase 9 | Complete |
| PERF-01 | Phase 12 | Pending |

**覆盖率：**
- v1 需求总计: 22
- 映射到阶段: 22
- 未映射: 0 ✓
- 已完成: 10 (Phase 1: 8, Phase 2: 3, Phase 3: 1, Phase 4: 3)
- 待完成: 12 (Phase 6: 2, Phase 7: 6, Phase 8: 3, ENCODER: 1)

---
*需求定义时间：2025-03-12*
*最后更新：2026-03-15 after Phase 5 添加*
