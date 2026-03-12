# Phase 2: Build System - Context

**Gathered:** 2026-03-12
**Status:** Ready for planning

---

<domain>
## Phase Boundary

创建功能性的 CMake 构建系统，支持多种硬件后端（CUDA、Metal、Vulkan、CPU）以及跨平台编译（Linux、macOS、Windows）。

</domain>

<decisions>
## Implementation Decisions

### CMake 配置

- **CMake 版本**: 使用 CMake 3.20+，与 ggml 要求兼容
- **目标类型**: 静态库（libwan.a 或 wan.lib），便于集成
- **生成器**: Ninja（Linux/macOS）和 MSBuild（Windows）
- **C++ 标准**: C++17（与 ggml 和原项目兼容）

### 后端支持

- **CUDA 后端**: 完整支持，通过 GGML_CUDA 选项控制
- **Metal 后端**: 完整支持（macOS 专用），通过 GGML_METAL 选项控制
- **Vulkan 后端**: 完整支持，通过 GGML_VULKAN 选项控制
- **CPU 后端**: 始终支持，作为默认选项

### 第三方依赖

- **GGML**: 通过符号链接引用父目录（../ggml）
- **JSON 解析**: 使用 thirdparty/json.hpp（从原项目复制）
- **ZIP 支持**: 使用 thirdparty/zip.h（从原项目复制）
- **其他依赖**: 如有需要，从原项目复制

### 编译选项

- **调试模式**: 支持 Debug 和 Release 构建
- **编译器**: 支持 GCC、Clang、MSVC
- **平台检测**: 使用 CMake 自动检测平台特性

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets

- **stable-diffusion.cpp 的 CMakeLists.txt**: 包含完整的后端配置逻辑，可参考
- **GGML 子模块**: 已在 Phase 1 中正确初始化
- **thirdparty 目录**: 已在 Phase原 1 中复制所有必要的依赖文件

### Established Patterns

- **选项模式**: 使用 `option()` 函数定义构建选项，CMake 现代做法
- **条件编译**: 使用 `if()` 和条件依赖项控制后端特定代码
- **目标属性**: 使用 `target_include_directories()` 控制头文件可见性（PUBLIC vs PRIVATE）
- **子目录集成**: 使用 `add_subdirectory()` 集成 GGML

### Integration Points

- **与 GGML 集成**: 链接到 GGML 目标，继承其后端配置
- **安装规则**: 定义正确的安装路径（include 目录、库文件）
- **版本检测**: 通过 git 或文件检测获取版本信息

</code_context>

<specifics>
## Specific Ideas

- 参考 stable-diffusion.cpp 的 CMakeLists.txt 中的完整选项定义
- 为每个后端创建独立的编译配置块
- 使用 CMake 的 `find_package()` 检测必要的系统库

</specifics>

<deferred>
## Deferred Ideas

- CI/CD 配置：可以在后续阶段添加 GitHub Actions 或其他 CI 配置
- Docker 支持：可以在后续阶段添加 Dockerfile
- 包管理配置：CPM 或 vcpkg 可以在后续考虑

</deferred>

---
*Phase: 02-build-system*
*Context gathered: 2026-03-12*
