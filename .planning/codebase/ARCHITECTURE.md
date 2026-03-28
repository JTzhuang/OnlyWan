# Architecture

**Analysis Date:** 2026-03-28

## Pattern Overview

**Overall:** Hierarchical Model Runner Pattern with Modular Component Architecture

**Key Characteristics:**
- Model factory registration system for dynamic model instantiation
- Inheritance-based runner hierarchy (GGMLRunner → specialized runners)
- Composition of reusable blocks (GGMLBlock) for neural network layers
- Separation of concerns: tokenization, embedding, inference, I/O
- Support for multiple model variants (T2V, I2V, TI2V) through configuration

## Layers

**Model Registry & Factory Layer:**
- Purpose: Centralized model registration and instantiation
- Location: `src/model_factory.cpp`, `src/model_registry.hpp`
- Contains: Model factory registrations for CLIP, T5, WAN VAE, WAN Transformer
- Depends on: Individual model runner classes
- Used by: Application code to load models by name

**Model Runner Layer:**
- Purpose: Orchestrate model inference and graph building
- Location: `src/wan.hpp` (WanRunner), `src/clip.hpp` (CLIPTextModelRunner), `src/t5.hpp` (T5Runner), `src/vae.hpp` (WanVAERunner)
- Contains: Runner classes that inherit from GGMLRunner
- Depends on: GGMLBlock components, GGML backend
- Used by: API layer and application code

**Block/Component Layer:**
- Purpose: Reusable neural network building blocks
- Location: `src/common_block.hpp`, `src/wan.hpp`, `src/vae.hpp`
- Contains: Conv2d, Conv3d, Linear, LayerNorm, Attention blocks, etc.
- Depends on: GGML operations (ggml_extend.hpp)
- Used by: Model runners to construct computation graphs

**GGML Extension Layer:**
- Purpose: Wrapper functions around GGML operations
- Location: `src/ggml_extend.hpp`
- Contains: Helper functions for tensor operations, attention, convolution, etc.
- Depends on: GGML library
- Used by: All block implementations

**Utility & Support Layer:**
- Purpose: Tokenization, preprocessing, I/O, configuration
- Location: `src/clip.hpp` (CLIPTokenizer), `src/t5.hpp` (T5UniGramTokenizer), `src/preprocessing.hpp`, `src/config_loader.hpp`
- Contains: Tokenizers, preprocessors, model loaders
- Depends on: Vocabulary files, JSON config
- Used by: API layer

## Data Flow

**Inference Pipeline (T2V Example):**

1. **Input Preparation**
   - Text prompt → CLIPTokenizer → token IDs
   - Token IDs → CLIPTextModelRunner → text embeddings [N, L, text_dim]
   - Noise tensor [N*C, T, H, W] → VAE encoder (optional)

2. **WAN Transformer Forward Pass**
   - Input: x [N*C, T, H, W], timestep [N], context [N, L, text_dim]
   - Patch embedding: x → [N, t_len*h_len*w_len, dim]
   - Time embedding: timestep → [N, dim] or [N, T, dim]
   - Text embedding: context → [N, L, dim]
   - Positional encoding: RoPE → [pos_len, axes_dim_sum/2, 2, 2]
   - Transformer blocks (32-40 layers): self-attention + cross-attention + FFN
   - Head: output → [N, t_len*h_len*w_len, pt*ph*pw*C]
   - Unpatchify: → [N*C, T, H, W]

3. **VAE Decoding**
   - Latent z [N*C, T, H, W] → WanVAERunner.decode()
   - Decoder3d: upsampling + residual blocks + attention
   - Output: video frames [N*C, T, H, W]

**State Management:**
- Computation graph built per inference step
- Tensor caching for positional encodings (OP-02)
- Feature cache for VAE encoder/decoder (feat_cache)
- Backend tensor management (CPU/GPU offloading)

## Key Abstractions

**GGMLRunner (Base Class):**
- Purpose: Abstract base for all model runners
- Examples: `src/wan.hpp` (WanRunner), `src/clip.hpp` (CLIPTextModelRunner), `src/t5.hpp` (T5Runner), `src/vae.hpp` (WanVAERunner)
- Pattern: Template method pattern - subclasses implement build_graph() and compute()

**GGMLBlock (Component Base):**
- Purpose: Reusable neural network layer
- Examples: Conv2d, Linear, LayerNorm, Attention, ResidualBlock
- Pattern: Composite pattern - blocks contain sub-blocks via `blocks` map

**WanRunner (Transformer DiT):**
- Purpose: Diffusion Transformer for video generation
- Variants: T2V (text-to-video), I2V (image-to-video), TI2V (text+image-to-video)
- Key components: Patch embedding, text embedding, time embedding, transformer blocks, head
- Supports: VACE (Video Augmented Cross-attention Enhancement) layers

**WanVAERunner (Video VAE):**
- Purpose: Encode/decode video to/from latent space
- Variants: wan-vae-t2v, wan-vae-t2v-decode, wan-vae-i2v, wan-vae-ti2v
- Key components: Encoder3d, Decoder3d with causal convolutions
- Supports: Decode-only mode for inference

**CLIPTextModelRunner:**
- Purpose: Text encoding for conditioning
- Variants: clip-vit-l-14, clip-vit-h-14, clip-vit-bigg-14
- Output: Text embeddings [N, L, text_dim]

**T5Runner:**
- Purpose: Alternative text encoder
- Variants: t5-standard, t5-umt5
- Output: Text embeddings [N, L, text_dim]

## Entry Points

**Model Loading:**
- Location: `src/model_factory.cpp`
- Triggers: Application calls model factory with model name
- Responsibilities: Instantiate runner, load weights, initialize parameters

**Inference:**
- Location: `src/wan.hpp` (WanRunner::compute), `src/vae.hpp` (WanVAERunner::compute)
- Triggers: Application calls runner.compute() with input tensors
- Responsibilities: Build computation graph, execute on backend, return output

**API Layer:**
- Location: `src/api/wan-api.cpp`, `src/api/wan_t2v.cpp`, `src/api/wan_i2v.cpp`
- Triggers: High-level API calls
- Responsibilities: Orchestrate model pipeline, handle I/O

## Error Handling

**Strategy:** Assertion-based with GGML_ASSERT macros

**Patterns:**
- Dimension validation: GGML_ASSERT(b == 1) for batch size
- Type checking: GGML_ASSERT(decode_only == false) for encode operations
- Resource validation: GGML_ASSERT(work_ctx != nullptr) for context initialization
- Fallback: Abort with descriptive message on invalid configuration

## Cross-Cutting Concerns

**Logging:**
- Framework: LOG_INFO, LOG_DEBUG, LOG_ERROR macros
- Usage: Model loading, inference timing, error reporting

**Validation:**
- Tensor shape validation in forward() methods
- Parameter existence checks in init_params()
- Type compatibility checks in block initialization

**Authentication:**
- Not applicable (inference-only system)

**Performance Optimization:**
- OP-02: PE (Positional Encoding) caching to avoid redundant CPU computation
- FUS-02: Inplace GELU for operator fusion in FFN blocks
- CG-02: CUDA graph automatic kernel merging
- Offload strategy: Parameters can be offloaded to CPU, activated on demand

---

*Architecture analysis: 2026-03-28*
