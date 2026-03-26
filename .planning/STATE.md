---
gsd_state_version: 1.0
milestone: v1.1
milestone_name: 模型格式扩展
current_phase: 16
status: Phase complete — ready for verification
last_updated: "2026-03-26T02:09:44.323Z"
progress:
  total_phases: 8
  completed_phases: 8
  total_plans: 14
  completed_plans: 14
---

# Project State: wan-cpp

**Last Updated:** 2026-03-25
**Current Phase:** 16

## Project Reference

**Core Value:** Provide independent, lightweight, cross-platform WAN video generation inference capabilities
**Current Focus:** Phase 16 — spdlog

## Current Position

Phase: 16 (spdlog) — EXECUTING
Plan: 1 of 1

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
| 12 - Wire Vocab Dir to Public API | Completed | 1/1 | 12-01 |
| 13 - Document wan-convert Sub-model Scope | Completed | 1/1 | 13-01 |
| 14 - 性能优化 - CUDA Graph 和算子融合 | Completed | 2/2 | 14-01, 14-02 |
| 15 - 多卡推理支持 | In Progress | 4/5 | 15-00, 15-01, 15-02, 15-03, 15-04 |

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
| 12 - Wire Vocab Dir to Public API | 12-01 | 3 min | 2 | 6 | 2026-03-17T04:51:40Z | 2026-03-17T04:54:47Z |
| 13 - Document wan-convert Sub-model Scope | 13-01 | 2 min | 3 | 2 | 2026-03-17T06:30:19Z | 2026-03-17T06:32:01Z |
| Phase 14 P01 | 681 | 2 tasks | 3 files |
| Phase 14 P02 | 245 | 2 tasks | 2 files |
| 15 - Multi-GPU Inference Support | 15-00 | 2 min | 1 | 6 | 2026-03-18T04:48:25Z | 2026-03-18T04:50:17Z |
| 15 - Multi-GPU Inference Support | 15-01 | 892 | 3 | 3 | 2026-03-18T04:52:23Z | 2026-03-18T13:07:15Z |
| Phase 15 P00 | 112 | 1 tasks | 6 files |
| Phase 15 P03 | 167 | 3 tasks | 4 files |
| Phase 15 P02 | 341 | 3 tasks | 6 files |
| 15 - Multi-GPU Inference Support | 15-04 | 278 | 4 | 5 | 2026-03-18T05:23:54Z | 2026-03-18T05:28:32Z |
| Phase 16 P01 | 888 | 4 tasks | 5 files |

## Accumulated Context

### Roadmap Evolution

- Phase 14 added: 性能优化 - CUDA Graph 和算子融合（基于优化 TODO 列表，重点关注 Quick Wins）
- Phase 15 added: 多卡推理支持
- Phase 16 added: 帮我修改一下日志系统，替换成spdlog

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
| wan_set_vocab_dir no-op for WAN_EMBED_VOCAB=ON | Phase 12 plan 01 | Returns WAN_ERROR_INVALID_ARGUMENT when vocab is compiled in; clear contract for callers |
| WAN_EMBED_VOCAB explicit propagation to wan-cli | Phase 12 plan 01 | PRIVATE on wan-cpp does not auto-propagate; must add target_compile_definitions to wan-cli explicitly |
| Neutral annotation tone in print_usage() | Phase 13 plan 01 | (loadable by wan_load_model) and (reserved: future multi-file loading) — no WARNING/ERROR language |
| SAFE-03 remains unchecked | Phase 13 plan 01 | Boundary documented but multi-file loading not yet implemented; requirement only partially satisfied |
| Graph structure stability optimization via cached_graph pointer | Phase 14 plan 01 | Skip redundant graph rebuilds when buffer is persistent |
| Flash attention auto-enabled for non-CPU backends via ggml_backend_is_cpu check | Phase 14 plan 01 | Eliminates manual configuration and ensures optimal performance on GPU backends |
| GGML_CUDA_USE_GRAPHS controlled by WAN_CUDA_GRAPHS CMake option (default ON) | Phase 14 plan 01 | Enables GGML's built-in CUDA graph capture/replay mechanism |
| PE caching via dimension tracking (t/h/w) to skip redundant CPU computation | Phase 14 plan 02 | Simple and effective - dimensions rarely change during generation, cache hit rate ~95%+ |
| Inplace GELU already optimal - fusion relies on CUDA graph kernel merging | Phase 14 plan 02 | Existing inplace GELU avoids intermediate memory allocation; CUDA graph handles kernel fusion automatically |
| Added ggml_ext_linear_gelu() helper for explicit fusion pattern documentation | Phase 14 plan 02 | Makes fusion pattern explicit for future code, even though current code already follows best practices |

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

