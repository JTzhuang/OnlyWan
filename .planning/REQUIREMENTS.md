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
- [ ] **CORE-02**: 提取图像预处理功能（preprocessing.hpp）
- [x] **CORE-03**: 提取工具函数（util.h/cpp）
- [x] **CORE-04**: 提取模型加载相关代码（model.h/cpp 中的 WAN 部分）
- [x] **CORE-05**: 修正所有相对包含路径以适应新结构

### 构建系统

- [ ] **BUILD-01**: 创建 CMakeLists.txt 支持多平台编译
- [x] **BUILD-02**: 配置 GGML 子模块编译
- [x] **BUILD-03**: 支持多种后端（CUDA、Metal、Vulkan、CPU）
- [x] **BUILD-04**: 配置第三方依赖（json.hpp、zip.h）

### 公共 API

- [x] **API-01**: 创建 C 风格公共头文件（wan.h）
- [x] **API-02**: 实现模型加载接口
- [ ] **API-03**: 实现文本生成视频（T2V）接口
- [ ] **API-04**: 实现图像生成视频（I2V）接口
- [ ] **API-05**: 实现配置参数接口（seed、steps、guidance scale 等）

### 示例程序

- [x] **EX-01**: 提取并适配 CLI 示例程序
- [ ] **EX-02**: 实现视频输出功能（AVI 格式）
- [x] **EX-03**: 添加基础命令行参数解析
- [x] **EX-04**: 创建使用示例文档

### 编码器集成

- [ ] **ENCODER-01**: 集成 T5 文本编码器
- [ ] **ENCODER-02**: 集成 CLIP 图像编码器

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
| CORE-02 | Phase 7 | Pending |
| CORE-03 | Phase 1 | Completed |
| CORE-04 | Phase 7 | Complete |
| CORE-05 | Phase 1 | Completed |
| BUILD-01 | Phase 6 | Pending |
| BUILD-02 | Phase 2 | Completed |
| BUILD-03 | Phase 2 | Completed |
| BUILD-04 | Phase 2 | Completed |
| API-01 | Phase 3 | Completed |
| API-02 | Phase 7 | Complete |
| API-03 | Phase 8 | Pending |
| API-04 | Phase 8 | Pending |
| API-05 | Phase 6 | Pending |
| EX-01 | Phase 4 | Completed |
| EX-02 | Phase 8 | Pending |
| EX-03 | Phase 4 | Completed |
| EX-04 | Phase 4 | Completed |
| ENCODER-01 | Phase 7 | Pending |
| ENCODER-02 | Phase 7 | Pending |

**覆盖率：**
- v1 需求总计: 22
- 映射到阶段: 22
- 未映射: 0 ✓
- 已完成: 10 (Phase 1: 8, Phase 2: 3, Phase 3: 1, Phase 4: 3)
- 待完成: 12 (Phase 6: 2, Phase 7: 6, Phase 8: 3, ENCODER: 1)

---
*需求定义时间：2025-03-12*
*最后更新：2026-03-15 after Phase 5 添加*
