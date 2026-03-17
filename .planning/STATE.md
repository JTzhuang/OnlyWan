---
gsd_state_version: 1.0
milestone: v1.1
milestone_name: 模型格式扩展
current_phase: 11
status: unknown
last_updated: "2026-03-17T03:42:33.159Z"
progress:
  total_phases: 3
  completed_phases: 3
  total_plans: 4
  completed_plans: 4
---

# Project State: wan-cpp

**Last Updated:** 2026-03-17
**Current Phase:** 11

## Project Reference

**Core Value:** Provide independent, lightweight, cross-platform WAN video generation inference capabilities
**Current Focus:** Phase 10 - Safetensors Runtime Loading

## Current Position

| Field | Value |
|-------|-------|
| Phase | 10 - Safetensors Runtime Loading |
| Plan | 01 (complete) |
| Status | Complete |
| Progress | 100% |

## Phase Progress

| Phase | Status | Plans | Completed |
|-------|--------|-------|------------|
| 1 - Foundation | Completed | 1/1 | 01-foundation |
| 2 - Build System | Completed | 1/1 | 02-build-system |
| 3 - Public API | Completed | 1/1 | 03-public-api |
| 4 - Examples | Completed | 1/1 | 04-examples |
| 5 - Encoders | Completed | 1/1 | 05-encoders |
| 6 - Fix Duplicate Symbols | Completed | 1/1 | 06-01 |
| 7 - Wire Core Model to API | Completed | 3/3 | 07-01, 07-02, 07-03 |
| 8 - Implement Generation + AVI Output | Completed | 2/2 | 08-01, 08-02 |
| 9 - API Fixes + Vocab + Mmap | Completed | 2/2 | 09-01, 09-02 |
| 10 - Safetensors Runtime Loading | Completed | 1/1 | 10-01 |
| 11 - Safetensors Conversion Tool | Completed | 1/1 | 11-01 |

## Performance Metrics

| Phase | Plan | Duration | Tasks | Files | Started | Completed |
|-------|------|----------|-------|---------|-----------|
| 1 - Foundation | 01-foundation | 16 min | 10 | 30+ | 2026-03-12T04:43:06Z | 2026-03-12T04:57:08Z |
| 2 - Build System | 02-build-system | 20 min | 6 | 5 | 2026-03-12T13:05:00Z | 2026-03-12T13:25:00Z |
| 3 - Public API | 03-public-api | 45 min | 5 | 12 | 2026-03-12T14:12:00Z | 2026-03-12T14:57:00Z |
| 4 - Examples | 04-examples | 5 min | 4 | 4 | 2026-03-15T07:32:16Z | 2026-03-15T07:37:00Z |
| 5 - Encoders | 05-encoders | - | - | - | - | - |
| 6 - Fix Duplicate Symbols | 06-01 | 15 min | 3 | 4 | 2026-03-16T03:06:00Z | 2026-03-16T03:21:18Z |
| 7 - Wire Core Model | 07-02 | 5 min | 1 | 3 | 2026-03-16T05:54:39Z | 2026-03-16T05:59:53Z |
| 7 - Wire Core Model | 07-03 | 5 min | 2 | 3 | 2026-03-16T06:02:56Z | 2026-03-16T06:07:53Z |
| 8 - Implement Generation | 08-02 | 15 min | 2 | 3 | 2026-03-16T11:38:00Z | 2026-03-16T11:53:00Z |
| 9 - API Fixes + Vocab + Mmap | 09-01 | 3 min | 3 | 3 | 2026-03-16T16:07:00Z | 2026-03-16T16:10:00Z |
| 9 - API Fixes + Vocab + Mmap | 09-02 | 4 min | 3 | 3 | 2026-03-16T16:06:08Z | 2026-03-17T00:10:00Z |
| 10 - Safetensors Runtime Loading | 10-01 | 10 min | 2 | 2 | 2026-03-17T01:52:00Z | 2026-03-17T02:02:50Z |
| 11 - Safetensors Conversion Tool | 11-01 | 6 min | 2 | 5 | 2026-03-17T02:57:54Z | 2026-03-17T03:03:56Z |

## Accumulated Context

### Key Decisions

