# Project Research Summary

**Project:** wan-cpp (Standalone C++ Library for WAN Video Generation)
**Domain:** C++ AI Inference Library (Video Generation)
**Researched:** 2026-03-12
**Confidence:** HIGH

## Executive Summary

This project extracts a standalone C++ library for WAN video generation from the stable-diffusion.cpp monorepo. Expert practice for such libraries involves: (1) using a C-compatible public API with opaque handles for ABI stability and language interoperability; (2) employing a modern CMake-based build system with proper target scoping; (3) managing the GGML dependency as a git submodule for version control; and (4) implementing RAII-based resource management for memory safety. The extraction follows established patterns from the parent project while creating clean separation from SD-specific code.

The recommended approach is to build in clearly defined phases: first establish the project skeleton and extract core code (Phase 1), then build out the CMake system with multi-backend support (Phase 2), create the public C API layer (Phase 3), and finally add production polish with examples and testing (Phase 4). Key risks include header dependency chains breaking during extraction, submodule synchronization issues, and macro name collisions when the library is used alongside the parent project. These are mitigated by maintaining a clean include directory structure, pinning GGML to specific commits, and namespacing all macros and constants.

## Key Findings

### Recommended Stack

**Core technologies:**
- **C++17**: Language standard — Required by ggml and parent project; provides sufficient modern features without sacrificing platform compatibility
- **CMake 3.20+**: Build system — Modern target-based approach with proper include directory scoping and cross-platform support
- **ggml 0.9.7+**: Tensor computation backend — Required dependency from parent project via git submodule; provides cross-platform ML operations with CUDA/Metal/Vulkan backends
- **Ninja**: Build generator — Significantly faster than Makefiles for large C++ projects
- **Catch2 3.4+**: Testing framework — Header-only option available, excellent CMake integration

**Supporting libraries (header-only):**
- **nlohmann/json 3.11+**: JSON parsing for config/model files — Single-header, zero-build dependency
- **stb_image**: Image I/O for video preprocessing — Minimal, widely-used
- **miniz**: ZIP compression for weight loading — Already used in parent project

### Expected Features

**Must have (table stakes):**
- **Model loading from GGUF files** — Standard format for GGML-based libraries, essential for weight management
- **Basic inference API (T2V, I2V, FLF2V)** — Core functionality for text-to-video, image-to-video, and flow-latent-flow-to-video
- **Configuration parameters** — Control generation quality, speed, and output (seed, steps, guidance scale, resolution)
- **Memory management** — Prevent OOM on resource-constrained devices through weight offloading and tensor allocation
- **Multi-backend support (CUDA, Metal, Vulkan, CPU)** — Cross-platform compatibility and hardware choice
- **CMake build system** — Standard C++ project build with multi-platform support
- **CLI interface** — Quick testing, validation, and user acceptance
- **Video output (AVI export)** — Save generated videos, basic expectation
- **Error handling** — Graceful failures with clear error messages

**Should have (competitive):**
- **Progress callbacks** — Enable UI integration and user feedback during generation
- **C-style API wrapper** — Language bindings for Python, Rust, and other languages

- **RAII-based resource management** — Automatic cleanup and memory safety

**Defer (v2+):**
- **Streaming output** — Can be added later, not blocking MVP
- **Memory pooling** — Optimization, not functional requirement
- **Batch generation** — Use multiple instances for now
- **Spectrum caching** — Complex feature, optional improvement

### Architecture Approach

The recommended architecture uses a layered design with a clean separation between public API (C-compatible with opaque handles) and internal C++ implementation. This ensures ABI stability while allowing modern C++ patterns internally. The GGML dependency is managed as a git submodule with CMake integration, and third-party dependencies are header-only to avoid build complexity. Key patterns include Builder pattern for configuration, RAII for resource management, and proper include directory scoping via modern CMake targets.

