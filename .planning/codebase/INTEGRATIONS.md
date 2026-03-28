# External Integrations

**Analysis Date:** 2026-03-28

## Model Architecture & Types

**Four Core Model Types (via Model Registry):**

1. **CLIP Text Encoder** - `CLIPTextModelRunner`
   - Location: `src/clip.hpp` (line 908)
   - Registered variants: `clip-vit-l-14`, `clip-vit-h-14`, `clip-vit-bigg-14`
   - Purpose: Text encoding for prompt conditioning
   - Interface: `forward(ctx, input_ids, embeddings, mask, max_token_idx, return_pooled, clip_skip)`
   - Output: Text embeddings for diffusion guidance

2. **T5 Text Encoder** - `T5Runner`
   - Location: `src/t5.hpp` (line 756)
   - Registered variants: `t5-standard`, `t5-umt5`
   - Purpose: Advanced text encoding with relative position buckets
   - Interface: `forward(ctx, input_ids, relative_position_bucket, attention_mask)`
   - Output: Hidden states [N, n_token, model_dim]
   - Supports: Standard T5 and multilingual UMT5 tokenization

3. **WAN VAE (Video Autoencoder)** - `WAN::WanVAERunner`
   - Location: `src/wan.hpp` (line 1119)
   - Registered variants: `wan-vae-t2v`, `wan-vae-t2v-decode`, `wan-vae-i2v`, `wan-vae-ti2v`
   - Purpose: Video encoding/decoding for latent space representation
   - Modes: Full encode/decode or decode-only (for inference)
   - Supports: T2V, I2V, TI2V variants via `SDVersion` parameter
   - Interface: `compute(n_threads, z, decode_graph, output)`
   - Graph size: 10240 * z->ne[2] (adaptive based on input)

4. **WAN Transformer (DiT)** - `WAN::WanRunner`
   - Location: `src/wan.hpp` (line 2020)
   - Registered variants: `wan-runner-t2v`, `wan-runner-i2v`, `wan-runner-ti2v`
   - Purpose: Diffusion transformer for video generation
   - Model variants by layer count:
     - 30 layers: 1.3B models (dim=1536, ffn_dim=8960, 12 heads)
     - 40 layers: 14B models (dim=5120, ffn_dim=13824, 40 heads)
     - 30 layers TI2V: 5B model (dim=3072, ffn_dim=14336, 24 heads)
   - Supports: T2V, I2V, TI2V, VACE variants
   - Positional encoding caching: PE cached to avoid redundant CPU->GPU transfer

## Model Registry System

**Location:** `src/model_registry.hpp`, `src/model_factory.cpp`

**Registration Mechanism:**
- Template-based factory pattern with type erasure
- Macro: `REGISTER_MODEL_FACTORY(ModelType, VersionString, FactoryBody)`
- Thread-safe singleton registry with mutex protection
- Factory signature: `std::unique_ptr<ModelType>(ggml_backend_t, bool, String2TensorStorage, std::string)`

**Model Creation Flow:**
1. `ModelRegistry::instance()->create<ModelType>(version, backend, offload, tensor_map, prefix)`
2. Registry lookup by type name + version string
3. Factory lambda invoked with backend and tensor storage
4. Returns unique_ptr to initialized model runner

**Force-load Function:**
- `wan_force_model_registrations()` - Prevents linker dead-code elimination of model_factory.cpp

## APIs & External Services

**Model Loading:**
- GGUF Format - Model file format for quantized inference
  - Purpose: Load pre-trained WAN2.1/WAN2.2 video generation models
  - Implementation: `src/model.cpp`, `src/model.h`
  - No external API calls; local file-based loading

**No External APIs:**
- This is a standalone inference library with no external API integrations
- All model inference is performed locally
- No cloud service dependencies
- No remote model downloading (models must be provided locally)

## Data Storage

**Databases:**
- None - This is a pure inference library with no persistent storage

**File Storage:**
- Local filesystem only
  - Model files: GGUF format (binary model weights)
  - Input images: JPG, PNG (via stb_image)
  - Output videos: AVI format (via custom avi_writer)
  - Vocabulary files: Optional external directory or embedded in binary

