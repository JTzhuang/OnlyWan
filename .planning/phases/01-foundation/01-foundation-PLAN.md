# Plan: Phase 1 - Foundation

**Phase:** 1 - Foundation
**Status:** Ready
**Created:** 2026-03-12

---

<metadata>
<dependencies>
- None
</dependencies>
<files_modified>
</files_modified>
<autonomous>false</autonomous>
</metadata>

## Summary

建立完整的项目结构并提取 WAN 核心代码，正确管理依赖。这是项目的第一阶段，为后续的构建系统和 API 开发奠定基础。

## Requirements

| ID | Requirement | Status |
|----|-------------|--------|
| STRUCT-01 | 创建独立的目录结构（include、src、examples、thirdparty、ggml 子模块） | Pending |
| STRUCT-02 | 配置 .gitmodules 引用 ggml 子模块 | Pending |
| STRUCT-03 | 添加 .gitignore 和基础 README | Pending |
| CORE-01 | 提取 wan.hpp 及其依赖文件（common_block.hpp、rope.hpp、vae.hpp、flux.hpp） | Pending |
| CORE-02 | 提取图像预处理功能（preprocessing.hpp） | Pending |
| CORE-03 | 提取工具函数（util.h/cpp） | Pending |
| CORE-04 | 提取模型加载相关代码（model.h/cpp 中的 WAN 部分） | Pending |
| CORE-05 | 修正所有相对包含路径以适应新结构 | Pending |

---

## Tasks

### Task 1: 创建项目目录结构

创建独立的 C++ 库目录结构，包括源代码、头文件、示例和第三方依赖目录。

**Requirements:** STRUCT-01

**Steps:**
1. 在当前目录（/home/jtzhuang/projects/stable-diffusion.cpp/wan）创建标准目录结构：
   - `include/wan-cpp/` - 公共头文件
   - `src/` - 实现代码
   - `examples/` - 示例程序
   - `thirdparty/` - 第三方依赖
2. 验证所有目录已创建

**Verification:**
- [ ] `include/` 目录存在
- [ ] `src/` 目录存在
- [ ] `examples/` 目录存在
- [ ] `thirdparty/` 目录存在

---

### Task 2: 配置 GGML 子模块

通过 git 子模块方式引用 GGML 依赖，确保版本可追踪。

**Requirements:** STRUCT-02

**Steps:**
1. 在当前目录创建 `.gitmodules` 文件，内容为：
   ```
   [submodule "ggml"]
       path = ../ggml
   ```
2. 初始化 git 子模块：`git submodule add ../ggml`
3. 更新子模块：`git submodule update --init --recursive`

**Verification:**
- [ ] `.gitmodules` 文件存在
- [ ] `ggml/` 目录已克隆
- [ ] GGML 子模块处于正常追踪状态（不是 detached HEAD）

---

### Task 3: 创建基础文档

创建 README.md 和 .gitignore 文件。

**Requirements:** STRUCT-03

**Steps:**
1. 创建 `.gitignore` 文件，忽略以下内容：
   ```
   build/
   bin/
   *.o
   *.a
   *.so
   *.dll
   *.dylib
   *.dSYM
   ```
2. 创建 `README.md` 文件，包含：
   - 项目概述
   - 从 stable-diffusion.cpp 提取说明
   - 编译说明
   - 使用示例

**Verification:**
- [ ] `.gitignore` 文件存在
- [ ] `README.md` 文件存在
- [ ] README.md 包含项目概述

---

### Task 4: 提取 WAN 核心文件

复制 WAN 模型核心代码到新项目。

**Requirements:** CORE-01

