# Technology Stack

**Analysis Date:** 2026-03-28

## Languages

**Primary:**
- C++ 17 - Core library implementation, model inference, video generation
- C - Public API interface, CLI examples, utility functions

**Secondary:**
- CMake - Build system configuration

## Runtime

**Environment:**
- Cross-platform: Linux, macOS (Metal), Windows, and specialized hardware

**Build System:**
- CMake 3.20+ - Primary build configuration
- Make - Build execution

## Frameworks

**Core:**
- GGML (ggml) - Machine learning inference framework for model computation
  - Location: `ggml/` (git submodule)
  - Purpose: Tensor operations, backend abstraction, model loading
  - Supports multiple backends: CPU, CUDA, Metal, Vulkan, OpenCL, SYCL, ROCm (HIP), MUSA

**Video Processing:**
- AVI Writer - Custom video encoding to AVI format
  - Location: `examples/cli/avi_writer.c`, `examples/cli/avi_writer.h`
  - Purpose: Output video file generation

**Image Processing:**
- stb_image - Single-header image loading library
  - Location: `thirdparty/stb_image.h`
  - Purpose: Load input images for I2V generation

- stb_image_resize - Single-header image resizing
  - Location: `thirdparty/stb_image_resize.h`
  - Purpose: Resize images to model input dimensions

- stb_image_write - Single-header image writing
  - Location: `thirdparty/stb_image_write.h`
  - Purpose: Write processed images

**Tokenization & NLP:**
- DARTS (Double-Array Trie) - Trie-based tokenizer
  - Location: `thirdparty/darts.h`
  - Purpose: Efficient vocabulary lookup for text tokenization

- SentencePiece - Subword tokenization (ported)
  - Location: `src/t5.hpp`
  - Purpose: T5 model tokenization

**Utilities:**
- JSON - nlohmann/json single-header library
  - Location: `thirdparty/json.hpp`
  - Purpose: Model configuration parsing, metadata handling

- ZIP - miniz-based compression library
  - Location: `thirdparty/zip.c`, `thirdparty/zip.h`, `thirdparty/miniz.h`
  - Purpose: GGUF model file handling, archive operations

- ordered_map - Ordered key-value container
  - Location: `thirdparty/ordered_map.hpp`
  - Purpose: Maintain insertion order in configuration maps

## Key Dependencies

**Critical:**
- GGML - Inference engine, tensor computation, backend abstraction
  - Why it matters: Core to all model inference operations; enables multi-backend support

- stb_image - Image loading for I2V mode
  - Why it matters: Enables image-to-video generation capability

**Infrastructure:**
- ZIP library (miniz) - Model file handling
  - Purpose: Read/write GGUF model archives

- JSON library - Configuration parsing
  - Purpose: Parse model metadata and generation parameters

## Configuration

**Environment:**
- Backend selection via CMake options: `WAN_CUDA`, `WAN_METAL`, `WAN_VULKAN`, `WAN_OPENCL`, `WAN_SYCL`, `WAN_HIPBLAS`, `WAN_MUSA`
- Vocab embedding: `WAN_EMBED_VOCAB` (OFF by default for faster builds)
- Build type: Release (default), Debug, MinSizeRel, RelWithDebInfo

**Build:**
- `CMakeLists.txt` - Main build configuration
- `ggml/CMakeLists.txt` - GGML dependency configuration
- `thirdparty/CMakeLists.txt` - Third-party library setup
- `examples/CMakeLists.txt` - Example applications

**Compiler Flags:**
- MSVC: `_CRT_SECURE_NO_WARNINGS`, `_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING`
- SYCL: `-Wno-narrowing -fsycl` with precision flags
- GGML_MAX_NAME=128 - Maximum tensor name length

## Platform Requirements

**Development:**
- C++17 compatible compiler (GCC, Clang, MSVC)
- CMake 3.20+
- Git (for version detection)
- Optional: CUDA toolkit (for CUDA backend)
- Optional: Metal SDK (for macOS Metal backend)
- Optional: Vulkan SDK (for Vulkan backend)

**Production:**
- Deployment target: Any platform with C++17 support
- Hardware backends: CPU (always available), GPU (CUDA/Metal/Vulkan/OpenCL/SYCL/ROCm/MUSA optional)
- Memory: Varies by model size (GGUF format supports quantization for reduced memory)

---

*Stack analysis: 2026-03-28*
