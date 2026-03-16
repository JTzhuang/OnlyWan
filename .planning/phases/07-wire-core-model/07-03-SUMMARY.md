---
phase: 07-wire-core-model
plan: 03
subsystem: generation-pipelines
tags: [t2v, i2v, t5-encoder, clip-encoder, wan-runner, odr]
dependency_graph:
  requires: [07-01, 07-02]
  provides: [wan_generate_video_t2v_ex, wan_generate_video_i2v_ex, encoder-to-model-wiring]
  affects: [src/api/wan-api.cpp, src/api/wan_t2v.cpp, src/api/wan_i2v.cpp]
tech_stack:
  added: []
  patterns: [single-TU-runner-calls, ggml_extend-not-preprocessing, correct-sd_image_to_ggml_tensor-signature]
key_files:
  created: []
  modified:
    - src/api/wan-api.cpp
    - src/api/wan_t2v.cpp
    - src/api/wan_i2v.cpp
decisions:
  - wan_generate_video_t2v_ex and wan_generate_video_i2v_ex implemented in wan-api.cpp (not wan_t2v/i2v.cpp) to avoid ODR violations from non-inline runner definitions
  - ggml_extend.hpp included directly instead of preprocessing.hpp to avoid ODR from non-inline convolve/gaussian_kernel already emitted by util.cpp
  - sd_image_to_ggml_tensor called with correct signature (sd_image_t by value, ggml_tensor* output) after discovering plan had wrong signature
metrics:
  duration: "5 min"
  completed: "2026-03-16T06:07:53Z"
  tasks_completed: 2
  files_modified: 3
---

# Phase 7 Plan 3: Wire Core Model — Encoder-to-Model Wiring Summary

**One-liner:** T5 tokenize+compute and CLIP sd_image_to_ggml_tensor+compute wired to WanRunner::compute in wan-api.cpp, closing ENCODER-01, ENCODER-02, CORE-01, CORE-02.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Rewrite wan_t2v.cpp — T5 encode and pass context to WanRunner | e037abb | src/api/wan_t2v.cpp, src/api/wan-api.cpp |
| 2 | Rewrite wan_i2v.cpp — CLIP encode via ctx->clip_runner and pass real clip_fea to WanRunner | 006cf27 | src/api/wan_i2v.cpp, src/api/wan-api.cpp |

## Decisions Made

1. **_ex implementations in wan-api.cpp** — Both `wan_generate_video_t2v_ex` and `wan_generate_video_i2v_ex` were moved to `wan-api.cpp` (the single TU owning all runner headers). Placing them in `wan_t2v.cpp`/`wan_i2v.cpp` required including `wan.hpp`/`t5.hpp`/`clip.hpp`, which caused duplicate `DiT::` symbol linker errors — the same ODR pattern fixed in Plans 01 and 02.

2. **ggml_extend.hpp not preprocessing.hpp** — `preprocessing.hpp` defines `convolve`, `gaussian_kernel`, etc. as non-inline functions and is already included by `util.cpp`. Including it in `wan-api.cpp` would cause duplicate symbol errors. `ggml_extend.hpp` provides `sd_image_to_ggml_tensor` as `__STATIC_INLINE__`, safe to include in multiple TUs.

3. **Correct sd_image_to_ggml_tensor signature** — The plan's interface snippet showed a non-existent `(ggml_context*, sd_image_t*)` returning `ggml_tensor*`. The actual signature is `void sd_image_to_ggml_tensor(sd_image_t image, ggml_tensor* tensor, bool scale = true)` — requires pre-allocating the tensor with `ggml_new_tensor_4d` first.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] ODR violation: including wan.hpp/t5.hpp in wan_t2v.cpp caused duplicate DiT:: symbols**
- **Found during:** Task 1 build verification
- **Issue:** Plan specified `#include "wan.hpp"` and `#include "t5.hpp"` in wan_t2v.cpp. These headers contain non-inline `DiT::` definitions already emitted by wan-api.cpp, causing linker duplicate symbol errors.
- **Fix:** Moved `wan_generate_video_t2v_ex` implementation to wan-api.cpp. wan_t2v.cpp reduced to stub with only `wan-internal.hpp`.
- **Files modified:** src/api/wan_t2v.cpp, src/api/wan-api.cpp
- **Commit:** e037abb

**2. [Rule 1 - Bug] ODR violation: preprocessing.hpp non-inline functions already emitted by util.cpp**
- **Found during:** Task 2 build verification
- **Issue:** Plan specified `#include "preprocessing.hpp"` in wan_i2v.cpp. preprocessing.hpp defines `convolve`, `gaussian_kernel` etc. as non-inline functions. util.cpp already includes preprocessing.hpp, causing duplicate symbol errors if wan-api.cpp also includes it.
- **Fix:** Include `ggml_extend.hpp` directly instead — `sd_image_to_ggml_tensor` is `__STATIC_INLINE__` there, safe for multiple TUs.
- **Files modified:** src/api/wan-api.cpp
- **Commit:** 006cf27

**3. [Rule 1 - Bug] Wrong sd_image_to_ggml_tensor signature in plan interface**
- **Found during:** Task 2 first build attempt
- **Issue:** Plan showed `ggml_tensor* pixel_values = sd_image_to_ggml_tensor(work_ctx, &sd_img)` — this signature does not exist. Actual signature: `void sd_image_to_ggml_tensor(sd_image_t image, ggml_tensor* tensor, bool scale = true)`.
- **Fix:** Allocate tensor with `ggml_new_tensor_4d(work_ctx, GGML_TYPE_F32, w, h, c, 1)` first, then call `sd_image_to_ggml_tensor(sd_img, pixel_values)`.
- **Files modified:** src/api/wan-api.cpp
- **Commit:** 006cf27

## Build Verification

```
cmake --build .../wan/build 2>&1 | grep -E "error:|undefined reference"
# (no output — zero errors)
```

## Self-Check: PASSED

| Item | Status |
|------|--------|
| src/api/wan-api.cpp | FOUND |
| src/api/wan_t2v.cpp | FOUND |
| src/api/wan_i2v.cpp | FOUND |
| commit e037abb | FOUND |
| commit 006cf27 | FOUND |
| t5_embedder->tokenize in wan-api.cpp | FOUND |
| t5_embedder->model.compute in wan-api.cpp | FOUND |
| wan_runner->compute (T2V) in wan-api.cpp | FOUND |
| sd_image_to_ggml_tensor in wan-api.cpp | FOUND |
| clip_runner->compute in wan-api.cpp | FOUND |
| wan_runner->compute (I2V) in wan-api.cpp | FOUND |
| no global t5_tokenizer in wan_t2v.cpp | CONFIRMED |
| no global clip_tokenizer in wan_i2v.cpp | CONFIRMED |
| zero build errors | CONFIRMED |
