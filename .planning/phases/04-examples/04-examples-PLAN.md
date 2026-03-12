# Plan: Phase 4 - Examples

**Phase:** 4 - Examples
**Status:** Ready
**Created:** 2026-03-12

---

<metadata>
<dependencies>
- Phase 3 (Public API)
</dependencies>
<files_modified>
</files_modified>
<autonomous>false</autonomous>
</metadata>

## Summary

提供生产就绪的示例程序，展示库的完整使用方法。这是最后一个阶段，完成后项目即可投入使用。

## Requirements

| ID | Requirement | Status |
|----|-------------|--------|
| EX-01 | 创建 CLI 示例程序 | Pending |
| EX-02 | 实现视频输出功能（AVI 格式） | Pending |
| EX-03 | 添加基本命令行参数解析 | Pending |
| EX-04 | 创建使用示例文档 | Pending |

---

## Tasks

### Task 1: 创建 CLI 示例程序

创建命令行工具程序，展示库的使用方法。

**Requirements:** EX-01

**Steps:**
1. 在 `examples/cli/` 目录创建 `main.cpp` 文件
2. 包含必要的头文件：
   - `#include <wan-cpp/wan.h>` - 公共 API
   - `#include "common.hpp"` - 工具函数
   - `#include "stable-diffusion.h"` - 主库类型定义
3. 实现基本命令行参数解析
4. 添加主函数入口点
5. 创建 CMakeLists.txt 文件用于编译示例

**Verification:**
- [ ] `examples/cli/main.cpp` 文件存在
- [ ] 包含所有必要的头文件
- [ ] 可以编译链接到主库

---

### Task 2: 实现视频输出功能（AVI 格式）

实现 AVI 格式的视频输出功能。

**Requirements:** EX-02

**Steps:**
1. 从原项目复制 `avi_writer.h` 到 `examples/cli/`
2. 在 `src/` 中创建 `avi_writer.cpp` 实现
3. 确保视频编码功能正确工作

**Verification:**
- [ ] `avi_writer.h` 文件存在
- [ ] `avi_writer.cpp` 文件存在
- [ ] 视频输出功能正常

---

### Task 3: 添加基本命令行参数解析

实现命令行参数解析功能。

**Requirements:** EX-03

**Steps:**
1. 创建 `examples/cli/arg_options.hpp` 或在 `main.cpp` 中实现
2. 支持模型路径、提示词、输出路径等基本参数
3. 支持可选参数（CFG scale、采样步数等）
4. 创建帮助文本生成功能

**Verification:**
- [ ] 参数解析器实现
- [ ] 支持所有基本参数
- [ ] 帮助功能可用

---

### Task 4: 创建使用示例文档

创建 README.md 文档，展示库的典型使用方法。

**Requirements:** EX-04

**Steps:**
1. 在 `examples/` 目录创建 `README.md`
2. 包含以下内容：
   - 项目简介
   - 快速开始指南
   - 基本参数说明
   - 使用示例
   - 编译和安装说明

**Verification:**
- [ ] `examples/README.md` 文文件存在
- [ ] 文档包含所有必要部分

---

## Success Criteria

Phase 4 的成功标准：

1. CLI 示例程序存在且可以编译
2. 视频输出功能（AVI 格式）正常工作
3. 命令行参数解析功能完整
4. 使用文档（README.md）提供清晰的使用指南
5. 示例程序可以链接到主库

---

## Notes

- 当前工作目录：`/home/jtzhuang/projects/stable-diffusion.cpp/wan`
- 依赖：Phase 3 的公共 API 需要已完成
- 这是最后一个阶段，完成后项目即可投入使用
