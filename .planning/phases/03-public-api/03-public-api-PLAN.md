# Plan: Phase 3 - Public API

**Phase:** 3 - Public API
**Status:** Ready
**Created:** 2026-03-12

---

<metadata>
<dependencies>
- Phase 2 (Build System)
</dependencies>
<files_modified>
</files_modified>
<autonomous>false</autonomous>
</metadata>

## Summary

通过 C 风格公共 API 暴露 WAN 功能，实现语言互操作性（ABI 稳定性）。

## Requirements

| ID | Requirement | Status |
|----|-------------|--------|
| API-01 | 创建 C 风格公共头文件（wan.h） | Pending |
| API-02 | 实现模型加载接口 | Pending |
| API-03 | 实现文本生成视频（T2V）接口 | Pending |
| API-04 | 实现图像生成视频（I2V）接口 | Pending |
| API-05 | 实现配置参数接口 | Pending |

---

## Tasks

### Task 1: 创建 C 风格公共头文件

创建 `include/wan-cpp/wan.h`，定义 C 风格 API 声明。

**Requirements:** API-01

**Steps:**
1. 创建 `include/wan-cpp/` 目录
2. 创建 `wan.h` 文件，包含：
   - 前向声明（`extern "C"`）
   - Opaque 句柄类型定义（`typedef struct wan_context wan_context_t`）
   - 模型加载函数声明
   - 文本到视频生成函数声明
   - 图像到视频生成函数声明
   - 参数配置函数声明
   - 释放函数声明
   - 错误码定义
3. 确保函数命名使用 `wan_` 前缀避免冲突

**Verification:**
- [ ] `include/wan-cpp/wan.h` 文件存在
- [ ] 所有必要的函数声明都包含
- [ ] 使用 `extern "C"` 声明导出

---

### Task 2: 实现模型加载接口

实现从 GGUF 文件加载 WAN 模型的功能。

**Requirements:** API-02

**Steps:**
1. 在 `src/` 中创建 `wan_loader.cpp`
2. 实现加载函数，包装已提取的 model.cpp 中的 WAN 加载逻辑
3. 处理 WAN2.1 和 WAN2.2 版本检测
4. 正确处理 GGML backend 初始化
5. 提供清晰的错误消息

**Verification:**
- [ ] `src/wan_loader.cpp` 文件存在
- [ ] 可以成功加载 WAN 模型文件
- [ ] 版本检测逻辑正确

---

### Task 3: 实现文本生成视频（T2V）接口

实现从文本提示生成视频的接口。

**Requirements:** API-03

**Steps:**
1. 在 `src/` 中创建 `wan_t2v.cpp`
2. 实现文本到视频生成函数
3. 支持 CFG scale、采样步数等参数
4. 调用已提取的 WAN 类进行推理

**Verification:**
- [ ] `src/wan_t2v.cpp` 文件存在
- [ ] 函数签名正确
- [ ] 能够生成视频

---

### Task 4: 实现图像生成视频（I2V）接口

实现从图像提示生成视频的接口。

**Requirements:** API-04

**Steps:**
1. 在 `src/` 中创建 `wan_i2v.cpp`
2. 实现图像到视频生成函数
3. 支持输入图像、条件帧等参数
4. 调用已提取的 WAN 类进行推理

**Verification:**
- [ ] `src/wan_i2v.cpp` 文件存在
- [ ] 函数签名正确
- [ ] 能够生成视频

---

### Task 5: 实现配置参数接口

实现生成参数配置功能。

**Requirements:** API-05

**Steps:**
1. 在 `src/` 中创建 `wan_config.cpp`
2. 实现参数设置函数
3. 支持常用参数（seed、steps、width、height 等）
4. 提供参数验证功能

**Verification:**
- [ ] `src/wan_config.cpp` 文件存在
- [ ] 所有必要的参数设置函数都实现
- [ ] 参数验证逻辑正确

---

## Success Criteria

Phase 3 的成功标准：

1. C 头文件（wan.h）存在且稳定，所有函数声明完整
2. 用户可以使用 API 加载 GGUF 文件中的 WAN 模型
3. 用户可以使用 API 生成文本到视频
4. 用户可以使用 API 生成图像到视频
5. 用户可以通过 API 配置生成参数
6. 库可以链接到 C、C++，并具有不透明句柄模式便于语言绑定

---

## Notes

- 当前工作目录：`/home/jtzhuang/projects/stable-diffusion.cpp/wan`
- CMakeLists.txt 已在 Phase 2 中配置好
- WAN 核心代码已在 Phase 1 中提取
- 需要确保新的 API 与已提取的代码兼容

