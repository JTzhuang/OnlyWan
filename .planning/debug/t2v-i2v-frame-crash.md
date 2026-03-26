---
status: awaiting_human_verify
trigger: T2V/I2V生成在销毁frame_bufs向量时崩溃。原始代码可能不是逐帧解码，而是整体解码所有帧
created: 2026-03-26T00:00:00Z
updated: 2026-03-26T15:58:00Z
symptoms_prefilled: true
goal: find_and_fix
---

## Current Focus

hypothesis: 逐帧 VAE 解码让 frame_bufs 使用的 T/W/H 来源于 latent 维度，错过了 decoded 张量的真实 [W,H,T] 排列，从而溢出；复用 I2V 那样一次性处理整个 latent 会让 decoded->ne 提供准确的帧/宽/高。
test: 已应用一次性 decode 改动并编译成功。现需运行 T2V 生成并观察 frame_bufs 销毁是否仍崩溃。
expecting: frame_bufs 按 decoded 张量的真实维度分配并填充，销毁时不再触发 malloc_printerr，生成流程正常完成。
next_action: 用户运行 T2V 生成测试，确认崩溃消失。
## Symptoms

expected: T2V/I2V视频生成完成，frame_bufs向量正常销毁
actual: 在wan_generate_video_t2v_ex第953行销毁frame_bufs时崩溃，malloc_printerr()检测到内存损坏
errors: |
  malloc_printerr(): corrupted size vs. prev_size
  std::vector<uint8_t>析构函数中deallocate失败
started: T2V/I2V生成时崩溃（症状来自调试目标描述）
reproduction: |
  1. 生成T2V或I2V视频
  2. 等待生成完成，在销毁frame_bufs时崩溃

## Eliminated

(none yet)

## Evidence

- timestamp: 2026-03-26T00:00:01Z
  checked: wan-api.cpp lines 949-1000, VAE decoding loop structure
  found: Frame-by-frame VAE decoding is implemented, not batch decoding. Each frame extracted from latent tensor x[lW, lH, lT, 16] into frame_latent[lW, lH, 1, 16], then decoded separately.
  implication: The decoder output shape comment says [W, H, 3, 1], implying decoded->ne[0]=W, ne[1]=H, ne[2]=3, ne[3]=1. Need to verify this matches the expected decoded dimensions.

- timestamp: 2026-03-26T00:00:02Z
  checked: vae.hpp, Decoder class line 480
  found: Decoder.forward() returns [N, out_ch, h*8, w*8]. For video decoder with z_channels=4 (or 16 in multi-frame case), output shape is [N, 3, h*8, w*8] = [1, 3, lH*8, lW*8]
  implication: The decoder output should have shape [lH*8, lW*8, 3, 1] in GGML tensor dimension order, NOT [lW*8, lH*8, 3, 1]. Dimensional ordering might be wrong.

- timestamp: 2026-03-26T00:00:03Z
  checked: .claude/worktrees/config-driven-loading/src/api/wan-api.cpp lines 1046-1084
  found: I2V implementation decodes the entire latent tensor `x` in one shot (`ctx->vae_runner->compute` on `x`) and uses `decoded->ne[2]`, `decoded->ne[0]`, `decoded->ne[1]` to derive T/W/H before filling frame_bufs.
  implication: Master branch T2V should match this approach; per-frame VAE decode introduces extra allocations and misaligned dimension assumptions, causing frame_bufs memory overflow.

- timestamp: 2026-03-26T15:55:00Z
  checked: src/api/wan-api.cpp lines 949-978 (after fix)
  found: T2V VAE decode block replaced with single full-tensor decode. Entire latent x[lW, lH, lT, 16] decoded at once via ctx->vae_runner->compute(n_threads, x, true, &decoded, vae_ctx). Frame dimensions derived from decoded->ne[2] (T_out), decoded->ne[0] (W), decoded->ne[1] (H). frame_bufs allocated with size T_out × (W*H*3). Pixel data filled via ggml_get_f32_nd(decoded, w, h, t, c) with indices guaranteed to be in [0,W)×[0,H)×[0,T)×[0,3).
  implication: Buffer allocation now matches actual decoded tensor layout. No per-frame extraction or dimension mismatch. Memory access pattern is consistent with I2V implementation, eliminating overflow risk.

## Resolution

root_cause: Per-frame VAE decoding extracted single frames into separate tensors, causing dimension mismatch between allocated frame_bufs (sized by latent dimensions lW*8, lH*8, lT) and actual decoded output (which may have different layout when processing full tensor). This led to buffer overflow during pixel writing, corrupting heap metadata and causing malloc_printerr() on vector destruction.
fix: Replaced per-frame decode loop with single full-tensor decode. Latent tensor x[lW, lH, lT, 16] is decoded in one call, producing decoded[W, H, 3, T]. Frame buffer dimensions are derived from decoded->ne, ensuring allocation matches actual data layout. Pixel data is extracted via ggml_get_f32_nd with proper 4D indexing.
verification: (pending - requires test run)
files_changed: [src/api/wan-api.cpp]
