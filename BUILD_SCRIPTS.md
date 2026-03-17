# WAN-CPP Build Scripts

编译脚本集合，用于构建 WAN-CPP 推理引擎，支持多后端和性能优化。

## 脚本说明

### 1. `build.sh` - 完整编译脚本

功能完整的编译脚本，支持所有后端和编译选项。

**用法：**
```bash
./build.sh [backend] [options]
```

**后端选项：**
- `cuda` - NVIDIA CUDA（默认）
- `metal` - Apple Metal
- `vulkan` - Vulkan
- `opencl` - OpenCL
- `cpu` - CPU only

**编译选项：**
- `-t, --type TYPE` - 编译类型：Debug、Release（默认：Release）
- `-j, --jobs N` - 并行编译数（默认：CPU 核心数）
- `-d, --dir DIR` - 编译目录（默认：build_${backend}）
- `--tests` - 编译测试
- `--no-examples` - 跳过示例编译
- `-v, --verbose` - 详细输出
- `-c, --clean` - 清理后重新编译
- `-h, --help` - 显示帮助

**示例：**
```bash
# CUDA Release 编译（默认）
./build.sh cuda

# Metal Debug 编译
./build.sh metal -t Debug

# CUDA Release + 测试，8 个并行任务
./build.sh cuda --tests -j 8

# 清理后重新编译
./build.sh cuda --clean

# CPU 编译，详细输出
./build.sh cpu -v
```

### 2. `quick-build.sh` - 快速编译脚本

交互式菜单，快速选择常用编译配置。

**用法：**
```bash
./quick-build.sh
```

**菜单选项：**
1. CUDA Release（优化版，默认）
2. CUDA Debug（带调试符号）
3. Metal Release（Apple Silicon）
4. CPU Release（无 GPU）
5. CUDA Release + Tests
6. 清理重建（CUDA Release）
7. 自定义配置
0. 退出

**示例：**
```bash
# 交互式菜单
./quick-build.sh

# 选择选项 1 进行 CUDA Release 编译
```

### 3. `benchmark.sh` - 性能基准测试脚本

测试性能优化的效果。

**用法：**
```bash
./benchmark.sh
```

**功能：**
- 运行性能基准测试
- 测量去噪步延迟
- 测量编码器延迟
- 测量 VAE 延迟
- 保存结果到文件

**配置参数：**
- `STEPS=20` - 去噪步数
- `WIDTH=512` - 宽度
- `HEIGHT=512` - 高度
- `FRAMES=16` - 帧数
- `SEED=42` - 随机种子

**输出：**
- 基准测试结果保存到 `.benchmark/results.txt`
- 生成的视频保存到 `.benchmark/` 目录

## 编译流程

### 快速开始

```bash
# 1. 快速编译（CUDA Release）
./build.sh cuda

# 2. 运行 CLI
cd build_cuda
./bin/wan-cli --help

# 3. 生成视频
./bin/wan-cli --model model.gguf --prompt "a beautiful landscape" --output output.avi
```

### 完整流程

```bash
# 1. 清理旧编译
./build.sh cuda --clean

# 2. 编译 + 测试
./build.sh cuda --tests -j 8

# 3. 运行测试
cd build_cuda
ctest

# 4. 运行基准测试
cd ..
./benchmark.sh

# 5. 安装
cd build_cuda
cmake --install .
```

## 编译选项详解

### 后端选择

**CUDA（推荐用于 NVIDIA GPU）**
```bash
./build.sh cuda
```
- 启用 CUDA Graph 优化
- 启用 Flash Attention
- 最佳性能

**Metal（Apple Silicon）**
```bash
./build.sh metal
```
- 原生 Apple Silicon 支持
- 启用 Metal Performance Shaders

**Vulkan（跨平台）**
```bash
./build.sh vulkan
```
- 支持 Linux、Windows、macOS
- 广泛的 GPU 支持

**OpenCL（通用）**
```bash
./build.sh opencl
```
- 支持大多数 GPU
- 跨平台兼容

**CPU（调试用）**
```bash
./build.sh cpu
```
- 无 GPU 依赖
- 用于调试和测试

### 编译类型

**Release（生产环境）**
```bash
./build.sh cuda -t Release
```
- 优化编译
- 最佳性能
- 较小的二进制文件

**Debug（开发调试）**
```bash
./build.sh cuda -t Debug
```
- 包含调试符号
- 便于调试
- 较大的二进制文件

## 性能优化

编译脚本自动启用以下优化：

| 优化项 | 预期收益 | 状态 |
|--------|---------|------|
| CG-01: 缓冲区持久化 | 2-5x 去噪循环 | ✅ |
| OP-01: Flash Attention | 10-20% attention | ✅ |
| CG-02: CUDA Graph | 10-30% 图执行 | ✅ |
| OP-02: RoPE PE GPU 化 | 5-10% 每步 | ✅ |
| FUS-02: Linear + GELU 融合 | 5-10% FFN | ✅ |

## 输出文件

编译完成后，输出文件位置：

```
build_cuda/
├── bin/
│   ├── wan-cli          # 命令行工具
│   └── wan-convert      # 模型转换工具
├── lib/
│   └── libwan-cpp.a     # 静态库
└── CMakeFiles/          # CMake 文件
```

## 故障排除

### 编译失败

**问题：** CMake 配置失败
```bash
# 解决方案：清理后重新编译
./build.sh cuda --clean
```

**问题：** CUDA 编译错误
```bash
# 检查 CUDA 安装
nvcc --version

# 检查 CUDA 路径
echo $CUDA_PATH
```

**问题：** 内存不足
```bash
# 减少并行任务数
./build.sh cuda -j 2
```

### 运行时错误

**问题：** 找不到 CUDA 库
```bash
# 设置 LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$CUDA_PATH/lib64:$LD_LIBRARY_PATH
```

**问题：** 模型加载失败
```bash
# 检查模型文件
ls -lh models/

# 使用绝对路径
./bin/wan-cli --model /path/to/model.gguf
```

## 高级用法

### 自定义编译目录

```bash
./build.sh cuda -d my_build
cd my_build
```

### 详细编译输出

```bash
./build.sh cuda -v
```

### 并行编译优化

```bash
# 使用所有 CPU 核心
./build.sh cuda -j $(nproc)

# 使用特定数量的核心
./build.sh cuda -j 16
```

### 编译后安装

```bash
cd build_cuda
cmake --install . --prefix /usr/local
```

## 环境变量

可以通过环境变量自定义编译行为：

```bash
# 设置 CUDA 路径
export CUDA_PATH=/usr/local/cuda

# 设置 CMake 生成器
export CMAKE_GENERATOR="Ninja"

# 设置编译器
export CC=gcc
export CXX=g++
```

## 性能测试

运行基准测试验证优化效果：

```bash
./benchmark.sh
```

查看结果：
```bash
cat .benchmark/results.txt
```

## 许可证

这些脚本是 WAN-CPP 项目的一部分，遵循项目许可证。

## 支持

如有问题，请参考：
- `.planning/OPTIMIZATION_TODOS.md` - 优化清单
- `.planning/phases/14-cuda-graph/` - 优化实现文档
- `CMakeLists.txt` - CMake 配置
