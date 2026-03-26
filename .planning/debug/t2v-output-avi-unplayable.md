---
status: verifying
trigger: "T2V生成完成但output.avi无法打开"
created: 2026-03-26T07:43:27Z
updated: 2026-03-26T07:46:00Z
---

## Current Focus

hypothesis: Tensor dimension indexing is wrong. Decoder outputs [N, 3, H, W] but code assumes [W, H, T, C]
test: Verify actual tensor shape from VAE decoder output
expecting: Confirm tensor is [W, H, 3, N] not [W, H, N, 3], causing wrong frame extraction
next_action: Fix tensor indexing in frame extraction loop

## Symptoms

expected: Generated output.avi should be playable in video player
actual: Program completes without crash, output.avi is created but cannot be opened
errors: File cannot be opened in player (unplayable)
reproduction: Run T2V generation to completion
started: Unknown (likely recent)

## Eliminated

(none yet)

## Evidence

- timestamp: 2026-03-26T07:44:33Z
  checked: avi_writer.c implementation
  found: AVI writer uses uncompressed DIB format (24-bit RGB), not MJPG. Frame data written as raw RGB bytes.
  implication: Frame data format is correct, but need to verify frame_bufs is populated correctly

- timestamp: 2026-03-26T07:44:33Z
  checked: wan-api.cpp frame population logic (lines 969-975)
  found: Frames extracted from decoded tensor using ggml_get_f32_nd(decoded, w, h, t, c) and converted to uint8_t
  implication: Data extraction looks correct, but need to verify decoded tensor has valid data

- timestamp: 2026-03-26T07:44:33Z
  checked: AVI file structure in avi_writer.c
  found: RIFF header, LIST hdrl with avih and strl chunks, LIST movi with frame data. Sizes calculated and patched correctly.
  implication: AVI structure looks valid. Issue likely in frame data or tensor content

- timestamp: 2026-03-26T07:45:00Z
  checked: VAE Decoder output shape (vae.hpp line 480)
  found: Decoder returns [N, out_ch, h*8, w*8] = [N, 3, H, W] where N=num_frames, out_ch=3
  implication: Tensor layout is [W, H, 3, N] in GGML (row-major), NOT [W, H, N, 3]

- timestamp: 2026-03-26T07:45:00Z
  checked: Frame extraction code (wan-api.cpp lines 965-975)
  found: Code reads T_out=decoded->ne[2], W=decoded->ne[0], H=decoded->ne[1], then accesses ggml_get_f32_nd(decoded, w, h, t, c)
  implication: BUG FOUND - ne[2] is 3 (channels), not T_out (frames). Should be ne[3]=N (frames). Accessing wrong dimension!

## Resolution

root_cause: Tensor dimension indexing bug. VAE decoder outputs [W, H, 3, N] but code reads T_out from ne[2] (which is 3, the channel count) instead of ne[3] (which is N, the frame count). This causes T_out=3, creating only 3 frames instead of actual frame count, and accessing wrong tensor indices for frame data.
fix: Changed ne[2] to ne[3] for T_out, and reordered tensor access from ggml_get_f32_nd(decoded, w, h, t, c) to ggml_get_f32_nd(decoded, w, h, c, t) in both T2V and I2V generation functions
verification: pending
files_changed: [src/api/wan-api.cpp]
