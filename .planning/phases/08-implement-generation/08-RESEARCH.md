# Phase 8: Implement Generation + AVI Output - Research

**Researched:** 2026-03-16
**Domain:** WAN video generation pipeline, denoising loop, VAE decode, AVI output
**Confidence:** HIGH

## Summary

Phase 8 completes the generation pipeline by implementing the denoising loop inside `wan_generate_video_t2v_ex` and `wan_generate_video_i2v_ex`, decoding latents via the VAE, and writing output frames to an AVI file via `avi_writer`. All wiring from Phase 7 is in place: T5/CLIP encoders are called, `WanRunner::compute` is invoked. What remains is the iterative sampling loop, VAE decode, and AVI write.

The reference implementation in `stable-diffusion.cpp` shows the exact pattern: initialize noise latent, call the Euler loop with a lambda that calls the diffusion model per step, then call `decode_first_stage` on the result. The WAN-specific VAE decode uses `decode_video=true` and expands temporal dimension as `((T-1)*4)+1`.

The `avi_writer.h` in `examples/cli/` declares `create_mjpg_avi_from_rgb_frames` but the implementation in `avi_writer.c` is a stub. The plan must include implementing it. `wan_load_image` must be implemented using `stb_image.h` (available at `thirdparty/stb_image.h`).

**Primary recommendation:** Implement the Euler sampler loop directly in `wan-api.cpp`, call `WanRunner::compute` per step, decode with VAE, convert float tensors to uint8 RGB, then call `create_mjpg_avi_from_rgb_frames`. Implement `avi_writer.c` using `stb_image_write.h` for JPEG frames and implement `wan_load_image` using `stb_image.h`.

---

## User Constraints

### Locked Decisions
- Follow `stable-diffusion.cpp` original implementation exactly — no divergence in algorithm, latent normalization constants, scheduler logic, or VAE decode strategy
- Single TU (`wan-api.cpp`) owns all runner headers and `_ex` generation functions
- `ggml_extend.hpp` (not `preprocessing.hpp`) for image tensor helpers
- Explicit CMake source list (no GLOB_RECURSE)
- `WAN_API` visibility on all public symbols
- Raw RGB AVI output (no JPEG encoding required — use uncompressed DIB codec)

### Claude's Discretion
- Exact ggml_context buffer sizes for denoising work buffers
- Whether to split denoising into a helper function or keep inline in wan-api.cpp
- stb_image include path

### Deferred (OUT OF SCOPE)
- Multiple sampler methods
- Preview callbacks during denoising
- Tiled VAE decode

---

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| API-03 | `wan_generate_video_t2v_ex` runs denoising loop and produces video frames | Euler loop pattern from reference, `WanRunner::compute` already wired |
| API-04 | `wan_generate_video_i2v_ex` runs denoising loop from input image and produces video frames | Same loop; I2V passes image latent as conditioning, `wan_load_image` needed |
| EX-02 | CLI end-to-end T2V and I2V flows produce valid AVI files | `create_mjpg_avi_from_rgb_frames` in `avi_writer.h`; stub needs real implementation |

---

## Standard Stack

| Library | Purpose | Location |
|---------|---------|----------|
| ggml | Tensor ops, graph execution | project symlink |
| stb_image.h | Load PNG/JPG for `wan_load_image` | thirdparty/ (check parent repo) |
| stb_image_write.h | JPEG encode for AVI frames (if MJPG) | thirdparty/ (check parent repo) |
| denoiser.hpp | Sigma schedules, Euler sampler reference | ../src/denoiser.hpp |
| rng.hpp / rng_mt19937.hpp | Seeded noise generation | src/ (copied from parent) |

---

## Architecture Patterns

### Recommended Changes
No new directories needed. All changes land in existing files:
```
src/api/
  wan-api.cpp        # denoising loop + VAE decode + AVI call added here
examples/cli/
  avi_writer.c       # implement create_mjpg_avi_from_rgb_frames (raw RGB frames)
```

### Pattern 1: Euler Denoising Loop (flow matching)
```cpp
// Linear sigmas from 1.0 to 0.0 for flow matching
std::vector<float> sigmas(steps + 1);
for (int i = 0; i <= steps; i++) {
    sigmas[i] = 1.0f - (float)i / (float)steps;
}

// Initialize noise latent: [W/8, H/8, (frames-1)/4+1, 16]
int64_t lW = params->width / 8;
int64_t lH = params->height / 8;
int64_t lT = (params->num_frames - 1) / 4 + 1;
ggml_tensor* x = ggml_new_tensor_4d(work_ctx, GGML_TYPE_F32, lW, lH, lT, 16);
// fill with randn scaled by sigmas[0]=1.0

// Euler loop
for (int i = 0; i < steps; i++) {
    float sigma = sigmas[i];
    // call WanRunner::compute with x, sigma as timestep, context, clip_fea
    ggml_tensor* denoised = nullptr;
    ctx->wan_runner->compute(n_threads, x, /*timestep=sigma*/, context, clip_fea,
                             nullptr, nullptr, nullptr, 1.f, &denoised, step_ctx);
    // d = (x - denoised) / sigma
    // x = x + d * (sigmas[i+1] - sigma)
}
```