**Steps:**
1. 从 `/home/jtzhuang/projects/stable-diffusion.cpp/src/wan.hpp` 复制到 `src/wan.hpp`
2. 从 `/home/jtzhuang/projects/stable-diffusion.cpp/src/common_block.hpp` 复制到 `src/common_block.hpp`
3. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/rope.hpp` 复制到 `src/rope.hpp`
4. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/vae.hpp` 复制到 `src/vae.hpp`
5. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/flux.hpp` 复制到 `src/flux.hpp`
6. 验证所有文件已成功复制

**Note:** 需要修改包含路径以适应新目录结构（在 Task 9 中处理）

**Verification:**
- [ ] `src/wan.hpp` 存在
- [ ] `src/common_block.hpp` 存在
- [ ] `src/rope.hpp` 存在
- [ ] `src/vae.hpp` 存在
- [ ] `src/flux.hpp` 存在

---

### Task 5: 提取公共依赖头文件

提取被 WAN 核心文件依赖的公共头文件。

**Steps:**
1. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/ggml_extend.hpp` 复制到 `src/ggml_extend.hpp`
2. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/common_dit.hpp` 复制到 `src/common_dit.hpp`（如果需要）
3. 验证文件已复制

**Verification:**
- [ ] `src/ggml_extend.hpp` 存在
- [ ] 依赖的头文件已提取

---

### Task 6: 提取图像预处理功能

复制图像预处理相关代码。

**Requirements:** CORE-02

**Steps:**
1. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/preprocessing.hpp` 复制到 `src/preprocessing.hpp`
2. 检查是否有对应的 .cpp 实现文件，如果有则一并复制
3. 验证文件已复制

**Verification:**
- [ ] `src/preprocessing.hpp` 存在
- [ ] 预处理功能代码完整

---

### Task 7: 提取工具函数

复制工具函数和辅助代码。

**Requirements:** CORE-03

**Steps:**
1. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/util.h` 复制到 `src/util.h`
2. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/util.cpp` 复制到 `src/util.cpp`
3. 验证文件已复制

**Verification:**
- [ ] `src/util.h` 存在
- [ ] `src/util.cpp` 存在

---

### Task 8: 提取模型加载相关代码

复制模型加载和版本检测相关的代码（仅 WAN 相关部分）。

**Requirements:** CORE-04

**Steps:**
1. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/model.h` 复制到 `src/model.h`
2. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/model.cpp` 复制到 `src/model.cpp`
3. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/src/gguf_reader.hpp` 复制到 `src/gguf_reader.hpp`
4. 验证文件已复制

**Note:** 可能需要过滤掉非 WAN 相关的代码

**Verification:**
- [ ] `src/model.h` 存在
- [ ] `src/model.cpp` 存在
- [ ] `src/gguf_reader.hpp` 存在

---

### Task 9: 修正包含路径

修改所有提取的文件中的 `#include` 语句，使用正确的相对路径。

**Requirements:** CORE-05

**Steps:**
1. 检查所有 `src/` 下的 `.hpp` 和 `.h` 文件
2. 修正包含路径，确保使用 `"filename"` 而不是 `<filename>`
3. 移除对稳定扩散项目特定路径的引用
4. 使用相对路径 `#include "other_file.hpp"`
5. 验证所有包含路径在新结构下有效

**Verification:**
- [ ] 所有文件的包含路径已修正
- [ ] 没有对外部稳定扩散项目的硬编码引用

---

### Task 10: 复制第三方依赖

复制必要的第三方依赖文件。

**Steps:**
1. 从 `/home/jtzhuang/projects/stable-diffusionser.cpp/thirdparty/` 复制 json.hpp、ordered_map.hpp、zip.h 等到新项目的 `thirdparty/`
2. 验证文件已复制

**Verification:**
- [ ] `thirdparty/json.hpp` 存在
- [ ] `thirdparty/ordered_map.hpp` 存在
- [ ] `thirdparty/zip.h` 存在

---

## Success Criteria

Phase 1 的成功标准：

1. 项目目录结构存在（include、src、examples、thirdparty、.planning）
2. GGML 子模块正确初始化且可追踪（不在 detached HEAD）
3. 所有 WAN 核心文件（wan.hpp、common_block.hpp、rope.hpp、vae.hpp、flux.hpp）已提取且包含路径有效
4. 支持文件（preprocessing.hpp、util.h/cpp、model.h/cpp WAN 组件）已提取
5. 所有宏名称使用 WAN 特定前缀以避免与 stable-diffusion.cpp 冲突
6. README.md 提供基本项目概述和使用说明

---

## Notes

- 当前工作目录：`/home/jtzhuang/projects/stable-diffusion.cpp/wan`
- 源代码位置：`/home/jtzhuang/projects/stable-diffusion.cpp`
- 所有相对包含路径需要仔细检查
- GGML 子模块指向父目录的 ggml：`../ggml`
