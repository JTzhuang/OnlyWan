# Project State: wan-cpp

**Last Updated:** 2026-03-12
**Current Phase:** 3 - Public API

## Project Reference

**Core Value:** Provide independent, lightweight, cross-platform WAN video generation inference capabilities

**Current Focus:** Phase 3 - Public API (C API with T2V, I2V, and configuration)

## Current Position

| Field | Value |
|-------|-------|
| Phase | 3 - Public API |
| Plan | 01 |
| Status | Completed |
| Progress | 100% |

## Phase Progress

| Phase | Status | Plans | Completed |
|-------|--------|-------|------------|
| 1 - Foundation | Completed | 1/1 | 01-foundation |
| 2 - Build System | Completed | 1/1 | 02-build-system |
| 3 - Public API | Completed | 1/1 | 03-public-api |
| 4 - Examples | Not started | yet to be planned | - |

## Performance Metrics

| Phase | Plan | Duration | Tasks | Files | Started | Completed |
|-------|------|----------|-------|-------|---------|-----------|
| 1 - Foundation | 01-foundation | 16 min | 10 | 30+ | 2026-03-12T04:43:06Z | 2026-03-12T04:57:08Z |
| 2 - Build System | 02-build-system | 20 min | 6 | 5 | 2026-03-12T13:05:00Z | 2026-03-12T13:25:00Z |
| 3 - Public API | 03-public-api | 45 min | 5 | 12 | 2026-03-12T14:12:00Z | 2026-03-12T14:57:00Z |

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

### Technical Notes

- **Source location:** /home/jtzhuang/projects/stable-diffusion.cpp/
- **Core files extracted:** wan.hpp, common_block.hpp, rope.hpp, vae.hpp, flux.hpp, preprocessing.hpp, util.h/cpp, model.h/cpp, ggml_extend.hpp, common_dit.hpp, gguf_reader.hpp
- **GGML integration:** Via symbolic link to ../ggml (cannot use git submodule to share directory)
- **CMake version:** 3.20+ required
- **C++ standard:** C++17
- **Type definitions:** Consolidated in wan-types.h
- **Build artifacts:** libwan-cpp.a (1.5MB static library)
- **Supported backends:** CPU (default), CUDA, Metal, Vulkan, OpenCL, SYCL, HIPBLAS, MUSA

### Known Risks

- Hardcoded relative include paths in extracted headers (fixed with wan-types.h)
- Symbolic link for GGML may need adjustment for production use
- Transitive header dependencies not obvious
- Preprocessor macro name collisions (partially addressed with WAN_ prefixes)

### Mitigations Applied

- Created unified include directory structure
- Created wan-types.h to replace stable-diffusion.h
- Included all necessary headers and source files
- Copied all supporting files (rng, name_conversion, ordered_map)
- Documented GGML symbolic link in .gitmodules
- Created functional CMake build system with multi-backend support
- Fixed compilation issues (missing includes and declarations)

## Todo Items

### Blocking

- None

### Pending

- Begin Phase 4 planning (Examples)

## Session Continuity

**Last Action:** Phase 3 - Public API completed successfully
**Next Action:** Begin Phase 4 planning or execution
**Context:** Public API framework implemented with model loading, T2V, I2V, and configuration. Full Wan model integration pending Phase 4 completion. Ready for example program implementation.

---
*State updated: 2026-03-12*
