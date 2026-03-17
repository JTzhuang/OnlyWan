# Codebase Structure

**Analysis Date:** 2026-03-17

## Directory Layout

```
wan/
├── include/wan-cpp/          # Public headers
│   ├── wan.h                 # C API interface
│   └── wan-internal.hpp      # Internal C++ structures
├── src/                      # Core implementation
│   ├── api/                  # C API implementation
│   │   ├── wan-api.cpp       # Main API functions
│   │   ├── wan_config.cpp    # Backend configuration
│   │   ├── wan_t2v.cpp       # Text-to-video generation
│   │   ├── wan_i2v.cpp       # Image-to-video generation
│   │   └── wan_loader.cpp    # Model format detection
│   ├── vocab/                # Tokenizers
│   │   ├── vocab.h/cpp       # Tokenizer registry
│   │   ├── clip_t5.hpp       # CLIP/T5 tokenizers
│   │   ├── mistral.hpp       # Mistral tokenizer
│   │   ├── qwen.hpp          # Qwen tokenizer
│   │   └── umt5.hpp          # UMT5 tokenizer
│   ├── wan.hpp               # WAN model runner
│   ├── flux.hpp              # Flux model runner
│   ├── t5.hpp                # T5 text encoder
│   ├── clip.hpp              # CLIP vision encoder
│   ├── vae.hpp               # VAE decoder
│   ├── common_dit.hpp        # DiT utilities (patchify, etc.)
│   ├── common_block.hpp      # Base neural network blocks
│   ├── model.h/cpp           # Model loader
│   ├── name_conversion.h/cpp # Tensor name mapping
│   ├── tokenize_util.h/cpp   # Tokenization utilities
│   ├── util.h/cpp            # General utilities
│   ├── ggml_extend.hpp       # GGML extensions
│   ├── gguf_reader.hpp       # GGUF parsing
│   ├── rope.hpp              # RoPE positional encoding
│   ├── rng.hpp               # RNG interface
│   ├── rng_mt19937.hpp       # Mersenne Twister
│   ├── rng_philox.hpp        # Philox RNG
│   ├── preprocessing.hpp     # Image preprocessing
│   ├── ordered_map.hpp       # Ordered map container
│   ├── json.hpp              # JSON parsing
│   └── wan-types.h           # Type definitions
├── examples/                 # Example applications
│   ├── cli/                  # CLI tool
│   │   ├── main.cpp          # CLI implementation
│   │   ├── avi_writer.h/c    # AVI video encoding
│   │   └── README.md         # CLI documentation
│   ├── convert/              # Model converter
│   │   ├── main.cpp          # Converter implementation
│   │   └── README.md         # Converter documentation
│   └── README.md             # Examples overview
├── thirdparty/               # External dependencies
│   ├── zip.h                 # ZIP file handling
│   └── stb_image.h           # Image loading
├── CMakeLists.txt            # Build configuration
├── .clang-format             # Code formatting rules
├── .clang-tidy               # Linting configuration
├── format-code.sh            # Format script
└── README.md                 # Project overview
```

## Directory Purposes

**include/wan-cpp/:**
- Purpose: Public API headers
- Contains: C interface, internal structures for implementation
- Key files: `wan.h` (stable C API), `wan-internal.hpp` (C++ internals)

**src/api/:**
- Purpose: C API implementation and model loading
- Contains: API functions, backend setup, generation logic
- Key files: `wan-api.cpp` (main implementation), `wan_loader.cpp` (format detection)

**src/vocab/:**
- Purpose: Text tokenization for different models
- Contains: Tokenizer implementations, vocabulary data
- Key files: `vocab.h` (registry), `clip_t5.hpp`, `mistral.hpp`, `qwen.hpp`, `umt5.hpp`

**src/ (root):**
- Purpose: Core model implementations and utilities
- Contains: Model runners (WAN, Flux), encoders (T5, CLIP), VAE, utilities
- Key files: `wan.hpp`, `flux.hpp`, `t5.hpp`, `clip.hpp`, `vae.hpp`, `model.h`

**examples/cli/:**
- Purpose: Full-featured command-line tool
- Contains: CLI argument parsing, progress display, video output
- Key files: `main.cpp` (CLI logic), `avi_writer.h/c` (video encoding)

**examples/convert/:**
- Purpose: Model format conversion utility
- Contains: Safetensors → GGUF conversion logic
- Key files: `main.cpp` (converter implementation)

