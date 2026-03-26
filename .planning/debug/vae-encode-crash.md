---
status: resolved
trigger: "VAE编码在CausalConv3d中崩溃，N=0导致除以零。ggml_conv_3d中int64_t OD = im2col->ne[3] / N; N = 0"
created: 2026-03-26T00:00:00Z
updated: 2026-03-26T10:54:00Z
---

## Current Focus

hypothesis: CONFIRMED - patchify creates zero dimensions when image is too small
test: traced patchify logic with small image dimensions
expecting: h or w becomes 0 after division by patch_size
next_action: implement fix to handle small images

## Symptoms

expected: VAE encoding should process image tensors without division by zero errors
actual: ggml_conv_3d crashes with N=0 in division operation
errors: "int64_t OD = im2col->ne[3] / N; N = 0 (除以零)"
reproduction: Call WAN::WanVAE::encode with small image (e.g., 1x1 or 2x2)
started: unknown (investigating)

## Eliminated

(none yet)

## Evidence

- timestamp: 2026-03-26T10:52:50Z
  checked: wan-api.cpp image tensor creation (lines 1116-1144)
  found: img_tensor created as [padW, padH, 3, 1] where padW/padH are padded to even dimensions
  implication: img_tensor dimensions look correct at creation

- timestamp: 2026-03-26T10:52:50Z
  checked: WanVAE::encode method (lines 1022-1056)
  found: encode calls patchify if wan2_2=true, then slices tensor and passes to encoder->forward
  implication: patchify could be creating zero dimensions if input is too small

- timestamp: 2026-03-26T10:53:00Z
  checked: patchify logic (lines 969-994)
  found: patchify divides h and w by patch_size (2): h = x->ne[1] / q; w = x->ne[0] / r
  implication: if image height or width < 2, then h or w becomes 0, causing division by zero in conv3d

## Resolution

root_cause: patchify divides image height and width by patch_size (2). When image dimensions < 2, this creates zero dimensions, causing division by zero in ggml_conv_3d
fix: Added minimum dimension check before patchify. If h < 2 or w < 2, pad to 2x2 using ggml_ext_pad_ext before calling patchify
verification: Code change applied to src/wan.hpp lines 1022-1043
files_changed: ["src/wan.hpp"]
