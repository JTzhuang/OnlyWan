# Codebase Structure

**Analysis Date:** 2026-03-28

## Directory Layout

```
OnlyWan/
├── src/                          # Core model implementations
│   ├── wan.hpp                   # WAN Transformer (DiT) - main model
│   ├── vae.hpp                   # Video VAE encoder/decoder
│   ├── clip.hpp                  # CLIP text encoder
│   ├── t5.hpp                    # T5 text encoder
│   ├── common_block.hpp           # Shared neural network blocks
│   ├── ggml_extend.hpp            # GGML operation wrappers
│   ├── rope.hpp                   # RoPE positional encoding
│   ├── model_factory.cpp          # Model registration & instantiation
│   ├── model_registry.hpp         # Model registry interface
│   ├── model.cpp/model.h          # Base model classes
│   ├── config_loader.hpp/cpp      # Configuration loading
│   ├── preprocessing.hpp          # Input preprocessing
│   ├── api/                       # High-level API
│   │   ├── wan-api.cpp            # Main API entry point
│   │   ├── wan_t2v.cpp            # Text-to-video API
│   │   ├── wan_i2v.cpp            # Image-to-video API
│   │   ├── wan_loader.cpp         # Model loading utilities
│   │   └── wan_config.cpp         # Configuration utilities
│   ├── vocab/                     # Tokenizer vocabularies
│   │   ├── clip_t5.hpp
│   │   ├── mistral.hpp
│   │   ├── qwen.hpp
│   │   └── umt5.hpp
│   └── util.cpp/util.h            # General utilities
├── tests/cpp/                     # C++ test suite
│   ├── test_clip.cpp              # CLIP model inference test
│   ├── test_t5.cpp                # T5 model inference test
│   ├── test_vae.cpp               # VAE encode/decode test
│   ├── test_transformer.cpp       # WAN Transformer inference test
│   ├── test_factory.cpp           # Model factory test
│   ├── test_io_npy.cpp            # NumPy I/O test
│   ├── test_framework.hpp         # Test utilities
│   ├── test_helpers.hpp           # Test helper functions
│   ├── test_io_utils.hpp          # I/O utilities for tests
│   ├── model_factory.hpp          # Test model factory
│   └── test_data/                 # Test data files
└── build/                         # Build output directory
    └── bin/                       # Compiled binaries
```

## Directory Purposes

**src/ - Core Implementation:**
- Purpose: All model implementations and core logic
- Contains: Model runners, blocks, utilities, API
- Key files: wan.hpp (2300+ lines), vae.hpp, clip.hpp, t5.hpp

**src/api/ - High-Level API:**
- Purpose: User-facing API for model inference
- Contains: T2V, I2V pipeline orchestration
- Key files: wan-api.cpp, wan_t2v.cpp, wan_i2v.cpp

**src/vocab/ - Tokenizer Vocabularies:**
- Purpose: Vocabulary data for different tokenizers
- Contains: CLIP, T5, Mistral, Qwen vocabularies
- Key files: clip_t5.hpp, umt5.hpp

**tests/cpp/ - Test Suite:**
- Purpose: Unit and integration tests for all models
- Contains: Individual model tests, factory tests, I/O tests
- Key files: test_clip.cpp, test_t5.cpp, test_vae.cpp, test_transformer.cpp

**build/ - Build Artifacts:**
- Purpose: Compiled binaries and build outputs
- Contains: Executable binaries in build/bin/
- Generated: CMake build system output

## Key File Locations

**Entry Points:**
- `src/api/wan-api.cpp`: Main API entry point
- `src/api/wan_t2v.cpp`: T2V pipeline entry
- `src/api/wan_i2v.cpp`: I2V pipeline entry
- `src/model_factory.cpp`: Model instantiation entry

**Configuration:**
- `src/config_loader.hpp`: Configuration loading interface
- `src/config_loader.cpp`: Configuration implementation
- `src/wan-types.h`: Type definitions and constants

**Core Logic:**
- `src/wan.hpp`: WAN Transformer implementation (lines 1766-2318)
- `src/vae.hpp`: VAE implementation (lines 922-1297)
- `src/clip.hpp`: CLIP text encoder
- `src/t5.hpp`: T5 text encoder