**thirdparty/:**
- Purpose: External dependencies
- Contains: ZIP file handling, image loading libraries
- Key files: `zip.h`, `stb_image.h`

## Key File Locations

**Entry Points:**
- `include/wan-cpp/wan.h`: Public C API entry point
- `examples/cli/main.cpp`: CLI application entry point
- `examples/convert/main.cpp`: Model converter entry point

**Configuration:**
- `CMakeLists.txt`: Build system configuration
- `.clang-format`: Code style rules
- `.clang-tidy`: Static analysis rules

**Core Logic:**
- `src/api/wan-api.cpp`: Main API implementation (36KB)
- `src/model.cpp`: Model loading and tensor management
- `src/wan.hpp`: WAN model runner (header-only)
- `src/flux.hpp`: Flux model runner (header-only)

**Testing:**
- `examples/cli/main.cpp`: Functional test via CLI
- `examples/convert/main.cpp`: Format conversion test

## Naming Conventions

**Files:**
- `*.h` - C headers (public API, type definitions)
- `*.hpp` - C++ headers (implementations, templates)
- `*.cpp` - C++ source files (implementations)
- `*.c` - C source files (utilities like AVI writer)

**Directories:**
- `src/api/` - API implementation
- `src/vocab/` - Vocabulary/tokenizer modules
- `examples/` - Example applications
- `thirdparty/` - External dependencies

**Naming Patterns:**
- Model runners: `{ModelName}Runner` (e.g., `WanRunner`, `FluxRunner`)
- Encoders: `{ModelName}Embedder` (e.g., `T5Embedder`, `CLIPVisionModelProjectionRunner`)
- Blocks: `{BlockType}` (e.g., `Linear`, `Attention`, `RMSNorm`)
- Tokenizers: `{ModelName}Tokenizer` (e.g., `CLIPTokenizer`, `T5Tokenizer`)

## Where to Add New Code

**New Feature (e.g., new generation mode):**
- Primary code: `src/api/wan_*.cpp` (new file for feature)
- Header: `include/wan-cpp/wan.h` (add C API functions)
- Internal: `include/wan-cpp/wan-internal.hpp` (add internal structures)
- Example: `examples/cli/main.cpp` (add CLI support)

**New Model Support (e.g., new diffusion architecture):**
- Implementation: `src/{model_name}.hpp` (header-only runner)
- Loader: `src/api/wan_loader.cpp` (add format detection)
- Tokenizer: `src/vocab/{model_name}.hpp` (if needed)
- Example: `examples/convert/main.cpp` (add conversion support)

**New Tokenizer:**
- Implementation: `src/vocab/{model_name}.hpp`
- Registry: `src/vocab/vocab.h` (add to enum)
- Loader: `src/vocab/vocab.cpp` (add instantiation)

**Utilities:**
- Shared helpers: `src/util.h/cpp`
- GGML extensions: `src/ggml_extend.hpp`
- Image processing: `src/preprocessing.hpp`

## Special Directories

**build/:**
- Purpose: CMake build output
- Generated: Yes
- Committed: No
- Contains: Object files, executables, CMake cache

**models/:**
- Purpose: Model storage (not committed)
- Generated: No (user-provided)
- Committed: No
- Contains: GGUF model files

**.planning/:**
- Purpose: Project planning and documentation
- Generated: Yes (by GSD tools)
- Committed: Yes
- Contains: Roadmaps, requirements, phase documentation

**.claude/:**
- Purpose: Claude Code workspace metadata
- Generated: Yes
- Committed: No
- Contains: Session state, tool results

## Build System

**CMake Configuration:**
- Minimum version: 3.20
- C++ standard: C++17
- Default build type: Release
- Output: `build/bin/` for executables and libraries

**Backend Options (CMakeLists.txt):**
- `WAN_CUDA` - NVIDIA GPU support
- `WAN_METAL` - Apple Metal support
- `WAN_VULKAN` - Vulkan support
- `WAN_OPENCL` - OpenCL support
- `WAN_SYCL` - Intel SYCL support
- `WAN_HIPBLAS` - AMD ROCm support
- `WAN_MUSA` - MUSA support
- `WAN_BUILD_SHARED_LIBS` - Build as shared library
- `WAN_EMBED_VOCAB` - Embed vocab in binary (adds ~85MB, speeds up build)

**Library Targets:**
- `wan-cpp` - Main library (static or shared)
- `wan-cli` - CLI executable
- `wan-convert` - Model converter executable
- `ggml` - GGML backend (submodule)
- `zip` - ZIP library (thirdparty)
