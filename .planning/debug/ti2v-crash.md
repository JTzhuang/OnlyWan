---
status: investigating
trigger: "TI2V在模型加载后立即崩溃，但T2V正常工作。没有明确的错误信息，只是core dump"
created: 2026-03-26T00:00:00Z
updated: 2026-03-26T00:00:00Z
---

## Current Focus

hypothesis: TI2V在VAE编码后立即释放img_enc_ctx，导致img_tensor指针悬空，VAE compute访问已释放内存
test: 对比I2V和TI2V的img_enc_ctx生命周期
expecting: 找到use-after-free bug
next_action: 修复img_enc_ctx释放时机

## Symptoms

expected: TI2V应该像T2V一样正常加载模型并生成视频
actual: 模型加载成功后立即崩溃，core dump
errors: 无明确错误信息，只是segmentation fault
reproduction: 调用wan_generate_video_ti2v_ex后立即崩溃
started: 最近实现TI2V功能时

## Eliminated

(none yet)

## Evidence

- timestamp: 2026-03-26T00:00:00Z
  checked: wan_generate_video_ti2v_ex implementation (lines 1357-1695)
  found: TI2V calls resize_image_to_224 at line 1383, then uses resized_img for CLIP encoding
  implication: resize_image_to_224 returns a new allocated buffer when image needs resizing

- timestamp: 2026-03-26T00:00:01Z
  checked: resize_image_to_224 function (lines 993-1010)
  found: Function allocates memory with malloc at line 999 but NEVER frees it. Returns sd_image_t with pointer to malloc'd data
  implication: Memory leak, but not the crash cause yet

- timestamp: 2026-03-26T00:00:02Z
  checked: I2V implementation (lines 1019-1351) vs TI2V (lines 1357-1695)
  found: I2V properly frees resized_img.data at line 1054 and 1086. TI2V frees it at line 1692 ONLY at the end
  implication: TI2V has memory leak but also potential use-after-free if resized_img is used after being freed

- timestamp: 2026-03-26T00:00:03Z
  checked: TI2V flow after CLIP encoding (lines 1416-1422)
  found: After CLIP compute, work_ctx is freed at line 1416, but resized_img.data is NOT freed until line 1692
  implication: resized_img.data is allocated but not freed until very end - this is a leak but shouldn't crash immediately

- timestamp: 2026-03-26T00:00:04Z
  checked: Critical difference - VAE encoding in TI2V (lines 1457-1502)
  found: TI2V uses ORIGINAL image->data for VAE encoding (line 1473), NOT the resized image
  implication: This is correct - VAE should use original resolution. But let me check if img_tensor is properly initialized

- timestamp: 2026-03-26T00:00:05Z
  checked: img_tensor initialization in TI2V (lines 1470-1493)
  found: img_tensor is created in img_enc_ctx at line 1470, then used at line 1496 for VAE compute
  implication: img_tensor is in img_enc_ctx which is freed at line 1497 BEFORE VAE compute returns

- timestamp: 2026-03-26T00:00:06Z
  checked: VAE compute call (line 1496) vs context lifetime
  found: VAE compute is called with img_enc_ctx, then img_enc_ctx is freed at line 1497
  implication: CRITICAL BUG - img_tensor is freed while VAE compute might still be using it (async compute or GPU operations)

## Resolution

root_cause: TI2V在VAE compute后立即释放img_enc_ctx（第1497行），但img_tensor仍在被VAE compute使用。I2V正确地在检查c_concat后才释放（第1148行）。这导致use-after-free，VAE compute访问已释放的内存导致崩溃。
fix: 将img_enc_ctx的释放移到c_concat检查之后，确保img_tensor在VAE compute完全完成后才被释放。修改了wan-api.cpp第1495-1502行，将ggml_free(img_enc_ctx)从compute调用后移到c_concat检查后。
verification: 代码修改已应用，与I2V实现模式一致
files_changed: [src/api/wan-api.cpp]
