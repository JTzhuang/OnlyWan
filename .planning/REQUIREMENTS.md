# Requirements: wan-cpp

**Defined:** 2025-03-12
**Core Value:** 提供独立、轻量、跨平台的 WAN 视频生成推理能力

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

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

- [ ] **BUILD-01**: 创建 CMakeLists.txt 支持多平台编译
- [ ] **BUILD-02**: 配置 GGML 子模块编译
- [ ] **BUILD-03**: 支持多种后端（CUDA、Metal、Vulkan、CPU）
- [ ] **BUILD-04**: 配置第三方依赖（json.hpp、zip.h）

### 公共 API

- [ ] **API-01**: 创建 C 风格公共头文件（wan.h）
- [ ] **API-02**: 实现模型加载接口
- [ ] **API-03**: 实现文本生成视频（T2V）接口
- [ ] **API-04**: 实现图像生成视频（I2V）接口
- [ ] **API-05**: 实现配置参数接口（seed、steps、guidance scale 等）

### 示例程序

- [ ] **EX-01**: 提取并适配 CLI 示例程序
- [ ] **EX-02**: 实现视频输出功能（AVI 格式）
- [ ] **EX-03**: 添加基础命令行参数解析
- [ ] **EX-04**: 创建使用示例文档

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

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

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
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

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| STRUCT-01 | Phase 1 | Completed |
| STRUCT-02 | Phase 1 | Completed |
| STRUCT-03 | Phase 1 | Completed |
| CORE-01 | Phase 1 | Completed |
| CORE-02 | Phase 1 | Completed |
| CORE-03 | Phase 1 | Completed |
| CORE-04 | Phase 1 | Completed |
| CORE-05 | Phase 1 | Completed |
| BUILD-01 | Phase 2 | Pending |
| BUILD-02 | Phase 2 | Pending |
| BUILD-03 | Phase 2 | Pending |
| BUILD-04 | Phase 2 | Pending |
| API-01 | Phase 3 | Pending |
| API-02 | Phase 3 | Pending |
| API-03 | Phase 3 | Pending |
| API-04 | Phase 3 | Pending |
| API-05 | Phase 3 | Pending |
| EX-01 | Phase 4 | Pending |
| EX-02 | Phase 4 | Pending |
| EX-03 | Phase 4 | Pending |
| EX-04 | Phase 4 | Pending |

**Coverage:**
- v1 requirements: 20 total
- Mapped to phases: 20
- Unmapped: 0 ✓
- Completed: 8 (Phase 1)

---
*Requirements defined: 2025-03-12*
*Last updated: 2026-03-12 after Phase 1 completion*