### Pattern 2: Latent normalization (process_latent_in / process_latent_out)
```cpp
// WAN 16-channel mean/std from stable-diffusion.cpp:2424
static const float wan_latents_mean[16] = {
    -0.7571f, -0.7089f, -0.9113f,  0.1075f, -0.1745f,  0.9653f, -0.1517f,  1.5508f,
     0.4134f, -0.0715f,  0.5517f, -0.3632f, -0.1922f, -0.9497f,  0.2503f, -0.2921f};
static const float wan_latents_std[16] = {
    2.8184f, 1.4541f, 2.3275f, 2.6558f, 1.2196f, 1.7708f, 2.6052f, 2.0743f,
    3.2687f, 2.1526f, 2.8652f, 1.5579f, 1.6382f, 1.1253f, 2.8251f, 1.9160f};
// scale_factor = 1.0 for WAN (no additional scaling beyond mean/std normalization)
// process_latent_out: value = value * std[c] / scale_factor + mean[c]
```

### Pattern 3: VAE decode
```cpp
// After denoising, apply process_latent_out (inverse normalization)
// Then call WanVAERunner::compute with decode=true
ggml_tensor* decoded = nullptr;
ctx->vae_runner->compute(n_threads, x_denoised, /*decode=*/true, &decoded, vae_ctx);
// decoded shape: [W, H, T_expanded, 3] where T_expanded = ((lT-1)*4)+1
// clamp to [0,1]
```

### Pattern 4: Float tensor to uint8 RGB
```cpp
// GGML tensor layout: ne[0]=W, ne[1]=H, ne[2]=T, ne[3]=C(3)
// For each frame t, extract W*H*3 bytes
int T_out = decoded->ne[2];
std::vector<std::vector<uint8_t>> frame_data(T_out, std::vector<uint8_t>(W*H*3));
for (int t = 0; t < T_out; t++) {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            for (int c = 0; c < 3; c++) {
                float v = ggml_get_f32_nd(decoded, x, y, t, c);
                frame_data[t][(y*W+x)*3+c] = (uint8_t)(v * 255.0f + 0.5f);
            }
        }
    }
}
```

### Pattern 5: AVI raw RGB output (no JPEG needed)
```c
// Use raw RGB (DIB uncompressed) instead of MJPG to avoid JPEG encoder dependency
// Each frame chunk: "00dc" tag + size + raw BGR data (AVI uses BGR not RGB)
// Fix placeholder sizes by seeking back after writing all frames
```

### Pattern 6: wan_load_image using stb_image
```cpp
// In wan-api.cpp (or a new wan_image.cpp with only stb_image.h)
#define STB_IMAGE_IMPLEMENTATION  // only in one TU
#include "stb_image.h"

wan_error_t wan_load_image(const char* image_path, wan_image_t** out_image) {
    int w, h, c;
    uint8_t* data = stbi_load(image_path, &w, &h, &c, 3); // force RGB
    if (!data) return WAN_ERROR_IMAGE_LOAD_FAILED;
    wan_image_t* img = new wan_image_t();
    img->width = w; img->height = h; img->channels = 3;
    img->data = data; // stbi_load allocates with malloc, wan_free_image calls free()
    *out_image = img;
    return WAN_SUCCESS;
}
```

---

## Anti-Patterns to Avoid

- **Calling `preprocessing.hpp` from `wan-api.cpp`:** ODR violation — use `ggml_extend.hpp` only
- **Using GLOB_RECURSE in CMake:** Picks up untracked files — use explicit source list
- **Stack-allocating frame arrays:** 16 frames at 640×360×3 ≈ 11MB — use heap
- **Forgetting `stbi_image_free`:** Memory leak if stb_image used
- **Defining `STB_IMAGE_IMPLEMENTATION` in multiple TUs:** Duplicate symbol linker errors
- **Wrong temporal formula:** Output frames = `((lT-1)*4)+1`, not `lT*4`

---

## Common Pitfalls

### Pitfall 1: avi_writer.c is a stub
`create_mjpg_avi_from_rgb_frames` writes only a partial AVI header with no frame data. Must implement real frame writing with correct RIFF chunk sizes patched after all frames are written.

### Pitfall 2: WAN temporal dimension expansion
Latent T=5 → output 17 frames (`((5-1)*4)+1=17`). Allocate decode result with this formula.

### Pitfall 3: GGML tensor memory layout
GGML uses `ne[0]=W` (fastest), `ne[1]=H`, `ne[2]=T`, `ne[3]=C`. Use `ggml_get_f32_nd` or stride-based access, not flat array indexing.

### Pitfall 4: scale_factor for WAN
WAN uses `scale_factor=1.0` (unlike SD1.x which uses 0.18215). The `process_latent_out` formula is `value = value * std[c] + mean[c]` (no additional scale_factor division needed beyond the std).

### Pitfall 5: wan_load_image placement
Keep in `wan-api.cpp` or a dedicated `wan_image.cpp`. Do NOT put in `wan_loader.cpp` (header-free by Phase 7 design decision).

---

## Validation Architecture

### Test Strategy
- **Unit:** `wan_load_image` loads a known PNG/JPG and returns correct width/height/channels
- **Integration:** T2V pipeline produces non-empty AVI file with correct frame count
- **Integration:** I2V pipeline produces non-empty AVI file from input image
- **E2E:** CLI `wan-cli -m model.gguf -p "test" -o out.avi` exits 0 and produces valid AVI

### Verification Criteria
1. `wan_load_image` returns `WAN_SUCCESS` for valid image files
2. `wan_generate_video_t2v_ex` returns `WAN_SUCCESS` (not `WAN_ERROR_UNSUPPORTED_OPERATION`)
3. `wan_generate_video_i2v_ex` returns `WAN_SUCCESS`
4. Output AVI file size > 0 bytes and contains valid RIFF header
5. AVI frame count matches `((num_frames-1)/4*4)+1` temporal expansion
6. Library compiles without errors after changes

---

## RESEARCH COMPLETE
