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

## v1.2 Requirements

**里程碑目标：** 多卡推理支持，提升吞吐量和处理能力

### 多卡 API 与配置

- [x] **MGPU-01**: 扩展 `wan_params_t` 添加多卡配置字段（gpu_ids、num_gpus、distribution_strategy 枚举）
- [x] **MGPU-02**: 扩展 `wan_context` 支持多后端管理（多个 ggml_backend_t、ggml_backend_sched_t）
- [x] **MGPU-03**: CMake 添加 `WAN_NCCL` 选项，条件链接 NCCL 库

### 多卡推理实现

- [x] **MGPU-04**: 实现 GPU 设备枚举、验证（同构检查）和多后端初始化
- [x] **MGPU-05**: 实现张量并行 — 使用 GGML split buffer 将模型权重分布到多个 GPU
- [x] **MGPU-06**: 实现数据并行 — 多上下文并发生成（batch > 1 场景）
- [x] **MGPU-07**: 去噪循环适配多卡执行（ggml_backend_sched_graph_compute 替代 ggml_backend_graph_compute）

### CLI 与验证

- [ ] **MGPU-08**: wan-cli 添加 `--gpu-ids` 和 `--num-gpus` 参数
- [ ] **MGPU-09**: 多卡推理数值精度验证（与单卡结果偏差 < 0.01%）
- [ ] **MGPU-10**: 性能基准测试（步骤延迟、内存使用、通信带宽利用率）

### 错误处理

- [x] **MGPU-11**: 新增 `WAN_ERROR_GPU_FAILURE` 错误码，GPU 失败时立即中断并返回详细错误信息

### 向后兼容

- [x] **MGPU-12**: 不指定多卡配置时自动回退到单卡推理，现有 API 行为不变

## v1.3 Requirements

**里程碑目标：** C++ 单元测试框架与模型工厂模式

### 测试基础设施

- [x] **TEST-01**: 建立 C++ 测试构建系统 — CMake `WAN_BUILD_TESTS` 选项、`tests/cpp/` 子目录、`ctest` 集成、自定义轻量测试框架（断言宏 + 测试套件 + 报告）
- [x] **TEST-02**: 实现通用模板工厂模式 — `ModelFactory<ModelType, VersionEnum>` 支持动态注册 (`register_version`) 和统一创建 (`create`) 所有 Runner 版本，工厂自身通过单元测试验证
- [x] **TEST-03**: 为四个核心 Runner 建立初始化单元测试 — CLIP (3版本)、T5/UMT5 (2版本)、VAE/AutoEncoderKL (4版本)、Transformer/FluxRunner (5版本)，使用合成数据（无需真实权重）验证构造、`alloc_params_buffer`、`get_desc` 等基本接口

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
| CPU 多进程推理 | 当前仅支持 CUDA 多卡 |
| 跨机器分布式推理 | 需要网络通信，复杂度高，可作为 v2 功能 |
| 动态 GPU 分配 | 运行时动态增加/减少 GPU 数量，可作为未来优化 |
| FP8 计算 | 多卡推理中的低精度计算，需要特定硬件支持 |

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
| SAFE-03 | Phase 13 | Complete |
| FIX-01 | Phase 9 | Complete |
| FIX-02 | Phase 9 | Complete |
| PERF-01 | Phase 12 | Complete |
| MGPU-01 | Phase 15 | Planned |
| MGPU-02 | Phase 15 | Planned |
| MGPU-03 | Phase 15 | Planned |
| MGPU-04 | Phase 15 | Planned |
| MGPU-05 | Phase 15 | Planned |
| MGPU-06 | Phase 15 | Planned |
| MGPU-07 | Phase 15 | Planned |
| MGPU-08 | Phase 15 | Planned |
| MGPU-09 | Phase 15 | Planned |
| MGPU-10 | Phase 15 | Planned |
| MGPU-11 | Phase 15 | Planned |
| MGPU-12 | Phase 15 | Planned |
| TEST-01 | Phase 17 | Planned |
| TEST-02 | Phase 17 | Planned |
| TEST-03 | Phase 17 | Planned |

---
*需求定义时间：2025-03-12*
*最后更新：2026-03-27 after Phase 17 测试需求添加*
