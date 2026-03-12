# Roadmap: wan-cpp

**Project:** Standalone C++ Library for WAN Video Generation
**Created:** 2026-03-12
**Granularity:** Coarse (3-5 phases)

## Overview

This roadmap extracts a standalone C++ library for WAN video generation from the stable-diffusion.cpp monorepo. The library provides independent, lightweight, cross-platform inference capabilities for WAN2.1 and WAN2.2 series models.

## Progress Summary

| Phase | Status | Progress |
|-------|--------|----------|
| 1 - Foundation | Not started | 0% |
| 2 - Build System | Not started | 0% |
| 3 - Public API | Not started | 0% |
| 4 - Examples | Not started | 0% |

## Phases

- [ ] **Phase 1: Foundation** - Project structure and code extraction
- [ ] **Phase 2: Build System** - CMake configuration and multi-backend support
- [ ] **Phase 3: Public API** - C-style API and core inference functionality
- [ ] **Phase 4: Examples** - CLI tool, video output, and documentation

## Phase Details

### Phase 1: Foundation

**Goal**: Establish complete project structure and extract WAN core code with proper dependency management

**Depends on**: Nothing (first phase)

**Requirements**: STRUCT-01, STRUCT-02, STRUCT-03, CORE-01, CORE-02, CORE-03, CORE-04, CORE-05

**Success Criteria** (what must be TRUE):
1. Project directory structure exists (include, src, examples, thirdparty, .planning)
2. GGML submodule is properly initialized and trackable (not in detached HEAD)
3. All WAN core files (wan.hpp, common_block.hpp, rope.hpp, vae.hpp, flux.hpp) are extracted and include paths work
4. Supporting files (preprocessing.hpp, util.h/cpp, model.h/cpp WAN components) are extracted
5. All macro names use WAN-specific prefixes to avoid collisions with stable-diffusion.cpp
6. README.md provides basic project overview and usage instructions

**Plans**: TBD

---

### Phase 2: Build System

**Goal**: Create functional CMake build system supporting multiple hardware backends

**Depends on**: Phase 1 (needs extracted source files)

**Requirements**: BUILD-01, BUILD-02, BUILD-03, BUILD-04

**Success Criteria** (what must be TRUE):
1. CMakeLists.txt compiles successfully on Linux with CPU backend
2. CMakeLists.txt compiles successfully with CUDA backend (when CUDA available)
3. CMakeLists.txt compiles successfully with Metal backend (on macOS)
4. CMakeLists.txt compiles successfully with Vulkan backend (when Vulkan available)
5. Third-party dependencies (json.hpp, zip.h) are correctly configured and link
6. Build produces a static library file (libwan.a or wan.lib)

**Plans**: TBD

---

### Phase 3: Public API

**Goal**: Expose WAN functionality through C-style public API for language interoperability

**Depends on**: Phase 2 (needs compilable library)

**Requirements**: API-01, API-02, API-03, API-04, API-05

**Success Criteria** (what must be TRUE):
1. C header file (wan.h) exists with stable, documented function signatures
2. User can load a WAN model from GGUF file using API
3. User can generate video from text prompt (T2V mode) using API
4. User can generate video from image (I2V mode) using API
5. User can configure generation parameters (seed, steps, guidance scale) via API
6. Library can be linked from C, C++, and has opaque handle patterns ready for language bindings

**Plans**: TBD

---

### Phase 4: Examples

**Goal**: Provide production-ready examples demonstrating library usage

**Depends on**: Phase 3 (needs functional public API)

**Requirements**: EX-01, EX-02, EX-03, EX-04

**Success Criteria** (what must be TRUE):
1. CLI example program compiles and runs from command line
2. CLI accepts basic command-line arguments (model path, prompt, output path)
3. Generated videos can be saved in AVI format
4. Example documentation (README or docs/) shows typical usage patterns
5. CLI example demonstrates both T2V and I2V generation modes

**Plans**: TBD

---

## Dependencies Graph

```
Phase 1 (Foundation)
  └─> Phase 2 (Build System)
        └─> Phase 3 (Public API)
              └─> Phase 4 (Examples)
```

## Requirements Coverage

| Phase | Requirement Count | IDs |
|-------|------------------|-----|
| 1 - Foundation | 8 | STRUCT-01,02,03 + CORE-01,02,03,04,05 |
| 2 - Build System | 4 | BUILD-01,02,03,04 |
| 3 - Public API | 5 | API-01,02,03,04,05 |
| 4 - Examples | 4 | EX-01,02,03,04 |
| **Total** | **21** | **All v1 requirements mapped** |

## v2 Scope (Deferred)

The following features are intentionally deferred to v2:

- ENH-01: Progress callbacks for UI integration
- ENH-02: Preview generation during inference
- ENH-03: Batch generation support
- ENH-04: C++17-style API wrappers
- ENH-05: Streaming output for large videos
- ENH-06: Detailed logging system
- OPT-01: Memory pool mechanisms
- ENH-07: Spectrum caching functionality
- ENH-08: RAII resource management optimization

These features are tracked in REQUIREMENTS.md but not part of the v1 roadmap.

---
*Roadmap created: 2026-03-12*
