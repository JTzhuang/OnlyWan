# Plan: Phase 2 - Build System

**Phase:** 2 - Build System
**Status:** Ready
**Created:** 2026-03-12

---

<metadata>
<dependencies>
- Phase 1 (Foundation)
</dependencies>
<files_modified>
</files_modified>
<autonomous>false</autonomous>
</metadata>

## Summary

创建功能性的 CMake 构建系统，支持多种硬件后端（CUDA、Metal、Vulkan、CPU）以及跨平台编译（Linux、macOS、Windows）。

## Requirements

| ID | Requirement | Status |
|----|-------------|--------|
| BUILD-01 | 创建 CMakeLists.txt 支持多平台编译 | Pending |
| BUILD-02 | 配置 GGML 子模块编译 | Pending |
| BUILD-03 | 支持多种后端（CUDA、Metal、Vulkan、CPU） | Pending |
| BUILD-04 | 配置第三方依赖（json.hpp、zip.h） | Pending |

---

## Tasks

### Task 1: 创建 CMakeLists.txt 主文件

创建项目的主 CMake 配置文件。

**Requirements:** BUILD-01

**Steps:**
1. 设置 CMake 最低版本要求（3.20+）
2. 定义项目名称（wan-cpp）
3. 配置 C++ 标准（C++17）
4. 设置库输出目录和运行时输出目录
5. 添加 GGML 子模块
6. 添加第三方依赖目录（thirdparty）
7. 配置编译选项（Debug/Release）
8. 添加公共头文件目录（include）

**Verification:**
- [ ] CMakeLists.txt 文件存在
- [ ] CMake 最低版本设置为 3.20+
- [ ] 项目名称正确设置
- [ ] GGML 子模块正确添加
- [ ] 第三方依赖目录正确添加
- [ ] 公共头文件目录配置

---

### Task 2: 配置 GGML 子模块链接

将 GGML 子模块链接到主项目。

**Requirements:** BUILD-02

**Steps:**
1. 检查 GGML 子模块是否已添加
2. 确保 GGML 目标可用
3. 配置正确的目标属性（PUBLIC/PRIVATE）

**Verification:**
- [ ] GGML 子模块可被发现
- [ ] GGML 目标链接正确
- [ ] 后端选项正确传递给 GGML

---

### Task 3: 配置后端支持

配置各种硬件后端的编译选项。

**Requirements:** BUILD-03

**Steps:**
1. 定义 CUDA 后端选项（GGML_CUDA）
2. 定义 Metal 后端选项（GGML_METAL）
3. 定义 Vulkan 后端选项（GGML_VULKAN）
4. 定义 OpenCL 后端选项（GGML_OPENCL）
5. 定义 SYCL 后端选项（GGML_SYCL）
6. 配置后端专用编译定义
7. 根据平台默认选择合适的后端

**Verification:**
- [ ] 所有后端选项正确定义
- [ ] 后端定义传递给 GGML 正确
- [ ] 后端相关依赖正确配置

---

### Task 4: 配置第三方依赖

配置第三方库依赖（JSON、ZIP 等）。

**Requirements:** BUILD-04

**Steps:**
1. 定位第三方依赖文件（json.hpp、zip.h、stb_image.h 等）
2. 将依赖文件添加到 CMakeLists.txt
3. 配置依赖的包含路径
4. 确保依赖正确链接到主库

**Verification:**
- [ ] 所有第三方依赖文件正确定位
- [ ] 依赖正确添加到构建配置
- [ ] 依赖链接正确

---

### Task 5: 创建库目标

创建主 wan-cpp 库目标。

**Steps:**
1. 添加所有源文件（src/*.cpp、src/*.hpp）
2. 配置库类型（静态/动态）
3. 设置包含目录（include）
4. 链接 GGML 和第三方依赖
5. 配置编译选项

**Verification:**
- [ ] 库目标正确创建
- [ ] 所有源文件正确添加
- [ ] 包含目录正确配置
- [ ] 依赖正确链接

---

### Task 6: 配置编译选项

配置 Debug/Release 和其他编译选项。

**Steps:**
1. 定义 BUILD_SHARED_LIBS 选项
2. 配置编译器标志
3. 设置优化级别
4. 添加测试选项（如果需要）

**Verification:**
- [ ] 编译选项正确配置
- [ ] Debug/Release 模式可正常切换

---

## Success Criteria

Phase 2 的成功标准：

1. CMakeLists.txt 在 Linux 上成功编译（CPU 后端）
2. CMakeLists.txt 在 Linux 上成功编译（CUDA 后端，如果可用）
3. CMakeLists.txt 在 macOS 上成功编译（Metal 后端）
4. CMakeLists.txt 在 Linux 上成功编译（Vulkan 后端，如果可用）
5. CMakeLists.txt 在 Windows 上成功编译（MSVC）
6. 第三方依赖（json.hpp、zip.h）正确配置和链接
7. 构建产生静态库文件（libwan.a 或 wan.lib）

---

## Notes

- 当前工作目录：`/home/jtzhuang/projects/stable-diffusion.cpp/wan`
- CMake 版本要求：3.20+
- GGML 子模块位置：`ggml/`
- 第三方依赖位置：`thirdparty/`
- 目标平台：Linux、macOS、Windows
