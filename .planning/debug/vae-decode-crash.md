---
status: awaiting_human_verify
trigger: VAE decode SIGFPE crash - division by zero in ggml_conv_3d at line 4637
created: 2026-03-26T14:30:00Z
updated: 2026-03-26T14:46:00Z
---

## Current Focus
hypothesis: VAE decode expects 4D tensor [b*c, t, h, w] but frame-by-frame extraction creates [b*c, 1, h, w] which causes div-by-zero in conv3d
test: Examining VAE::decode implementation (wan.hpp:1058-1087) and decoder3d forward path
expecting: Find where dimension mismatch causes im2col->ne[3] to become zero
next_action: Trace through decode and decoder3d to find what conv3d expects vs what we're passing

## Symptoms
expected: VAE decode should process latent tensors without arithmetic errors
actual: SIGFPE in ggml_conv_3d with division by zero at im2col->ne[3] / N
errors: SIGFPE (Arithmetic exception) in ggml_conv_3d at line 4637
reproduction: |
  1. Run T2V or I2V generation with modified frame-by-frame extraction
  2. VAE decoder processes single frame latent
  3. Crash occurs in ggml_conv_3d during convolution operation
started: After previous fix to extract frame_latent[lW, lH, 16] from x[lW, lH, lT, 16]

## Eliminated

## Evidence

- timestamp: 2026-03-26T14:35:00Z
  checked: VAE::decode function signature and implementation (wan.hpp:1058-1087)
  found: |
    - Expects input z: [b*c, t, h, w] (4D tensor with time dimension)
    - Iterates over z->ne[2] (time dimension) frame by frame
    - For each frame i, slices x to get [b*c, 1, h, w] (single frame)
    - Decoder3d::forward is called with this single frame
  implication: Frame extraction is correct in VAE::decode. The crash must occur in Decoder3d when processing single-frame input

- timestamp: 2026-03-26T14:36:00Z
  checked: CausalConv3d::forward function (wan.hpp:56-82)
  found: |
    - Expects x: [N*IC, ID, IH, IW] (4D tensor)
    - Calls ggml_ext_conv_3d which internally does im2col operation
    - im2col->ne[3] is used in division at line 4637
    - CausalConv3d can handle 1-frame input (ID=1) in theory
  implication: Need to check if issue is with how conv3d is being called or if input tensor is malformed

- timestamp: 2026-03-26T14:37:00Z
  checked: Previous fix context - looking for where frame_latent is extracted
  found: |
    - User mentioned: "从x[lW, lH, lT, 16]提取帧为frame_latent[lW, lH, 16]（3D）"
    - This is extracting a single frame as 3D instead of 4D with T=1
    - Problem: decoder expects [b*c, 1, h, w] but code is creating [lW, lH, 16] (no time dimension)
  implication: The REAL issue is that the frame extraction code is removing the time dimension instead of reducing it to 1

- timestamp: 2026-03-26T14:40:00Z
  checked: Located frame extraction code in wan-api.cpp at lines 964 and 1293
  found: |
    - Line 964 (T2V code): ggml_new_tensor_3d(vae_ctx, GGML_TYPE_F32, lW, lH, latent_channels)
    - Line 1293 (I2V code): ggml_new_tensor_3d(vae_ctx, GGML_TYPE_F32, lW, lH, latent_channels)
    - Both create 3D tensors when VAE::decode expects 4D with time dimension
    - Tensor layout mismatch: [lW, lH, 16] vs [16, 1, lH, lW] (GGML convention)
  implication: This is definitely the root cause - VAE conv3d receives wrong tensor shape

- timestamp: 2026-03-26T14:42:00Z
  checked: Applied fix to both locations in wan-api.cpp
  found: |
    - Changed ggml_new_tensor_3d to ggml_new_tensor_4d
    - Added time dimension (1) to tensor shape
    - Updated pointer indexing: old was (w*nb[0] + h*nb[1] + c*nb[2])
    - New is (w*nb[0] + h*nb[1] + 0*nb[2] + c*nb[3])
    - Now frame_latent: [lW, lH, 1, latent_channels] (4D as expected)
  implication: VAE decoder should now receive properly shaped tensor and conv3d can compute im2col correctly

## Resolution
root_cause: Line 964 and 1293 in wan-api.cpp create 3D tensor frame_latent[lW, lH, 16] but VAE::decode expects 4D tensor [b*c, t, h, w]. When passed to conv3d, the missing time dimension causes im2col computation to fail with division by zero at line 4637 of ggml_conv_3d.
fix: Changed frame_latent from ggml_new_tensor_3d to ggml_new_tensor_4d with time dimension = 1. Updated tensor creation from (vae_ctx, GGML_TYPE_F32, lW, lH, latent_channels) to (vae_ctx, GGML_TYPE_F32, lW, lH, 1, latent_channels) and adjusted indexing in dst pointer calculation to include the time dimension (0*frame_latent->nb[2]) and channel (c*frame_latent->nb[3]).
verification: |
  - Build successful: no compilation errors in wan-api.cpp
  - Code change verified in both locations (lines 965 and 1295)
  - Tensor shape now matches VAE::decode expectations
  - Ready for runtime testing
files_changed: ["src/api/wan-api.cpp"]