**Caching:**
- Positional encoding cache in `WAN::WanRunner` (pe_cached, cached_pe_t/h/w)
- Feature map cache in `WAN::WanVAERunner` for partial graph computation
- No persistent caching layer

## Authentication & Identity

**Auth Provider:**
- None - No authentication required
- Standalone library with no user management or access control

## Monitoring & Observability

**Error Tracking:**
- None - No external error tracking service

**Logs:**
- Console-based logging via callback mechanism
  - Implementation: `wan_log_cb_t` callback in `include/wan-cpp/wan.h`
  - Log levels: DEBUG (0), INFO (1), WARN (2), ERROR (3)
  - User-provided callback: `wan_set_log_callback()`
  - Location: `src/api/wan-api.cpp` (global `g_log_callback`, `g_log_user_data`)

**Progress Tracking:**
- Progress callback mechanism for generation steps
  - Implementation: `wan_progress_cb_t` callback in `include/wan-cpp/wan.h`
  - Provides: current step, total steps, progress percentage (0.0-1.0)
  - User-provided callback: `wan_params_t.progress_cb`
  - Location: `src/api/wan-api.cpp`

## CI/CD & Deployment

**Hosting:**
- Not applicable - This is a library, not a hosted service
- Deployed as: Static library (.a) or shared library (.so/.dll)

**CI Pipeline:**
- GitHub Actions (CI configuration in `ggml/.github/`)
- No external CI service integration in wan-cpp itself

## Environment Configuration

**Required env vars:**
- None - Configuration is entirely CMake-based at build time
- Runtime configuration via C API parameters (`wan_params_t`)

**Secrets location:**
- Not applicable - No secrets or credentials required

## Webhooks & Callbacks

**Incoming:**
- None - Standalone library with no network endpoints

**Outgoing:**
- Progress callback: User-provided function pointer
  - Type: `wan_progress_cb_t`
  - Called during generation steps
  - Location: `include/wan-cpp/wan.h` (lines 67-75)

- Log callback: User-provided function pointer
  - Type: `wan_log_cb_t`
  - Called for logging events
  - Location: `include/wan-cpp/wan.h` (lines 95-101)

## Model Format & Loading

**GGUF Model Format:**
- Binary format for quantized neural network weights
- Supports multiple quantization levels (fp16, int8, int4, etc.)
- Loaded via GGML's GGUF reader
- Location: `src/model.cpp` - Model loading and parsing

**Supported Models:**
- WAN2.1 series - Text-to-video, image-to-video generation
- WAN2.2 series - Enhanced video generation models
- Model format: GGUF (`.gguf` files)
- No model downloading; must be provided by user

## Vocabulary & Tokenization

**Vocabulary Loading:**
- Two modes:
  1. Embedded: Compiled into binary (WAN_EMBED_VOCAB=ON)
  2. Runtime mmap: Loaded from external directory (WAN_EMBED_VOCAB=OFF, default)

- Location: `src/vocab/vocab.cpp`, `src/vocab/vocab.h`
- Supported tokenizers:
  - CLIP tokenizer (for text encoding)
  - T5 tokenizer (for text encoding)
  - Mistral tokenizer
  - Qwen tokenizer
  - UMT5 tokenizer

**Vocab Directory:**
- Required when WAN_EMBED_VOCAB=OFF (default)
- Specified via CLI: `--vocab-dir` parameter
- Contains tokenizer vocabulary files for model inference

## Tensor Storage & Parameter Loading

**String2TensorStorage:**
- Type: `std::map<std::string, TensorStorage>`
- Purpose: Maps tensor names to storage metadata (dimensions, type)
- Used by all model runners during initialization
- Location: `src/model.h`

**Parameter Initialization:**
- Each model runner calls `init(params_ctx, tensor_storage_map, prefix)` during construction
- Prefix parameter allows multiple model instances with different tensor namespaces
- Lazy loading: Tensors loaded on-demand from GGUF files

---

*Integration audit: 2026-03-28*
