# Architecture

**Analysis Date:** 2026-03-17

## Pattern Overview

**Overall:** Layered C++ library with C API wrapper for video generation

**Key Characteristics:**
- Header-only model implementations (DiT, VAE, text encoders)
- GGML-based tensor computation backend with pluggable hardware support
- C API facade (`wan.h`) wrapping C++ internals (`wan-internal.hpp`)
- Model-agnostic loader supporting GGUF, safetensors, CKPT, and diffusers formats
- Modular encoder/decoder architecture (T5, CLIP, VAE)

## Layers

**Public C API Layer:**
- Purpose: Stable C interface for video generation (T2V, I2V)
- Location: `include/wan-cpp/wan.h`
- Contains: Function declarations, error codes, opaque handles, parameter structures
- Depends on: None (pure C interface)
- Used by: CLI examples, external applications

**Internal C++ API Layer:**
- Purpose: Bridge between C API and implementation, manage context lifecycle
- Location: `include/wan-cpp/wan-internal.hpp`, `src/api/wan-api.cpp`
- Contains: Context structures, model loading, parameter conversion, error handling
- Depends on: GGML, model runners, encoders
- Used by: C API implementation

**Model Runner Layer:**
- Purpose: Execute diffusion models (WAN, Flux, SD variants)
- Location: `src/wan.hpp`, `src/flux.hpp`, `src/common_dit.hpp`
- Contains: DiT blocks, attention mechanisms, normalization, forward passes
- Depends on: GGML, common blocks, RoPE, VAE
- Used by: API layer for inference

**Encoder/Decoder Layer:**
- Purpose: Text and image encoding, VAE decoding
- Location: `src/t5.hpp`, `src/clip.hpp`, `src/vae.hpp`
- Contains: T5 text encoder, CLIP vision encoder, VAE decoder
- Depends on: GGML, common blocks
- Used by: Model runners for conditioning

**Model Loading Layer:**
- Purpose: Load and convert model weights from various formats
- Location: `src/model.h`, `src/model.cpp`, `src/api/wan_loader.cpp`
- Contains: `ModelLoader` class, format detection, tensor storage, conversion
- Depends on: GGML, GGUF, zip, safetensors parsing
- Used by: API layer during model initialization

**Vocabulary Layer:**
- Purpose: Tokenization for text encoders
- Location: `src/vocab/vocab.h`, `src/vocab/vocab.cpp`, `src/vocab/*.hpp`
- Contains: Tokenizers (T5, CLIP, Mistral, Qwen, UMT5)
- Depends on: JSON parsing, utility functions
- Used by: T5 and CLIP encoders

**Utility Layer:**
- Purpose: Common utilities and helpers
- Location: `src/util.h`, `src/util.cpp`, `src/ggml_extend.hpp`, `src/rng*.hpp`
- Contains: String utilities, image processing, GGML extensions, RNG implementations
- Depends on: GGML, standard library
- Used by: All layers

**Backend Layer:**
- Purpose: Hardware acceleration abstraction
- Location: `src/api/wan_config.cpp`, CMakeLists.txt
- Contains: Backend selection (CPU, CUDA, Metal, Vulkan, OpenCL, SYCL, HIP, MUSA)
- Depends on: GGML backend system
- Used by: Context initialization

## Data Flow

**Text-to-Video (T2V) Generation:**

1. User calls `wan_generate_video_t2v()` with prompt and parameters
2. C API converts parameters to internal `WanParams` structure
3. T5 encoder tokenizes prompt → text embeddings
4. WAN DiT model processes embeddings + noise → latent video
5. VAE decoder converts latent → RGB video frames
6. AVI writer encodes frames to output file
7. Progress callback invoked at each diffusion step

**Image-to-Video (I2V) Generation:**

1. User calls `wan_generate_video_i2v()` with image and optional prompt
2. Image loaded and preprocessed (resize, normalize)
3. CLIP vision encoder processes image → image embeddings
4. Optional: T5 encoder processes prompt → text embeddings
5. WAN DiT model processes embeddings + noise → latent video
6. VAE decoder converts latent → RGB video frames
7. AVI writer encodes frames to output file

**Model Loading:**

1. User calls `wan_load_model()` with GGUF file path
2. `ModelLoader` detects format (GGUF, safetensors, CKPT, diffusers)
3. Tensor metadata extracted and stored in `tensor_storage_map`
4. Model version detected from metadata (WAN2.1, WAN2.2, etc.)
5. Appropriate runner instantiated (WAN, Flux, SD variants)
6. Encoders (T5, CLIP) and VAE loaded
7. Context returned with all components ready

**State Management:**
- `wan_context` holds all model runners, encoders, backend, and parameters
- Runners are shared_ptr for safe multi-threaded access
- Backend context created once per context, reused for all operations
- Tensor storage map persists for weight access during inference

## Key Abstractions

**GGMLBlock:**
- Purpose: Base class for composable neural network components
- Examples: `src/common_block.hpp` (Linear, Conv2d, Attention, Norm)
- Pattern: Virtual `forward()` method, `params` map for weights, `blocks` map for sub-modules

**ModelLoader:**
- Purpose: Format-agnostic model weight loading
- Examples: `src/model.h`, `src/model.cpp`
- Pattern: Polymorphic `init_from_*()` methods, tensor name conversion, quantization support

**Runner Classes:**
- Purpose: Stateful inference engines for specific models
- Examples: `WAN::WanRunner`, `Flux::FluxRunner`, `VAE::VAEDecoder`
- Pattern: Shared context, forward pass returns GGML tensor, manages computation graph

**Embedder Classes:**
- Purpose: Text and image encoding
- Examples: `T5Embedder`, `CLIPVisionModelProjectionRunner`
- Pattern: Tokenize input → embed → return tensor

## Entry Points

**C API Entry Points:**
- `wan_load_model()` - Load model from GGUF file
- `wan_generate_video_t2v()` - Generate video from text
- `wan_generate_video_i2v()` - Generate video from image
- `wan_free()` - Clean up context

**CLI Entry Points:**
- `examples/cli/main.cpp` - Full-featured CLI with T2V/I2V support
- `examples/convert/main.cpp` - Model format converter (safetensors → GGUF)

**Library Entry Points:**
- `include/wan-cpp/wan.h` - Public C API
- `src/api/wan-api.cpp` - C API implementation
- `src/api/wan_t2v.cpp` - T2V generation logic
- `src/api/wan_i2v.cpp` - I2V generation logic

## Error Handling

**Strategy:** Error codes + error message strings

**Patterns:**
- All C API functions return `wan_error_t` enum (WAN_SUCCESS, WAN_ERROR_*)
- Error messages stored in `wan_context->last_error` (std::string)
- Retrieved via `wan_get_last_error()`
- C++ exceptions caught at API boundary, converted to error codes
- Invalid arguments validated before processing

## Cross-Cutting Concerns

**Logging:**
- Global callback `g_log_callback` set via `wan_set_log_callback()`
- Levels: DEBUG (0), INFO (1), WARN (2), ERROR (3)
- Used for model loading, inference progress, backend selection

**Validation:**
- Model format detection in `wan_loader.cpp` (GGUF metadata checks)
- Tensor shape validation during loading
- Parameter bounds checking (width, height, steps, etc.)

**Authentication:**
- None (local inference only)

**Threading:**
- `n_threads` parameter controls GGML parallelism
- 0 = auto-detect CPU count
- Runners are thread-safe via shared_ptr and GGML backend synchronization
- Progress callbacks invoked from inference thread
