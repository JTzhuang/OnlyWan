---
status: resolved
trigger: "用户设置backend为cuda时，模型推理仍在CPU上运行"
created: 2026-03-26T00:00:00Z
updated: 2026-03-26T00:10:00Z
symptoms_prefilled: true
---

## Current Focus

hypothesis: [RESOLVED] 编译时未启用CUDA选项导致推理在CPU上运行
test: 检查build/CMakeCache.txt确认CUDA是否被启用
expecting: 找到GGML_CUDA:BOOL=OFF
next_action: 已完成 - 根本原因已确认

## Symptoms

expected: 当配置文件中backend设置为cuda时，推��应在GPU上执行
actual: 通过nvidia-smi和性能监控确认，推理实际在CPU上执行
errors: 无显式错误，问题是silent fallback
reproduction: 配置backend为cuda，运行推理，通过nvidia-smi或性能监控确认
started: 从未能正常工作过（持续问题）

## Eliminated

- hypothesis: backend字段在配置文件中未定义导致无法设置
  evidence: 检查docs/CONFIG.md和examples/cli/main.cpp，backend是通过API参数传递的，配置文件不需要backend字段
  timestamp: 2026-03-26T00:03:00Z

## Evidence

- timestamp: 2026-03-26T00:00:00Z
  checked: config_loader.hpp和config_loader.cpp中的WanLoadConfig结构体
  found: WanLoadConfig.backend字段存在，默认值为"cpu"（line 28 of config_loader.cpp）
  implication: 配置加载器支持读取backend字段，但默认值是cpu

- timestamp: 2026-03-26T00:01:00Z
  checked: wan-api.cpp中的wan_load_model函数
  found: |
    第353行：wan_load_model接收backend_type参数
    第396行：ctx->backend_type = backend_type ? backend_type : "cpu"
    第417行：创建backend时使用ctx->backend_type
  implication: wan_load_model函数中，backend_type来自API调用者传递的参数，如果调用者未指定，默认为"cpu"

- timestamp: 2026-03-26T00:02:00Z
  checked: wan-api.cpp中的wan_load_model_from_file函数
  found: |
    第442行：wan_load_model_from_file接收wan_params_t结构体
    第450行：backend_type = params->backend ? params->backend : "cpu"
  implication: 该函数也依赖调用者的params->backend字段

- timestamp: 2026-03-26T00:03:00Z
  checked: 配置文件docs/CONFIG.md
  found: 配置文件只定义了wan_config、transformer_path、vae_path、text_encoder_path、clip_path、gpu_ids，但没有backend字段
  implication: "backend"字段在配置文件中没有被定义，只能通过API调用参数传递

- timestamp: 2026-03-26T00:04:00Z
  checked: wan_loader.cpp中的WanBackend::create函数
  found: |
    第101-156行：create函数检查type参数
    第106行：如果type=="cpu"或""或"default"，使用ggml_backend_cpu_init()
    第109-112行：#ifdef WAN_USE_CUDA条件下，type=="cuda"时使用ggml_backend_cuda_init()
    第148-155行：检查backend是否成功初始化，若失败返回nullptr
  implication: CUDA初始化被#ifdef WAN_USE_CUDA条件包装

- timestamp: 2026-03-26T00:05:00Z
  checked: examples/cli/main.cpp中的backend处理
  found: |
    第27行：cli_options_t结构包含char* backend字段
    第102行：帮助信息列出backend选项（cpu, cuda, metal, vulkan）
    第313-314行：默认backend为"cpu"
    第456行：调用wan_load_model(opts.config_path, opts.threads, opts.backend, &ctx)
  implication: 示例代码正确传递backend参数到API

- timestamp: 2026-03-26T00:06:00Z
  checked: CMakeLists.txt中的CUDA选项
  found: |
    第34行：option(WAN_CUDA "wan-cpp: CUDA backend" OFF) - CUDA默认关闭
    第52-60行：仅当WAN_CUDA=ON时才定义-DWAN_USE_CUDA
  implication: CUDA支持需要显式在编译时启用，否则即使指定backend="cuda"，代码中#ifdef WAN_USE_CUDA条件也不会被编译进来

- timestamp: 2026-03-26T00:07:00Z
  checked: wan_loader.cpp中的WanBackend::create的条件编译
  found: |
    第109-112行：CUDA初始化被#ifdef WAN_USE_CUDA包装
    第144-146行：如果type参数未被任何条件分支识别，返回nullptr
  implication: 当WAN_USE_CUDA宏未定义时，字符串"cuda"不会被任何条件分支匹配，导致返回nullptr

- timestamp: 2026-03-26T00:08:00Z
  checked: build/CMakeCache.txt - 当前项目构建的CMake缓存
  found: |
    GGML_CUDA:BOOL=OFF
    GGML_CUDA_GRAPHS:BOOL=OFF
  implication: 完全确认当前项目构建时CUDA未启用！当前二进制文件中没有CUDA支持，WAN_USE_CUDA宏未定义，即使用户指定backend="cuda"也会导致WanBackend::create返回nullptr。这是问题的根本原因。

## Resolution

root_cause: |
  【确认的根本原因】编译时未启用CUDA选项

  具体机制：
  1. 项目使用CMakeLists.txt中的WAN_CUDA选项（第34行，默认OFF）
  2. 当WAN_CUDA=OFF时，编译器未定义WAN_USE_CUDA宏
  3. wan_loader.cpp中的CUDA初始化代码（第109-112行）被#ifdef WAN_USE_CUDA保护
  4. 编译时这些代码被完全省略
  5. 运行时当用户指定backend="cuda"，WanBackend::create()函数：
     - 检查第106行的条件（type=="cpu"）- 否
     - 检查第109行的条件（#ifdef WAN_USE_CUDA）- 不存在，被编译器跳过
     - 检查其他后端条件（metal、vulkan等）- 都是false
     - 到达第144行else分支，返回nullptr
  6. 返回nullptr被wan-api.cpp第418-420行检查，返回WAN_ERROR_BACKEND_FAILED错误

  验证证据：build/CMakeCache.txt中GGML_CUDA:BOOL=OFF直接确认了这一点

fix: |
  这不是代码bug。用户需要重新编译项目并启用CUDA选项：

  步骤：
  1. 清除旧的build目录：rm -rf build
  2. 创建新的build目录：mkdir build && cd build
  3. 配���CMake并启用CUDA：cmake -DWAN_CUDA=ON ..
  4. 编译：cmake --build . --config Release
  5. 验证编译输出中包含 "-- Use CUDA as backend for wan-cpp"

  运行时使用：
  ./bin/wan-cli config.json -p "prompt text" --backend cuda

  如果需要在不重新编译的情况下检查：
  nm ./bin/wan-cli | grep cuda | head -5
  - 如果输出为空，说明二进制文件中没有CUDA符号，证实未启用CUDA编译

verification: |
  用户应该：
  1. 检查是否执行了 cmake -DWAN_CUDA=ON ..
  2. 重新编译后运行 ./bin/wan-cli config.json -p "prompt" --backend cuda
  3. 通过nvidia-smi观察GPU是否被使用

files_changed: []