**Testing:**
- `tests/cpp/test_transformer.cpp`: WAN Transformer tests
- `tests/cpp/test_vae.cpp`: VAE tests
- `tests/cpp/test_clip.cpp`: CLIP tests
- `tests/cpp/test_t5.cpp`: T5 tests

## Naming Conventions

**Files:**
- Model implementations: `{model_name}.hpp` (e.g., wan.hpp, vae.hpp)
- Tests: `test_{component}.cpp` (e.g., test_transformer.cpp)
- Utilities: `{purpose}.hpp` or `{purpose}.cpp` (e.g., ggml_extend.hpp)
- API: `wan_{variant}.cpp` (e.g., wan_t2v.cpp)

**Directories:**
- Core models: `src/`
- API layer: `src/api/`
- Vocabularies: `src/vocab/`
- Tests: `tests/cpp/`
- Build output: `build/`

## Where to Add New Code

**New Feature (e.g., new model variant):**
- Primary code: `src/wan.hpp` (add new WanParams configuration)
- Registration: `src/model_factory.cpp` (add REGISTER_MODEL_FACTORY)
- Tests: `tests/cpp/test_transformer.cpp` (add test case)

**New Component/Module (e.g., new attention mechanism):**
- Implementation: `src/wan.hpp` or `src/common_block.hpp` (add new class inheriting from GGMLBlock)
- Integration: Update parent block to use new component
- Tests: `tests/cpp/test_transformer.cpp` (add unit test)

**Utilities:**
- Shared helpers: `src/ggml_extend.hpp` (GGML wrappers)
- General utilities: `src/util.cpp` / `src/util.h`
- Preprocessing: `src/preprocessing.hpp`

**API Endpoints:**
- New pipeline: `src/api/wan_{variant}.cpp`
- Configuration: `src/config_loader.cpp`
- Loader utilities: `src/api/wan_loader.cpp`

## Special Directories

**src/vocab/:**
- Purpose: Tokenizer vocabulary data
- Generated: No (pre-built vocabularies)
- Committed: Yes (part of source)

**tests/cpp/test_data/:**
- Purpose: Test data files (binary tensors, reference outputs)
- Generated: No (pre-built test fixtures)
- Committed: Yes (small test files)

**build/:**
- Purpose: CMake build output
- Generated: Yes (by CMake)
- Committed: No (.gitignore)

## Model Variant Structure

**WAN Transformer Variants (src/wan.hpp):**
- T2V (Text-to-Video): 1.3B (30 layers, dim=1536) or 14B (40 layers, dim=5120)
- I2V (Image-to-Video): 1.3B or 14B with img_emb module
- TI2V (Text+Image-to-Video): 5B (30 layers, dim=3072)
- VACE: Optional Video Augmented Cross-attention Enhancement layers

**VAE Variants (src/vae.hpp):**
- wan-vae-t2v: Full encode/decode for T2V
- wan-vae-t2v-decode: Decode-only for inference
- wan-vae-i2v: Full encode/decode for I2V
- wan-vae-ti2v: Full encode/decode for TI2V

**Text Encoder Variants:**
- CLIP: clip-vit-l-14, clip-vit-h-14, clip-vit-bigg-14
- T5: t5-standard, t5-umt5

## Class Hierarchy

**GGMLRunner (Base):**
```
GGMLRunner (src/model.h)
├── WanRunner (src/wan.hpp, line 2020)
├── CLIPTextModelRunner (src/clip.hpp)
├── T5Runner (src/t5.hpp)
└── WanVAERunner (src/vae.hpp, line 1119)
```

**GGMLBlock (Component Base):**
```
GGMLBlock (src/ggml_extend.hpp)
├── Conv2d, Conv3d
├── Linear, LayerNorm
├── Attention blocks
├── Residual blocks
├── Wan-specific blocks:
│   ├── WanAttentionBlock (line 1503)
│   ├── WanSelfAttention (line 1299)
│   ├── WanCrossAttention (line 1359)
│   ├── WanT2VCrossAttention (line 1372)
│   ├── WanI2VCrossAttention (line 1410)
│   └── VaceWanAttentionBlock (line 1595)
└── VAE-specific blocks:
    ├── Encoder3d (line 588)
    ├── Decoder3d (line 751)
    ├── ResidualBlock (line 338)
    └── AttentionBlock (line 527)
```

---

*Structure analysis: 2026-03-28*