**Major components:**
1. **Public API Layer** — C-compatible interface with opaque handles (wan_context_t, wan_config_t) for language interoperability and ABI stability
2. **WAN Core Layer** — Core video generation model implementation including DiT blocks, VAE encoder/decoder, and flow matching
3. **Supporting Modules** — Rope position encoding, flux flow matching, common blocks, and GGML extensions
4. **Utility Layer** — Preprocessing, RNG utilities, tensor utilities, and GGML backend management
5. **Model Loader** — GGUF file parsing and memory allocation for model weights
6. **Context Manager** — RAII-based inference context and memory pool management

### Critical Pitfalls

**Top 5 pitfalls from research:**

1. **Hardcoded relative include paths** — Extracted headers reference other headers with relative paths that break in the new project structure. Avoid by creating unified include directory and using CMake's `${CMAKE_CURRENT_SOURCE_DIR}` for include paths.

2. **Submodule detached HEAD state** — GGML submodule ends up in detached HEAD causing inconsistent builds. Avoid by using `git submodule update --init --recursive` and pinning GGML to specific commit hash.

3. **Transitive header dependencies** — Headers compile in isolation but fail when used downstream because template dependencies aren't included. Avoid by using dependency discovery tools and including all headers in library sources.

4. **CMake target include directory scoping** — PRIVATE include directories become unavailable to downstream projects. Avoid by marking public headers as PUBLIC: `target_include_directories(wan PUBLIC include/)`.

5. **Preprocessor macro name collisions** — Macros like `CACHE_T` collide when library is used alongside stable-diffusion.cpp. Avoid by renaming all macros with WAN-specific prefix (`WAN_CACHE_T`).

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Project Skeleton and Code Extraction

**Rationale:** Must establish structure before functionality. This phase addresses critical pitfalls #1, #2, #3, #5, and #6 by setting up proper include paths, fixing submodule state, and renaming macros during extraction.

**Delivers:** Complete project structure with extracted WAN source code, git submodule for GGML, thirdparty dependencies, and macro namespace cleanup

**Addresses:** Model loading, CMake build system (basic), version detection

**Avoids:** Hardcoded relative includes, submodule detached HEAD, macro name collisions

### Phase 2: Build System and Multi-Backend Support

**Rationale:** Build system must support all backends before adding functionality. This phase addresses pitfalls #4 and #7 by configuring proper CMake target scoping and GGML backend integration.

**Delivers:** Complete CMakeLists.txt with backend options (CPU, CUDA, Metal, Vulkan), proper include directory scoping, and successful builds on all platforms

**Uses:** CMake 3.20+, Ninja, ggml submodule, platform-specific configuration

**Implements:** Multi-backend support, configuration parameters

**Avoids:** CMake target include directory scoping, GGML backend configuration mismatch

### Phase 3: Public API and Core Functionality

**Rationale:** Internal implementation must be exposed through stable public interface. This creates the C-compatible wrapper and implements the inference API.

**Delivers:** Public C API headers (wan.h, config.h, types.h), C++ wrapper implementation, basic inference for T2V and I2V modes

**Implements:** Basic inference API (T2V, I2V), C-style API wrapper, error handling, memory management (basic)

**Uses:** C API with opaque handles pattern, Builder pattern for configuration, RAII resource management

### Phase 4: Production Polish

**Rationale:** Once core functionality works, add features that make the library production-ready and usable.

**Delivers:** CLI interface, video output (AVI export), example programs, basic documentation, progress callbacks

**Implements:** CLI interface, video output, basic examples, progress callbacks, RAII-based resource management, verbose logging

### Phase 5: Quality Assurance and Optimization

**Rationale:** Verify quality and prepare for release.

**Delivers:** Test suite using Catch2, CI/CD configuration, performance optimizations, comprehensive documentation

**Implements:** Testing framework, error handling validation

### Phase Ordering Rationale

The phase order is driven by dependency chains discovered in research:

