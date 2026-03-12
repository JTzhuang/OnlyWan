# Phase 3: Public API - Context

**Gathered:** 2026-03-12
**Status:** Ready for planning

---

<domain>
## Phase Boundary

通过 C 风格公共 API 暴露 WAN 功能，实现语言互操作性（ABI 稳定性）。

</domain>

<decisions>
## Implementation Decisions

### 公共 API 设计

- **C API 风格**: 使用不透明句柄模式（`wan_context_t*` 类型）提供 ABI 稳定性
- **函数命名规范**: 使用 `wan_` 前缀避免与其他库冲突
- **错误处理**: 通过返回码和错误字符串提供详细的错误信息

### 模型加载

- **加载方式**: 从 GGUF 文件加载，与原项目保持一致
- **版本检测**: 自动检测 WAN2.1 vs WAN2.2 模型变体

### 文本生成视频（T2V）

- **函数签名**: 提供简洁的文本到视频生成接口
- **参数支持**: 支持步数、CFG scale、种子等常用参数

### 图像生成视频（I2V）

- **函数签名**: 提供图像到视频生成接口
- **参数支持**: 支持输入图像路径、条件帧等参数

### 配置接口

- **Builder 模式**: 提供链式配置 API（类似原项目的 ArgOptions）
- **参数管理**: 支持各种生成参数的设置

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets

- **stable-diffusion.cpp 的 sd-cli 示例**: 命令行参数解析实现（ArgOptions 结构）
- **GGML backend 初始化模式**: 参考原项目的后端初始化逻辑

### Established Patterns

- **回调函数模式**: 使用函数指针回调（如 `sd_progress_cb_t`）
- **RAII 资源管理**: 使用智能指针管理模型和计算上下文

### Integration Points

- **与 WAN 核心代码集成**: 公共 API 将调用已提取的 WAN 类
- **与 GGML 集成**: 使用 GGML 的张量操作和后端功能
- **与工具函数集成**: 使用已提取的 util.h/cpp 工具函数

</code_context>

<specifics>
## Specific Ideas

- 参考 stable-diffusion.cpp 中的 CLI 参数结构，设计更简洁的 wan_ 前缀版本
- 使用 `wan_context_t` 作为不透明句柄，包含模型和参数指针
- 提供分离的加载、生成、释放函数
- 考虑提供 C++ 包装类，简化 RAII 管理

</specifics>

<deferred>
## Deferred Ideas

None — 所有阶段 1 的功能都在原计划中

</deferred>

---
*Phase: 03-public-api*
*Context gathered: 2026-03-12*
