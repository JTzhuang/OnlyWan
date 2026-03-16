# Roadmap: wan-cpp

**Project:** Standalone C++ Library for WAN Video Generation
**Created:** 2026-03-12
**Granularity:** Coarse (3-5 phases)

## Overview

This roadmap extracts a standalone C++ library for WAN video generation from stable-diffusion.cpp monorepo. The library provides independent, lightweight, cross-platform inference inference capabilities for WAN2.1 and WAN2.2 series models.

## Progress Summary

| Phase | Status | Progress |
|-------|--------|----------|
| 1 - Foundation | Completed | 100% | 2026-03-12 |
| 2 - Build System | Completed | 100% | 2026-03-12 |
| 3 - Public API | Completed | 100% | 2026-03-15 |
| 4 - Examples | Completed | 100% | 2026-03-15 |
| 5 - Encoders | Completed | 100% | 2026-03-16 |
| 6 - Fix Duplicate Symbols | Planned | 0% | - |
| 7 - Wire Core Model to API | Planned | 0% | - |
| 8 - Implement Generation + AVI Output | Planned | 0% | - |

## Phases

- [x] **Phase 1: Foundation** - Project structure and code extraction
- [x] **Phase 2: Build System** - CMake configuration and multi-backend support
- [x] **Phase 3: Public API** - C-style API and core inference functionality
- [x] **Phase 4: Examples** - CLI tool, video writer, and documentation
- [x] **Phase 5: Encoders** - T5 and CLIP text/image encoder integration
- [ ] **Phase 6: Fix Duplicate Symbols** - Resolve linker failures from duplicate symbol definitions
- [ ] **Phase 7: Wire Core Model to API** - Connect wan.hpp model to API layer and encoder outputs
- [ ] **Phase 8: Implement Generation + AVI Output** - Implement T2V/I2V generation pipeline and wire AVI writer

## Phase Details

### Phase 1: Foundation

**Goal**: Establish complete project structure and extract WAN core code with proper dependency management

**Depends on**: Nothing (first phase)

**Requirements**: STRUCT-01, STRUCT-02, STRUCT-03, CORE-01, CORE-02, CORE-03, CORE-04, CORE-05

**Success Criteria** (what must be TRUE):
1. Project directory structure exists (include, src, examples, thirdparty, .planning)
2. GGML submodule is properly initialized and trackable (not in detached HEAD)
3. All WAN core files (wan.hpp, common_block.hpp, rope.hpp, vae.hpp, flux.hpp) are extracted and include paths work
4. Supporting files (preprocessing.hpp, util.h/cpp WAN components) are extracted
5. All macro names use WAN-specific prefixes to avoid collisions with stable-diffusion.cpp
6. README.md provides basic project overview and usage instructions

**Plans**: 01-foundation (Completed)
- [x] 01-foundation: Foundation plan execution
- See [01-foundation-SUMMARY.md](./phases/01-foundation/01-foundation-SUMMARY.md) for details

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
5. Third-party dependencies (json.hpp, zip.h) are correctly configured and linked
6. Build produces a static library file (libwan.a or wan.lib)

**Plans**: 02-build-system (Completed)
- [x] 02-build-system: Build system configuration
- See [02-build-system-SUMMARY.md](./phases/02-build-system/02-build-system-SUMMARY.md) for details

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

**Plans**: 03-public-api (Completed)
- [x] 03-public-api: Public API implementation
- See [03-public-api-SUMMARY.md](./phases/03-public-api/03-public-api-SUMMARY.md) for details

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

**Plans**: 04-examples (Completed)
- [x] 04-examples: CLI example with AVI output
- See [04-examples-SUMMARY.md](./phases/04-examples/04-examples-SUMMARY.md) for details

---

### Phase 5: Encoders

**Goal**: Integrate T5 and CLIP text/image encoders for complete video generation

**Depends on**: Phase 4 (needs functional CLI)

**Requirements**: ENCODER-01, ENCODER-02

**Success Criteria** (what must be TRUE):
1. T5 text encoder is integrated and compiles
2. CLIP image encoder is integrated and compiles
3. Vocabulary files (umt5.hpp, clip_t5.hpp) are included
4. Supporting infrastructure (darts.h, json.hpp) is integrated
5. CMakeLists.txt includes encoder source files
6. T2V generation uses T5 encoder
7. I2V generation uses CLIP encoder

**Plans**: 05-encoders (Completed)
- [x] 05-encoders: Encoder integration
- See [05-encoders-SUMMARY.md](./phases/05-encoders/05-encoders-SUMMARY.md) for details