| Decision | Context | Rationale |
|----------|---------|-----------|
| 4-phase structure | Roadmap creation | Coarse granularity; natural requirement grouping |
| C-style public API | Phase 3 planning | ABI stability and language interoperability |
| GGML as symbolic link | Phase 1 execution | Git submodule cannot share directory with parent repo |
| wan-types.h for standalone library | Phase 1 execution | Replaces stable-diffusion.h without full dependency |
| CMake 3.20+ requirement | Phase 2 planning | Matches GGML and modern CMake features |
| Helper functions separation | Phase 3 execution | Resolves linking issues between API files |
| Non-opaque wan_params_t | Phase 3 execution | Simplifies C API parameter access |
| CLI with full argument parsing | Phase 4 execution | Complete CLI functionality for users |
| Explicit CMake source list | Phase 6 execution | GLOB_RECURSE picked up untracked src/wan_i2v.cpp causing duplicate symbols |
| WAN_API on all wan_params_* | Phase 6 execution | ABI visibility required for correct Windows shared build exports |
| Forward declarations in wan-internal.hpp | Phase 7 plan 01 | wan.hpp/t5.hpp/clip.hpp contain non-inline defs; forward-declare runner types to prevent ODR violations across TUs |
| WanModel::load in wan-api.cpp | Phase 7 plan 02 | Runner construction must live in single TU that owns full header includes; wan_loader.cpp stays header-free to avoid ODR |
| _ex generation in wan-api.cpp | Phase 7 plan 03 | wan_generate_video_t2v_ex and i2v_ex moved to wan-api.cpp; calling runner methods requires complete types, only available in single-TU owning all headers |
| ggml_extend.hpp not preprocessing.hpp | Phase 7 plan 03 | preprocessing.hpp has non-inline convolve/gaussian_kernel already emitted by util.cpp; ggml_extend.hpp provides sd_image_to_ggml_tensor as __STATIC_INLINE__ safe for multiple TUs |
| STB_IMAGE_IMPLEMENTATION in wan-api.cpp only | Phase 8 plan 01 | Single TU rule — defining in wan-api.cpp avoids ODR violations; stbi_load force-RGB (desired_channels=3) simplifies downstream handling |
| AVI codec DIB /BI_RGB not MJPG | Phase 8 plan 01 | Raw uncompressed RGB eliminates JPEG encoder dependency; 00dc chunk tag is standard AVI convention even for uncompressed streams |
| avi_writer.c in CLI CMake sources | Phase 8 plan 02 | Function defined in .c file must be compiled into executable, not static library |
| avi_writer.h uses C headers not C++ | Phase 8 plan 02 | File compiled as C; cstdint/cstdio not valid in C compilation units |
| Shared load_vocab_file() helper | Phase 9 plan 02 | Centralizes mmap_read call — one call site instead of six per-function calls |
| WAN_EMBED_VOCAB defaults OFF | Phase 9 plan 02 | Default build excludes 127MB of .hpp arrays; embedded path preserved for backward compat |
| Windows FILE* fallback in vocab.cpp | Phase 9 plan 02 | Avoids Win32 MapViewOfFile complexity; FILE* sufficient for vocab loading |
| is_safetensors_file declared in model.h | Phase 10 plan 01 | Defined in model.cpp but missing header declaration; required for wan-api.cpp to call it |
| Safetensors branch no prefix in init_from_file | Phase 10 plan 01 | HF WAN checkpoints already have model.diffusion_model.* names; doubling prefix breaks all tensor lookups |
| get_sd_version for safetensors type inference | Phase 10 plan 01 | Safetensors has no metadata fields; model_type/version inferred from tensor names via get_sd_version |
| 4-arg save_to_gguf_file overload | Phase 11 plan 01 | Backward-compatible addition; original 3-arg overload unchanged; metadata injected via gguf_set_val_str loop |
| SUBMODEL_META map in wan-convert | Phase 11 plan 01 | Maps --type string to arch/version strings matching is_wan_gguf() key expectations |

### Technical Notes

- **Source location:** /home/jtzhuang/projects/stable-diffusion.cpp/
- **Core files extracted:** wan.hpp, common_block.hpp, rope.hpp, vae.hpp, flux.hpp, preprocessing.hpp, util.h/cpp, model.h/cpp, ggml_extend.hpp, common_dit.hpp, gguf_reader.hpp
- **GGML integration:** Via symbolic link to ../ggml (cannot use git submodule to share directory)
- **CMake version:** 3.20+ required
- **C++ standard:** C++17
- **Type definitions:** Consolidated in wan-types.h
- **Build artifacts:** libwan-cpp.a (1.5MB static library)
- **Supported backends:** CPU (default), CUDA, Metal, Vulkan, OpenCL, SYCL, HIPBLAS, MUSA
- **CLI implemented:** Full argument parsing for model, prompt, input, output, backend, threads, width, height, frames, fps, steps, seed, cfg, negative-prompt
- **Encoder files to integrate:** t5.hpp, clip.hpp, vocab/ (umt5.hpp, clip_t5.hpp), darts.h, tokenize_util.h, json.hpp

### Known Risks

- Encoder files are large (vocabulary files ~29 MB each)
- Complex dependency relationships between encoders and core components
- T5 and CLIP integration requires significant code (~2000+ lines)
- Build system may need updates for encoder compilation

### Mitigations Applied

- Created unified include directory structure
- Created wan-types.h to replace stable-diffusion.h
- Included all necessary headers and source files
- Copied all supporting files (rng, name_conversion, ordered_map)
- Documented GGML symbolic link in .gitmodules
- Created functional CMake build system with multi-backend support
- Fixed compilation issues (missing includes and declarations)
- Created complete CLI with comprehensive argument parsing
- Created Phase 5 plan for encoder integration

## Todo Items

### Blocking

- None

### Pending

- None — all phases complete

## Session Continuity

**Last Action:** Phase 11 Plan 01 - Safetensors conversion tool complete
**Next Action:** v1.1 milestone complete — all phases done
**Context:** Plan 11-01 added 4-arg save_to_gguf_file overload with gguf_set_val_str metadata injection, created wan-convert CLI with --input/--output/--type/--quant flags supporting 6 sub-model types, wired CMake. wan-convert builds and --help exits 0.

---
*State updated: 2026-03-17 — Phase 11 complete*
