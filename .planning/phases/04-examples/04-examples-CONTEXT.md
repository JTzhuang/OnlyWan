# Phase 4: Examples - Context

**Gathered:** 2026-03-12
**Status:** Ready for planning

---

<domain>
## Phase Boundary

提供生产就绪的示例程序，展示库的完整使用方法。

</domain>

<decisions>
## Implementation Decisions

### 示例程序

**CLI 适配**: 将 stable-diffusion.cpp 的 CLI 示例适配到新的 API
- 保留原有的命令行参数解析逻辑
- 修改 API 调用为新的 wan-* 函数

### 视频输出

**AVI 格式支持**: 保持原有的 avi_writer.h
- 确保视频输出功能正常工作

### 命令行参数

**参数映射**: 将原有参数映射到新 API
- 保持向后兼容的参数别名

### 编译配置

**示例 CMakeLists.txt**: 创建独立的示例编译目标
- 链接到主 wan-cpp 库
- 配置正确的包含路径

### Claude's Discretion

无特殊决策需要记录。

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets

- **stable-diffusion.cpp CLI 源例**: `examples/cli/main.cpp` - 可参考其参数处理逻辑
- **avi_writer.h**: `examples/cli/avi_writer.h` - 视频写入功能
- **ArgOptions 结构**: `examples/common/common.hpp` 中的参数定义模式

### Established Patterns

- **ArgOptions 使用模式**: 使用 struct 和 ArgOptions 类型定义参数
- **参数分组**: 按功能分组参数（生成、采样、输出等）
- **帮助文本生成**: 自动生成帮助文档

### Integration Points

- **与主库链接**: 示例需要链接到 `libwan-cpp.a`
- **使用公共头文件**: 包含 `include/wan-cpp/wan.h`
- **GGML 后端共享**: 使用已配置的后端选项

</code_context>

<specifics>
## Specific Ideas

参考 stable-diffusion.cpp 的 CLI 实现，创建简化但功能完整的示例程序：

1. 创建 `wan-cli` 可执行文件
2. 实现基本参数解析（model、prompt、output、width、height 等）
3. 添加视频输出功能（AVI 格式）
4. 支持常见的生成选项（steps、cfg scale、seed 等）

</specifics>

<deferred>
## Deferred Ideas

无需要延迟的想法。

</deferred>

---
*Phase: 04-examples*
*Context gathered: 2026-03-12*
