# External Integrations

**Analysis Date:** 2026-03-17

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
- None - No caching layer implemented

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

---

*Integration audit: 2026-03-17*
