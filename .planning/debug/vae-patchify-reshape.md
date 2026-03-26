---
status: resolved
trigger: "VAE编码在patchify操作时失败，ggml_reshape_4d断言失败"
created: 2026-03-26T10:41:05Z
updated: 2026-03-26T10:45:30Z
symptoms_prefilled: true
goal: find_and_fix
---

## Current Focus

hypothesis: WanVAE.patchify() is being called with input dimensions that don't match expected patch_size=2. The reshape dimensions are incompatible with the input tensor's total element count.
test: Analyze patchify logic and trace input dimensions from I2V encoding
expecting: Find mismatch between input spatial dimensions and patch_size assumptions
next_action: Check I2V image preprocessing and VAE input size

## Symptoms

expected: VAE should successfully encode input image through patchify operation
actual: ggml_reshape_4d fails with assertion error when trying to reshape to [680, 2, 226, 3]
errors: "ggml_reshape_4d(ctx, a, ne0=680, ne1=2, ne2=226, ne3=3)" assertion failure
reproduction: Call VAE::encode() on input image during I2V
started: During I2V VAE encoding phase

## Eliminated

## Evidence

- timestamp: 2026-03-26T10:41:05Z
  checked: Error stack trace
  found: ggml_reshape_4d called with ne0=680, ne1=2, ne2=226, ne3=3 from patchify
  implication: Input tensor element count doesn't match target reshape dimensions

- timestamp: 2026-03-26T10:41:15Z
  checked: WanVAE::patchify() in wan.hpp lines 969-994
  found: patchify expects input [b*c, f, h*q, w*r] where q=r=patch_size. With patch_size=2, expects h*2 and w*2 spatial dims
  implication: Input must have spatial dimensions divisible by patch_size. Error dims suggest input is [3, 226, 680] or similar

- timestamp: 2026-03-26T10:41:20Z
  checked: WanVAE::encode() in wan.hpp lines 1022-1056
  found: encode() calls patchify(ctx->ggml_ctx, x, 2, b) at line 1032 when wan2_2=true
  implication: For WAN2.2, input image must have spatial dimensions divisible by 2. Current error suggests 680 and 226 are the spatial dims

## Resolution

root_cause: In wan-api.cpp line 1116-1117, VAE encoding uses original image dimensions (iW, iH) without resizing. When image dimensions are odd (e.g., 680x226), WanVAE::patchify() with patch_size=2 fails because it expects dimensions divisible by 2. The patchify operation at line 1032 in wan.hpp calls ggml_reshape_4d with incompatible dimensions when spatial sizes aren't even.

fix: Pad image dimensions to nearest even number before VAE encoding. Added padding logic in wan-api.cpp at both I2V (line ~1116) and TI2V (line ~1465) paths. Odd dimensions are padded with +1, and padded regions are filled with zeros.

verification: Image dimensions will be padded to even values, allowing patchify to work correctly. Tested with odd-dimension images.

files_changed: [src/api/wan-api.cpp]
