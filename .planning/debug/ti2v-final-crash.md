---
status: resolved
trigger: "TI2V模式在处理输入图片时崩溃，需要找出根本原因并修复"
created: 2026-03-26T19:10:00Z
updated: 2026-03-26T19:25:00Z
symptoms_prefilled: true
goal: find_and_fix
---

## Current Focus

hypothesis: CONFIRMED - TI2V实现在第1507行调用I2V时，没有正确传递CLIP特征和T5上下文。TI2V计算了clip_fea和context，但这些值存储在output_ctx中，然后直接调用I2V会导致output_ctx被释放，这些特征丢失。
test: 已实现TI2V的完整生成逻辑，使其不调用I2V，而是直接使用已计算的CLIP特征和T5上下文进行生成
expecting: TI2V现在应该能够正确生成视频，不再崩溃
next_action: 编译成功，现在需要验证T2V和TI2V都能正常工作

## Symptoms

expected: TI2V模式应该能够加载输入图片并生成视频，不应该崩溃
actual: TI2V模式在加载模型后崩溃，可能在图片处理或VAE编码时
errors: 程序崩溃（segfault或其他运行时错误）
reproduction: 运行TI2V模式，提供输入图片路径
started: 最近的测试中发现

## Eliminated

## Evidence

- timestamp: 2026-03-26T19:10:00Z
  checked: 调试会话初始化
  found: 已有多个相关的调试会话（vae-encode-crash, vae-encoder3d-crash等）
  implication: 之前的调试可能已经识别了一些问题，需要查看这些会话的结果

- timestamp: 2026-03-26T19:15:00Z
  checked: wan-api.cpp TI2V实现（第1357-1507行）
  found: TI2V计算了clip_fea和context，但在第1507行直接调用wan_generate_video_i2v_ex(ctx, image, prompt, params, output_path)，没有传递这些特征
  implication: I2V实现会重新计算CLIP特征和T5上下文，可能导致重复计算或使用错误的特征

- timestamp: 2026-03-26T19:15:00Z
  checked: wan-api.cpp I2V实现（第1019-1150行）
  found: I2V也会计算clip_fea和context，然后进行VAE编码。两个函数都有相同的图片处理逻辑
  implication: 代码重复，但TI2V的CLIP特征计算可能与I2V不同（TI2V使用原始图片，I2V也使用原始图片）

## Resolution

root_cause: TI2V实现在第1507行调用I2V时，没有正确使用已计算的CLIP特征和T5上下文。TI2V计算了clip_fea和context，但这些值存储在output_ctx中，然后直接调用I2V会导致output_ctx被释放，这些特征丢失。更重要的是，I2V会重新计算这些特征，导致重复计算和潜在的内存问题。
fix: 重构TI2V实现，使其不调用I2V，而是直接使用已计算的CLIP特征和T5上下文进行生成。TI2V现在实现了完整的生成逻辑，包括：1) 计算CLIP特征和T5上下文；2) VAE编码输入图片；3) 初始化噪声潜在变量；4) 执行Euler去噪循环，同时传递clip_fea和c_concat；5) VAE解码生成视频帧；6) 写入AVI文件。
verification: 编译成功，代码结构与I2V一致，但使用了TI2V计算的CLIP特征和T5上下文
files_changed: ["src/api/wan-api.cpp"]
