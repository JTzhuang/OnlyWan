# Phase 16: 集成 spdlog 日志系统 - Context

**Gathered:** 2026-03-26
**Status:** Ready for planning

<domain>
## Phase Boundary

本阶段的目标是全面启用 `spdlog` 作为 `wan-cpp` 的日志后端。通过自定义 Sink 实现日志的多路分发（终端彩色输出 + 用户回调），并重构现有的 C API 以提供更清晰的日志接口。

</domain>

<decisions>
## Implementation Decisions

### 架构设计 (Architecture)
- **D-01: 自定义 Sink 集成**：实现一个自定义的 `spdlog::sink`（`sd_callback_sink`），用于将格式化后的日志消息分发给通过 C API 注册的回调函数。
- **D-02: 多 Sink 组合**：默认使用 `spdlog::sinks::stdout_color_sink_mt` 和自定义的 `sd_callback_sink_mt`。
- **D-03: 线程安全**：所有 spdlog 组件统一使用 `_mt` (Multi-threaded) 版本，确保在多卡并发场景下的安全性。

### C API 重构 (API Refactor)
- **D-04: 回调接口调整**：重构 `sd_set_log_callback`。回调函数现在接收 `spdlog` 已经处理好的完整格式化字符串，而不再是原始的 format string。
- **D-05: 动态级别控制**：新增 `wan_set_log_level(sd_log_level_t level)` API，允许运行时动态修改 `spdlog` 的全局过滤级别。

### 日志行为 (Behavior)
- **D-06: 默认级别**：在未设置环境变量时，默认日志级别为 `INFO`。
- **D-07: 环境变量支持**：启动时检查 `WAN_LOG_LEVEL` 环境变量并应用其值。
- **D-08: 日志格式**：
    - INFO 及以上：`[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v`
    - DEBUG 级别：`[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v` (包含文件名和行号)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

- `include/wan-cpp/wan.h` — C API 声明，需要在此重构日志回调接口
- `src/util.h` / `src/util.cpp` — 日志宏和 `log_printf` 的核心实现位置
- `thirdparty/spdlog` — 已集成的 spdlog 源码目录

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `thirdparty/spdlog`：已包含 v1.13.0 源码，无需额外 FetchContent。

### Integration Points
- `src/util.cpp` 中的 `log_printf`：是所有 `LOG_DEBUG/INFO/WARN/ERROR` 宏的统一出口。
- `wan-api.cpp`：作为 C API 的具体实现入口。

</code_context>

<specifics>
## Specific Ideas
- 回调函数原型建议：`void (*sd_log_cb_t)(const char* text, void* data)`。

</specifics>

<deferred>
## Deferred Ideas
- 异步日志模式：目前使用同步阻塞模式已足够，异步模式 (async_logger) 留待未来性能压力测试后再决定是否引入。

</deferred>

---
*Phase: 16-spdlog*
*Context gathered: 2026-03-26*