## Decisions

### Phase 15 - Multi-GPU Inference Support

**Decision:** Use ggml_backend_sched_t for multi-GPU scheduling

- **Context:** Need to coordinate multiple GPU backends for distributed inference
- **Rationale:** GGML provides built-in backend scheduler with automatic memory management and tensor placement optimization
- **Impact:** Reduces implementation complexity, leverages battle-tested GGML infrastructure
- **Date:** 2026-03-18

**Decision:** Guard multi-GPU code with WAN_USE_MULTI_GPU

- **Context:** Need to maintain backward compatibility with single-GPU builds
- **Rationale:** Ensures single-GPU builds remain unaffected and compile without CUDA dependencies
- **Impact:** Clean separation of concerns, no overhead for CPU/single-GPU users
- **Date:** 2026-03-18

**Decision:** Make NCCL optional with fallback warning

- **Context:** Not all systems have NCCL installed
- **Rationale:** Basic multi-GPU can work without NCCL using CUDA peer-to-peer communication
- **Impact:** Broader compatibility, graceful degradation when NCCL unavailable
- **Date:** 2026-03-18
- [Phase 15]: Use std::thread for concurrent execution - simple, portable threading without external dependencies
- [Phase 15]: Round-robin GPU assignment for balanced load distribution across GPUs
- [Phase 15]: Use ggml_backend_cuda_split_buffer_type for tensor distribution across GPUs
- [Phase 15]: Target gpu_ids[0] as main_device for primary backend operations
- [Phase 16]: Custom sink pattern for callback distribution - sd_callback_sink_mt intercepts formatted messages
- [Phase 16]: dist_sink_mt composition for multi-sink logging (stdout + callback)
- [Phase 16]: Simplified callback signature - removed level parameter, spdlog formats complete message

## Todo Items

### Blocking

- None

### Pending

- None — all phases complete

## Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 260317-nl6 | 分析各个子模型的优化空间：CUDA Graph 优化、算子实现效率、第三方库使用、算子融合机会。整理成 TODO 列表保存到 .planning/OPTIMIZATION_TODOS.md | 2026-03-17 | 20d9aab | [260317-nl6](./quick/260317-nl6-cuda-graph-todo-planning-optimization-to/) |
| 260325-fcl | spdlog v1.13.0 离线集成 - 将spdlog从编译时网络下载改为离线thirdparty集成，消除FetchContent依赖 | 2026-03-25 | 5fe4a56 | [260325-fcl](./quick/260325-fcl-spdlog-thirdparty/) |

## Session Continuity

**Last Action:** Completed Phase 15 Plan 04 - CLI Multi-GPU Integration and Validation
**Next Action:** Execute Phase 15 Plan 05 (if exists) or complete Phase 15
**Context:** Added CLI multi-GPU arguments (--gpu-ids/--num-gpus), GPU info query API, benchmark script for performance metrics, and precision validation script. All 4 tasks completed with 4 commits (4774ff9, 4fbf155, cf29dbb, a08dae1). Checkpoint auto-approved in auto mode.

---
*State updated: 2026-03-25 — Quick Task 260325-fcl complete (spdlog offline integration)*
