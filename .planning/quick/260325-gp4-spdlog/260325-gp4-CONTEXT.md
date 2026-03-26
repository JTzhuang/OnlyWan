# Quick Task 260325-gp4: spdlog 替换日志输出 - Context

**Gathered:** 2026-03-25
**Status:** Ready for planning

<domain>
## Task Boundary

检查 src/ 目录中所有打印日志的地方（printf, cout, cerr, fprintf, std::endl），全部替换为 spdlog 调用。

</domain>

<decisions>
## Implementation Decisions

### 日志级别映射
- 自动判断：stderr/cerr/fprintf(stderr, ...) 映射到 `spdlog::warn`，stdout/cout 映射到 `spdlog::info`。

### 替换范围
- 仅替换 `src/` 目录中的项目代码。

### 日志初始化
- 在程序启动或库初始化时添加 spdlog 初始化代码。

### Claude's Discretion
- 自动处理 spdlog 头文件的包含。
- 确保在替换时保留原始格式化逻辑。

</decisions>

<specifics>
## Specific Ideas

- 搜索 `printf`, `std::cout`, `std::cerr`, `fprintf(stderr, ...)`, `std::endl`。
- 在 `wan_load_model` 或类似初始化函数中添加 spdlog 初始化。
- 在 `src/util.cpp` 中定义全局 logger 并统一使用。

</specifics>

<canonical_refs>
## Canonical References

- spdlog 官方文档（v1.13.0）
- 项目已有的 spdlog 集成（thirdparty/spdlog/）

</canonical_refs>
