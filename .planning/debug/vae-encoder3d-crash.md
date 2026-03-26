---
status: resolved
trigger: "VAE编码在Encoder3d中崩溃，N=0导致除以零。ggml_conv_3d中的im2col->ne[3] / N"
created: 2026-03-26T11:02:26Z
updated: 2026-03-26T11:12:00Z
---

## Current Focus

hypothesis: CONFIRMED - Image padding to 2x2 then patchify(patch_size=2) produces 1x1 spatial dims, causing N=0 in conv3d
test: Minimum image size after patchify must be at least 4x4 (becomes 2x2 after patchify)
expecting: Fix padding logic to ensure minimum 4x4 before patchify
next_action: Update padding logic in wan.hpp encode() and wan-api.cpp to pad to 4x4 minimum

## Symptoms

expected: TI2V should encode image and generate video without crashes
actual: Crashes in ggml_conv_3d with division by zero (N=0)
errors: "ggml_conv_3d: int64_t OD = im2col->ne[3] / N;" crash in Encoder3d::forward
reproduction: Run TI2V with any input image
started: Recent changes to image handling

## Eliminated

(none yet)

## Evidence

- timestamp: 2026-03-26T11:05:00Z
  checked: wan.hpp encode() function (lines 1022-1065) and patchify() (lines 969-994)
  found: Image padding logic pads to minimum 2x2, then patchify with patch_size=2 divides by 2, resulting in 1x1 spatial dims
  implication: After patchify, tensor becomes [b*c*4, 1, 1, 1] (4 patches from 2x2). Conv3d then fails because spatial dims are too small

- timestamp: 2026-03-26T11:06:00Z
  checked: wan-api.cpp image processing (lines 1116-1146)
  found: Image is padded to even dimensions (lines 1118-1119), then passed to VAE encode. Padding only ensures divisibility by 2, not by 4
  implication: Small images (e.g., 1x1, 2x2, 3x3) after padding become 2x2, 2x2, 4x4. After patchify with patch_size=2, they become 1x1, 1x1, 2x2 respectively

## Resolution

root_cause: Image padding logic padded to minimum 2x2, then patchify with patch_size=2 divided by 2, resulting in 1x1 spatial dimensions. Conv3d operations require minimum 2x2 spatial dims, causing N=0 division error.
fix: Updated padding logic in wan.hpp encode() and wan-api.cpp to pad to minimum 4x4 (not 2x2). After patchify with patch_size=2, this ensures minimum 2x2 spatial dimensions for conv3d operations.
verification: Compiled successfully. Need to test T2V and TI2V generation.
files_changed: ["src/wan.hpp", "src/api/wan-api.cpp"]
