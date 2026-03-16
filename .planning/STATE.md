---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
current_phase: 7 - Wire Core Model to API
status: in-progress
last_updated: "2026-03-16T03:21:18Z"
progress:
  total_phases: 8
  completed_phases: 6
  total_plans: 6
  completed_plans: 6
---

# Project State: wan-cpp

**Last Updated:** 2026-03-16
**Current Phase:** 7 - Wire Core Model to API

## Project Reference

**Core Value:** Provide independent, lightweight, cross-platform WAN video generation inference capabilities
**Current Focus:** Phase 5 - Encoders (T5 and CLIP encoder integration)

## Current Position

| Field | Value |
|-------|-------|
| Phase | 7 - Wire Core Model to API |
| Plan | 01 |
| Status | Planned |
| Progress | 75% |

## Phase Progress

| Phase | Status | Plans | Completed |
|-------|--------|-------|------------|
| 1 - Foundation | Completed | 1/1 | 01-foundation |
| 2 - Build System | Completed | 1/1 | 02-build-system |
| 3 - Public API | Completed | 1/1 | 03-public-api |
| 4 - Examples | Completed | 1/1 | 04-examples |
| 5 - Encoders | Completed | 1/1 | 05-encoders |
| 6 - Fix Duplicate Symbols | Completed | 1/1 | 06-01 |
| 7 - Wire Core Model to API | Planned | 0/1 | - |
| 8 - Implement Generation + AVI Output | Planned | 0/1 | - |

## Performance Metrics

| Phase | Plan | Duration | Tasks | Files | Started | Completed |
|-------|------|----------|-------|---------|-----------|
| 1 - Foundation | 01-foundation | 16 min | 10 | 30+ | 2026-03-12T04:43:06Z | 2026-03-12T04:57:08Z |
| 2 - Build System | 02-build-system | 20 min | 6 | 5 | 2026-03-12T13:05:00Z | 2026-03-12T13:25:00Z |
| 3 - Public API | 03-public-api | 45 min | 5 | 12 | 2026-03-12T14:12:00Z | 2026-03-12T14:57:00Z |
| 4 - Examples | 04-examples | 5 min | 4 | 4 | 2026-03-15T07:32:16Z | 2026-03-15T07:37:00Z |
| 5 - Encoders | 05-encoders | - | - | - | - | - |
| 6 - Fix Duplicate Symbols | 06-01 | 15 min | 3 | 4 | 2026-03-16T03:06:00Z | 2026-03-16T03:21:18Z |

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

- Execute Phase 5 plan to integrate T5 and CLIP encoders
- Complete T2V/I2V generation with encoder integration

## Session Continuity

**Last Action:** Phase 6 - Fix Duplicate Symbols complete
**Next Action:** Execute Phase 7 - Wire Core Model to API
**Context:** Phase 6 eliminated all ODR violations: removed param/generation stubs from wan-api.cpp, added WAN_API to all wan_config.cpp exports, replaced GLOB_RECURSE with explicit source list, fixed avi_writer.h include guard. Library now links cleanly with 0 duplicate symbol errors.

---
*State updated: 2026-03-15*
