---
status: verifying
trigger: "Video output not playable - generated output.avi cannot be played in standard players"
created: 2026-03-26T06:14:43Z
updated: 2026-03-26T06:21:45Z
---

## Current Focus

hypothesis: CONFIRMED - VAE decoder output tensor dimensions misinterpreted
test: VAE outputs [W, H, 3, 1] but code reads as [W, H, T, C]
expecting: T_out gets 3 instead of actual frame count, creating invalid frame buffers
next_action: Fix dimension reading to properly handle VAE output shape

## Symptoms

expected: Generate playable output.avi file that works in standard video players
actual: Generated output.avi file cannot be played
errors: No explicit error messages (program may complete normally)
reproduction: Run `./build_cuda/bin/wan-cli ./models/config.json -p "a cat is playing basketball" --vocab-dir ./models/google/umt5-xxl/ -o output.avi` and attempt to play output.avi
started: Unknown if feature ever worked

## Eliminated

## Evidence

- timestamp: 2026-03-26T06:16:29Z
  checked: VAE decoder test in vae.hpp line 759
  found: Test shows input `z{1, 4, 8, 8}` which is [W, H, C, N] format, output is [W, H, 3, 1]
  implication: VAE decoder outputs single frame in [W, H, 3, 1] format (WHCN)

- timestamp: 2026-03-26T06:16:29Z
  checked: Frame extraction code in wan-api.cpp lines 965-967
  found: `T_out = decoded->ne[2]` reads dimension 2 as time, but VAE outputs [W, H, 3, 1]
  implication: T_out gets 3 (channel count) instead of actual frame count

- timestamp: 2026-03-26T06:16:29Z
  checked: Frame buffer allocation in wan-api.cpp line 968
  found: `frame_bufs(T_out, ...)` creates T_out buffers where T_out=3
  implication: Only 3 frame buffers created instead of actual num_frames (16 by default)

- timestamp: 2026-03-26T06:16:29Z
  checked: VAE input tensor in wan-api.cpp line 775
  found: `x = ggml_new_tensor_4d(denoise_ctx, GGML_TYPE_F32, lW, lH, lT, latent_channels)`
  implication: Input is [lW, lH, lT, latent_channels] but VAE decoder doesn't handle time dimension properly

- timestamp: 2026-03-26T06:16:29Z
  checked: Root cause
  found: VAE decoder is designed for single images [W, H, 3, 1], not video [W, H, T, 3]
  implication: Video latent tensor [lW, lH, lT, 16] needs to be decoded frame-by-frame or VAE needs video support

## Resolution

root_cause: VAE decoder outputs [W, H, 3, 1] but code reads dimensions as [W, H, T, C]. T_out gets 3 instead of num_frames, creating only 3 frame buffers instead of actual frame count. Video latent tensor needs frame-by-frame decoding.
fix: Modified both T2V and I2V functions to decode video frame-by-frame. Extract each frame from latent tensor [lW, lH, lT, 16] as [lW, lH, 16], decode separately, then convert to uint8 RGB frames.
verification: Code changes applied to wan-api.cpp lines 949-987 (T2V) and 1279-1317 (I2V)
files_changed: [src/api/wan-api.cpp]
