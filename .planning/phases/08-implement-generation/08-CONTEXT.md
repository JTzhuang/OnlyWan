# Phase 8: Implement Generation + AVI Output - Context

**Gathered:** 2026-03-16
**Status:** Ready for planning

<domain>
## Phase Boundary

Implement functional T2V and I2V video generation pipeline with AVI file output. This means:
replacing the `WAN_ERROR_UNSUPPORTED_OPERATION` stubs in `wan_generate_video_t2v_ex` and
`wan_generate_video_i2v_ex` with a real denoising loop + VAE decode + AVI write, and
implementing `wan_load_image`. Encoder wiring (T5/CLIP) is already done in Phase 7.

</domain>

<decisions>
## Implementation Decisions

### Overall approach
- Follow stable-diffusion.cpp original implementation exactly — no divergence in algorithm,
  latent normalization constants, scheduler logic, or VAE decode strategy.
- Primary reference: `../src/stable-diffusion.cpp` (the monorepo source).

### Denoising loop / scheduler
- Use Euler sampler (flow matching), matching `stable-diffusion.cpp` `sample_euler` path.
- Timestep schedule: discrete flow schedule, same sigmas/timesteps construction as reference.
- CFG (classifier-free guidance): apply unconditional pass with empty T5 context, same as reference.
- `set_flow_shift` / `DiscreteFlowDenoiser` pattern from reference for shift factor.

### Latent normalization
- `process_latent_in` before denoising: normalize with per-channel mean/std vectors.
  - WAN2.1 (16-channel): use the 16-element mean/std arrays from reference (lines 2424–2427).
  - WAN2.2 TI2V (48-channel): use the 48-element arrays from reference (lines 2429–2441).
- `process_latent_out` before VAE decode: inverse normalization, same arrays.
- Latent shape: `[W/8, H/8, T_latent, 16]` where `T_latent = ((num_frames - 1) / 4) + 1`.
- VAE scale factor: 8 for WAN2.1/I2V, 16 for WAN2.2 TI2V.

### VAE decode
- Call `WanVAERunner::compute(n_threads, latent, /*decode=*/true, &result, work_ctx)`.
- Video decode: output frame count `T = ((T_latent - 1) * 4) + 1` (inverse of latent compression).
- Clamp output to [0, 1] after decode (`ggml_ext_tensor_clamp_inplace`), same as reference.

### Image loading (wan_load_image)
- Use `stb_image` (already present in the ggml/stable-diffusion.cpp ecosystem).
- Load as RGB (3 channels), return `wan_image_t` with width/height/channels/data.
- Resize to match generation resolution if needed — follow reference `load_image_from_file` pattern.

### I2V initial latent
- VAE-encode the input image to get image latent, then blend with random noise at the
  appropriate noise level — follow reference `encode_first_stage` + noise init pattern.
- `c_concat` tensor for I2V: encoded image latent concatenated along channel dim, as in reference.

### AVI output
- Complete `avi_writer.c`: write actual raw RGB frame data into the `movi` LIST chunk.
- Each frame: `00dc` chunk tag + frame size + raw RGB bytes (width * height * 3).
- Fix placeholder sizes (RIFF file_size, strl_size, movi_size) by seeking back and patching
  after all frames are written — standard AVI finalization pattern.
- No JPEG encoding required: use raw RGB (`DIB ` / uncompressed) codec instead of MJPG,
  which avoids the JPEG encoder dependency entirely.
- Quality parameter remains unused (raw RGB has no quality setting).

### Claude's Discretion
- Exact ggml_context buffer sizes for denoising work buffers.
- Whether to split denoising into a helper function or keep inline in wan-api.cpp.
- stb_image include path (already available via ggml_extend.hpp or direct include).

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Reference implementation (primary)
- `../src/stable-diffusion.cpp` — Full WAN denoising loop, latent normalization constants,
  VAE decode with video support, flow shift, process_latent_in/out, generate_init_latent,
  decode_first_stage. Lines 2340–2805 are most relevant.

### Current stubs to replace
- `src/api/wan-api.cpp` — `wan_generate_video_t2v_ex`, `wan_generate_video_i2v_ex`,
  `wan_load_image` stubs. Phase 8 replaces the stub bodies.

### AVI writer
- `examples/cli/avi_writer.c` — Placeholder AVI writer. Phase 8 completes frame writing.
- `examples/cli/avi_writer.h` — `create_mjpg_avi_from_rgb_frames` signature (rename codec
  to raw RGB in implementation, keep signature compatible).

### Core model interfaces
- `src/wan.hpp` — `WanRunner::compute`, `WanVAERunner::compute` signatures.
- `src/api/wan-internal.hpp` — `wan_context` struct with runner/embedder members.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `WanVAERunner` (ctx->vae_runner): already loaded with weights in Phase 7 — call `compute(n_threads, latent, true, &result, work_ctx)` for decode.
- `T5Embedder` (ctx->t5_embedder): tokenize + compute already wired in `wan_generate_video_t2v_ex` — reuse for unconditional (empty prompt) CFG pass.
- `CLIPVisionModelProjectionRunner` (ctx->clip_runner): already wired in `wan_generate_video_i2v_ex`.
- `ggml_ext_tensor_clamp_inplace`: available via `ggml_extend.hpp`, use after VAE decode.
- `rng_mt19937.hpp` / `rng_philox.hpp`: available for latent noise initialization.

### Established Patterns
- Single TU rule: all runner calls stay in `wan-api.cpp` (Phase 7 decision — ODR constraint).
- `ggml_context` lifecycle: allocate work_ctx → compute → free, same pattern already in Phase 7 stubs.
- `wan_params_t` carries seed, steps, cfg, width, height, num_frames, fps — all available for the loop.

### Integration Points
- `wan_generate_video_t2v_ex` / `i2v_ex` in `wan-api.cpp`: replace the stub body after `WanRunner::compute` call.
- `wan_load_image` in `wan-api.cpp`: replace `WAN_ERROR_UNSUPPORTED_OPERATION` stub.
- `create_mjpg_avi_from_rgb_frames` in `examples/cli/avi_writer.c`: complete frame writing logic.
- CLI `main.cpp` already calls `wan_load_image` and `wan_generate_video_*_ex` — no CLI changes needed.

</code_context>

<specifics>
## Specific Ideas

- "保留stable-diffusion.cpp原始的方案就行" — follow the reference implementation exactly,
  no custom algorithms or divergence from the monorepo source.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 08-implement-generation*
*Context gathered: 2026-03-16*