---

### Phase 6: Fix Duplicate Symbols

**Goal**: Resolve linker failures caused by duplicate symbol definitions across source files

**Depends on**: Phase 5 (needs encoder integration complete)

**Requirements**: BUILD-01, API-05

**Gap Closure**: Closes gaps from v1.0 audit

**Success Criteria** (what must be TRUE):
1. `wan_params_*` functions defined in exactly one translation unit (remove from `wan-api.cpp` or `wan_config.cpp`)
2. `wan_generate_video_i2v*` defined in exactly one translation unit (remove `src/wan_i2v.cpp` or consolidate)
3. T2V/I2V generation stubs removed from `wan-api.cpp` (delegated to `wan_t2v.cpp`/`wan_i2v.cpp`)
4. CMakeLists.txt glob pattern does not pick up duplicate source files
5. Library links without duplicate symbol errors
6. `avi_writer.h` include guard fixed (`__.AVI_WRITER_H__` → `AVI_WRITER_H__`)

**Plans**: 06-fix-duplicate-symbols (Planned)
- [ ] 06-fix-duplicate-symbols: Fix linker failures

---

### Phase 7: Wire Core Model to API

**Goal**: Connect wan.hpp WAN model core to the API layer and wire encoder outputs to model inference

**Depends on**: Phase 6 (needs clean linking library)

**Requirements**: CORE-01, CORE-02, CORE-04, API-02, ENCODER-01, ENCODER-02

**Gap Closure**: Closes gaps from v1.0 audit

**Success Criteria** (what must be TRUE):
1. `wan_loader.cpp` instantiates WAN model from `wan.hpp` during model loading
2. Model weights loaded from GGUF file (not just metadata)
3. `preprocessing.hpp` used in I2V pipeline for image preprocessing
4. `model.h/cpp` functions called from `wan_loader.cpp` for model management
5. T5 token output from `wan_t2v.cpp` passed to WAN model inference
6. CLIP token output from `wan_i2v.cpp` passed to WAN model inference

**Plans**: 07-wire-core-model (Planned)
- [ ] 07-wire-core-model: Wire WAN model to API layer

---

### Phase 8: Implement Generation + AVI Output

**Goal**: Implement functional T2V and I2V video generation pipeline with AVI file output

**Depends on**: Phase 7 (needs wired model and encoders)

**Requirements**: API-03, API-04, EX-02

**Gap Closure**: Closes gaps from v1.0 audit

**Success Criteria** (what must be TRUE):
1. `wan_generate_video_t2v_ex` runs denoising loop and produces video frames
2. `wan_generate_video_i2v_ex` runs denoising loop from input image and produces video frames
3. `wan_load_image` implemented (loads image from file, not a stub)
4. VAE decoder called to convert latents to RGB frames
5. `avi_writer` called from generation pipeline to write output AVI file
6. CLI end-to-end T2V flow produces a valid AVI file
7. CLI end-to-end I2V flow produces a valid AVI file

**Plans**: 08-implement-generation (Planned)
- [ ] 08-implement-generation: Implement generation pipeline and AVI output

---

## Dependencies Graph

```
Phase 1 (Foundation)
 └─> Phase 2 (Build System)
        └─> Phase 3 (Public API)
              └─> Phase 4 (Examples)
                    └─> Phase 5 (Encoders)
                          └─> Phase 6 (Fix Duplicate Symbols)
                                └─> Phase 7 (Wire Core Model to API)
                                      └─> Phase 8 (Implement Generation + AVI Output)
```

## Requirements Coverage

| Phase | Requirement Count | IDs |
|-------|------------------|-----|
| 1 - Foundation | 8 | STRUCT-01,02,03 + CORE-01,02,03,04,05 |
| 2 - Build System | 4 | BUILD-01,02,03,04 |
| 3 - Public API | 5 | API-01,02,03,04,05 |
| 4 - Examples | 4 | EX-01,02,03,04 |
| 5 - Encoders | 2 | ENCODER-01,02 |
| 6 - Fix Duplicate Symbols | 2 | BUILD-01, API-05 |
| 7 - Wire Core Model to API | 6 | CORE-01,02,04 + API-02 + ENCODER-01,02 |
| 8 - Implement Generation + AVI Output | 3 | API-03, API-04, EX-02 |
| **Total** | **23** | **All v1 requirements** |

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

These features are tracked in REQUIREMENTS.md but not part of v1 roadmap.

---
*Roadmap created: 2026-03-12*
*Last updated: 2026-03-15 after Phase 5 addition*