- **Structure before code**: You cannot extract code without a proper project structure that addresses include path pitfalls (#1)
- **Build system before functionality**: You must have a working CMake setup before you can implement the API, otherwise you cannot compile/test code
- **API before polish**: The public interface must be defined before building examples or CLI tools
- **Functionality before quality**: You cannot test or optimize features that don't exist yet

This grouping based on architecture patterns ensures:
- Phase 1-2 handle infrastructure (structure + build system)
- Phase 3 delivers the core library (public API + inference)
- Phase 4 adds user-facing features (CLI + examples)
- Phase 5 validates and optimizes (testing + CI/CD)

### Research Flags

Phases likely needing deeper research during planning:

- **Phase 3 (Public API and Core Functionality):** Complex integration requiring API design research. The exact shape of the C API and callback mechanisms need careful consideration to balance usability with ABI stability.

- **Phase 2 (Build System and Multi-Backend Support):** Backend-specific integration patterns vary (CUDA vs Metal vs Vulkan). Each backend may have platform-specific requirements that need validation during planning.

Phases with standard patterns (skip research-phase):

- **Phase 1 (Project Skeleton and Code Extraction):** Well-documented through analysis of stable-diffusion.cpp codebase. The extraction path and file organization are clear from existing code structure.

- **Phase 4 (Production Polish):** CLI tools and AVI export follow established patterns from parent project's examples/cli directory.

- **Phase 5 (Quality Assurance and Optimization):** Catch2 testing and GitHub Actions CI follow standard C++ project patterns documented in Catch2 and CMake guides.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Based on analysis of stable-diffusion.cpp and ggml source code; CMake and C++17 requirements are explicit in the codebase |
| Features | MEDIUM | Based on code inspection and WebSearch results; official documentation not directly accessible but patterns from parent project are clear |
| Architecture | HIGH | Based on extensive analysis of existing code structure; patterns (C API, RAII, Builder) are established best practices |
| Pitfalls | MEDIUM | Based on WebSearch results and code analysis; some pitfalls are theoretical until encountered, but preventive strategies are well-defined |

**Overall confidence:** HIGH

The research is grounded in extensive analysis of the stable-diffusion.cpp codebase (HIGH confidence sources) supplemented by WebSearch for domain patterns (MEDIUM confidence). The key findings about stack, architecture, and pitfalls are supported by direct code inspection, while feature analysis combines code evidence with industry patterns.

### Gaps to Address

- **Public API design details:** The exact function signatures and callback mechanisms for the C API need validation during Phase 3 planning. Handle by prototyping with simple T2V example first, then expanding.

- **Backend-specific behavior:** CUDA, Metal, and Vulkan backends may have subtle differences in tensor handling not captured in the research. Handle by testing each backend independently during Phase 2 and documenting any quirks.

- **Model variant compatibility:** The exact differences between WAN2.1 and WAN2.2.2 (TI2V) models in terms of parameter compatibility need runtime validation. Handle by implementing version detection in Phase 1 and testing with multiple model files.

- **Memory requirements:** Specific memory footprint for different resolutions and video lengths is not precisely quantified. Handle by documenting expected memory usage based on testing and providing user-controllable buffer sizes.

## Sources

### Primary (HIGH confidence)

- **stable-diffusion.cpp source code** — Analyzed actual codebase at `/home/jtzhuang/projects/stable-diffusion.cpp` including `wan.hpp`, `common_block.hpp`, `rope_pos.hpp`, `vae_encoder.hpp`, `flux.hpp`, examples/cli/main.cpp
- **ggml source code** — Analyzed GGML library structure and CMake integration patterns
- **CMake 3.20+ Documentation** — Modern CMake target-based approach and include directory scoping

### Secondary (MEDIUM confidence)

- **WebSearch on C++ code extraction pitfalls** — General patterns and anti-patterns for library extraction
- **WebSearch on git submodule management** — Best practices for submodule synchronization
- **C++ Core Guidelines** — Established best practices for C++ library development
- **llama.cpp patterns** — Similar standalone C++ library structure (referenced via WebSearch)

### Tertiary (LOW confidence)

- **WebSearch on video generation libraries** — General context about AI inference libraries
- **ONNX Runtime C++ API documentation** — Referenced for comparison but not directly applicable
- **Catch2 documentation** — Standard testing framework but not critical to core functionality

---
*Research completed: 2026-03-12*
*Ready for roadmap: yes*
