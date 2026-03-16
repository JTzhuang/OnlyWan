# Phase 7: Wire Core Model to API - Research

**Researched:** 2026-03-16
**Domain:** C++ GGML model wiring, WAN DiT inference, encoder integration
**Confidence:** HIGH

## Summary

Phase 7 connects the already-extracted `wan.hpp` core (WanRunner, WanVAERunner) to the API layer in `wan_loader.cpp`, `wan_t2v.cpp`, and `wan_i2v.cpp`. The v1.0 audit confirmed that `wan.hpp` is compiled but never instantiated by any API file — the model weights are never loaded, and encoder token outputs are discarded. This phase closes CORE-01, CORE-02, CORE-04, API-02, ENCODER-01, and ENCODER-02.

The wiring pattern is well-established inside `wan.hpp` itself: `WanRunner::load_from_file_and_test` and `WanVAERunner::load_from_file_and_test` show the exact sequence — `ModelLoader::init_from_file_and_convert_name` then `alloc_params_buffer` then `get_param_tensors` then `ModelLoader::load_tensors`. The API layer must replicate this pattern, storing `WanRunner` and `WanVAERunner` instances inside `wan_context`.

For encoders: `T5Embedder` (in `t5.hpp`) wraps `T5Runner` + `T5UniGramTokenizer` and produces hidden states via `compute()`. `CLIPVisionModelProjection` (in `clip.hpp`) takes pixel_values and produces `clip_fea [N, 257, 1280]`. Both outputs feed directly into `WanRunner::compute()` as `context` and `clip_fea` arguments.

**Primary recommendation:** Add `WanRunner` and `WanVAERunner` (and `T5Embedder`) as members of `wan_context`, load weights in `wan_loader.cpp` using `ModelLoader`, and pass encoder outputs through to `WanRunner::compute()` in `wan_t2v.cpp` and `wan_i2v.cpp`.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CORE-01 | Instantiate WAN model from wan.hpp in API layer | WanRunner constructor + ModelLoader pattern in wan.hpp:2250-2289 |
| CORE-02 | Use preprocessing.hpp in I2V pipeline | preprocess_canny() and sd_image_to_ggml_tensor() in preprocessing.hpp + ggml_extend.hpp |
| CORE-04 | Call model.h/cpp functions from wan_loader.cpp | ModelLoader::init_from_file_and_convert_name + load_tensors in model.h:313-329 |
| API-02 | Load model weights from GGUF (not just metadata) | ModelLoader replaces bare gguf_init_from_file; alloc_params_buffer + load_tensors |
| ENCODER-01 | Pass T5 token output to WAN model inference | T5Embedder::compute() produces context tensor passed to WanRunner::compute(context=...) |
| ENCODER-02 | Pass CLIP token output to WAN model inference | CLIPVisionModelProjection::forward() produces clip_fea passed to WanRunner::compute(clip_fea=...) |
</phase_requirements>

## Standard Stack

### Core
| Component | Location | Purpose | Why Standard |
|-----------|----------|---------|--------------|
| WanRunner | src/wan.hpp:2009 | DiT model forward pass + weight loading | The only WAN inference implementation |
| WanVAERunner | src/wan.hpp:1111 | VAE encode/decode for latent space | The only VAE implementation |
| T5Embedder | src/t5.hpp:903 | T5 tokenizer + runner combined | Wraps T5Runner + T5UniGramTokenizer together |
| CLIPVisionModelProjection | src/clip.hpp:868 | CLIP image encoder with projection | Produces clip_fea expected by WanRunner |
| ModelLoader | src/model.h:292 | GGUF/safetensors weight loading | Used by all runners in wan.hpp |

### Supporting
| Component | Location | Purpose | When to Use |
|-----------|----------|---------|-------------|
| sd_image_to_ggml_tensor | src/ggml_extend.hpp:432 | Convert sd_image_t to ggml_tensor | I2V preprocessing before CLIP |
| preprocessing.hpp | src/preprocessing.hpp | Canny edge detection, image ops | I2V image preprocessing pipeline |
| ggml_backend_cpu_init | ggml | CPU backend init | Default backend in wan_loader.cpp |

**Installation:** No new packages needed. All components already present in src/.

## Architecture Patterns

### Recommended Project Structure
No structural changes needed. All wiring happens within existing files:
```
src/api/
  wan_loader.cpp   # Add WanRunner + T5Embedder instantiation, ModelLoader weight loading
  wan_t2v.cpp      # Add T5Embedder::compute() call, pass context to WanRunner::compute()
  wan_i2v.cpp      # Add CLIP pixel_values prep, CLIPVisionModelProjection, pass clip_fea
include/wan-cpp/
  wan-internal.hpp # Add WanRunner + T5Embedder + CLIPVisionModelProjection members to wan_context
```

### Pattern 1: ModelLoader Weight Loading
**What:** The canonical pattern for loading WAN model weights from GGUF
**When to use:** In wan_loader.cpp WanModel::load(), replacing bare gguf_init_from_file
**Source:** src/wan.hpp:2256-2279 (WanRunner::load_from_file_and_test)

```cpp
ModelLoader model_loader;
if (!model_loader.init_from_file_and_convert_name(file_path, "model.diffusion_model.")) {
    return WAN_ERROR_MODEL_LOAD_FAILED;
}

auto& tensor_storage_map = model_loader.get_tensor_storage_map();

std::shared_ptr<WAN::WanRunner> wan_runner = std::make_shared<WAN::WanRunner>(
    backend,
    false,                       // offload_params_to_cpu
    tensor_storage_map,
    "model.diffusion_model",
    VERSION_WAN2                 // or VERSION_WAN2_2_I2V
);

wan_runner->alloc_params_buffer();
std::map<std::string, ggml_tensor*> tensors;
wan_runner->get_param_tensors(tensors, "model.diffusion_model");
bool ok = model_loader.load_tensors(tensors);
```

### Pattern 2: T5Embedder Inference
**What:** Tokenize prompt and run T5 encoder to get context tensor
**When to use:** In wan_t2v.cpp before calling WanRunner::compute()
**Source:** src/t5.hpp:903-962 (T5Embedder)

```cpp
auto [tokens, weights, attention_mask] = t5_embedder->tokenize(prompt, 512, true);

// Build input_ids tensor (GGML_TYPE_I32) from tokens
// Build attention_mask tensor (GGML_TYPE_F32) from attention_mask

ggml_tensor* context = nullptr;
t5_embedder->model.compute(n_threads, input_ids_tensor, attn_mask_tensor, &context, output_ctx);
// context shape: [1, 512, 4096] for UMT5-XXL
// Pass as context arg to WanRunner::compute()
```

### Pattern 3: WanRunner Inference
**What:** Run one denoising step of the WAN DiT
**When to use:** Inside the sampling loop in wan_t2v.cpp / wan_i2v.cpp
**Source:** src/wan.hpp:2192-2207 (WanRunner::compute)

```cpp
ggml_tensor* output = nullptr;
wan_runner->compute(
    n_threads,
    x,           // noisy latent [N*C, T, H, W]
    timesteps,   // [N,]
    context,     // T5 hidden states [N, seq_len, 4096]
    clip_fea,    // CLIP features [N, 257, 1280] -- nullptr for T2V
    nullptr,     // c_concat
    nullptr,     // time_dim_concat
    nullptr,     // vace_context
    1.f,         // vace_strength
    &output,
    output_ctx
);
```

### Pattern 4: wan_context Extension
**What:** Add runner members to wan_context so they are accessible from all API files
**When to use:** Move wan_context full definition from wan-api.cpp into wan-internal.hpp

```cpp
// In wan-internal.hpp -- extend wan_context struct:
struct wan_context {
    std::string last_error;
    std::string model_path;
    std::shared_ptr<WAN::WanRunner>    wan_runner;   // replaces WanModelPtr model
    std::shared_ptr<WAN::WanVAERunner> vae_runner;   // replaces WanVAEPtr vae
    std::shared_ptr<T5Embedder>        t5_embedder;  // for T2V
    WanBackendPtr backend;
    WanParams params;
    int n_threads;
    std::string backend_type;
    std::string last_error;
};
```

### Pattern 5: SDVersion Detection
**What:** Map GGUF model_type string to SDVersion enum for WanRunner constructor
**When to use:** In wan_loader.cpp after is_wan_gguf() returns model_type
**Source:** src/model.h:112-117

```cpp
SDVersion sd_version = VERSION_WAN2;
if (model_type == "i2v")   sd_version = VERSION_WAN2_2_I2V;
if (model_type == "ti2v")  sd_version = VERSION_WAN2_2_TI2V;
```

### Pattern 6: wan_image_t to sd_image_t Conversion
**What:** Bridge public API image type to internal preprocessing type
**When to use:** In wan_i2v.cpp before calling preprocessing.hpp functions
**Source:** src/wan-types.h:174, src/preprocessing.hpp

```cpp
// wan_image_t and sd_image_t have compatible layouts
sd_image_t sd_img;
sd_img.width   = wan_img->width;
sd_img.height  = wan_img->height;
sd_img.channel = wan_img->channels;
sd_img.data    = wan_img->data;
// Now safe to pass to sd_image_to_ggml_tensor() or preprocess_canny()
```

### Anti-Patterns to Avoid
- **Bare gguf_init_from_file for weight loading:** Current wan_loader.cpp only reads metadata. ModelLoader must be used to actually load tensor weights.
- **Global encoder instances:** wan_t2v.cpp and wan_i2v.cpp currently use file-scope globals. Move encoder ownership into wan_context.
- **Constructing WanRunner without tensor_storage_map:** WanRunner infers model architecture from tensor names at construction time. Passing empty map causes GGML_ABORT.
- **Calling WanRunner::compute() without alloc_params_buffer():** Weights must be allocated and loaded before compute.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Weight loading from GGUF | Custom tensor parsing | ModelLoader::init_from_file_and_convert_name + load_tensors | Handles name conversion, quantization, zip/safetensors, chunking |
| Model architecture detection | Parse tensor names manually | WanRunner constructor with tensor_storage_map | Auto-detects num_layers, dim, model_type from tensor names |
| T5 tokenization | Custom BPE tokenizer | T5UniGramTokenizer (already in t5.hpp) | Unigram LM tokenizer with full vocab already embedded |
| CLIP image preprocessing | Custom resize/normalize | sd_image_to_ggml_tensor + CLIPVisionEmbeddings | Handles patch embedding, position encoding |
| RoPE position encoding | Custom PE generation | Rope::gen_wan_pe (called inside WanRunner::build_graph) | WAN-specific 3D RoPE already implemented |

**Key insight:** All the hard inference machinery is already in wan.hpp. Phase 7 is purely wiring -- connecting existing components through the API layer, not implementing new algorithms.

## Common Pitfalls

### Pitfall 1: WanRunner Constructor Requires Non-Empty tensor_storage_map
**What goes wrong:** Constructing WanRunner with empty tensor_storage_map causes `GGML_ABORT("invalid num_layers(0) of wan")` at runtime.
**Why it happens:** WanRunner infers num_layers by scanning tensor names in the map at construction time (wan.hpp:2023-2058). With empty map, num_layers stays 0.
**How to avoid:** Always call `ModelLoader::init_from_file_and_convert_name` first, then pass `model_loader.get_tensor_storage_map()` to WanRunner constructor.
**Warning signs:** Crash/abort immediately on WanRunner construction.

### Pitfall 2: GGUF Prefix Mismatch
**What goes wrong:** `get_param_tensors` returns tensors but `load_tensors` loads 0 weights -- model runs with uninitialized weights.
**Why it happens:** The prefix passed to `init_from_file_and_convert_name` and `get_param_tensors` must match the tensor name prefix in the GGUF file. WAN GGUF files use `"model.diffusion_model."` for the DiT and `"first_stage_model"` for VAE (see wan.hpp:2257, 2270).
**How to avoid:** Use `"model.diffusion_model."` for WanRunner and `"first_stage_model"` for WanVAERunner, matching the load_from_file_and_test examples in wan.hpp.
**Warning signs:** `load_tensors` returns true but model output is garbage.

### Pitfall 3: sd_image_t vs wan_image_t Mismatch
**What goes wrong:** preprocessing.hpp uses `sd_image_t` (from wan-types.h), but the public API uses `wan_image_t`. Passing wan_image_t directly to preprocessing functions causes type errors.
**Why it happens:** preprocessing.hpp was extracted from stable-diffusion.cpp and uses its own image type.
**How to avoid:** In wan_i2v.cpp, convert `wan_image_t*` to `sd_image_t` before calling preprocessing functions. Both have the same layout.
**Warning signs:** Compile error in wan_i2v.cpp when including preprocessing.hpp.

### Pitfall 4: T5Embedder Needs Correct GGUF Prefix
**What goes wrong:** T5 encoder weights are stored under a different prefix in the GGUF file than the DiT weights. Using the wrong prefix loads 0 T5 weights.
**Why it happens:** WAN GGUF files may store T5 weights under `"cond_stage_model."` or `"text_encoders.t5xxl."` prefix, separate from `"model.diffusion_model."`.
**How to avoid:** Scan actual GGUF tensor names with `model_loader.get_tensor_names()` to detect the T5 prefix dynamically before constructing T5Embedder.
**Warning signs:** T5Embedder loads but produces zero/garbage context tensors.

### Pitfall 5: wan_context Struct Visibility
**What goes wrong:** `wan_context` is defined in `wan-api.cpp` as a plain struct, but `wan_t2v.cpp` and `wan_i2v.cpp` only see the forward declaration from `wan.h`. Adding WanRunner members requires the full definition to be visible.
**Why it happens:** The internal struct definition is local to wan-api.cpp, not in wan-internal.hpp.
**How to avoid:** Move the full `wan_context` struct definition into `wan-internal.hpp` so all API translation units can access its members.
**Warning signs:** Compile error "incomplete type" when accessing ctx->wan_runner in wan_t2v.cpp.

## Code Examples

### Full WanRunner Load Sequence
```cpp
// Source: src/wan.hpp:2250-2289 (WanRunner::load_from_file_and_test)
ggml_backend_t backend = ggml_backend_cpu_init();

ModelLoader model_loader;
if (!model_loader.init_from_file_and_convert_name(file_path, "model.diffusion_model.")) {
    return WAN_ERROR_MODEL_LOAD_FAILED;
}

auto& tensor_storage_map = model_loader.get_tensor_storage_map();
for (auto& [name, ts] : tensor_storage_map) {
    if (ends_with(name, "weight")) {
        ts.expected_type = GGML_TYPE_F16;
    }
}

auto wan_runner = std::make_shared<WAN::WanRunner>(
    backend, false, tensor_storage_map, "model.diffusion_model", VERSION_WAN2);

wan_runner->alloc_params_buffer();
std::map<std::string, ggml_tensor*> tensors;
wan_runner->get_param_tensors(tensors, "model.diffusion_model");
bool ok = model_loader.load_tensors(tensors);
```

### WanVAERunner Load Sequence
```cpp
// Source: src/wan.hpp:1260-1288 (WanVAERunner::load_from_file_and_test)
auto vae_runner = std::make_shared<WAN::WanVAERunner>(
    backend, false, tensor_storage_map, "", false, VERSION_WAN2);

vae_runner->alloc_params_buffer();
std::map<std::string, ggml_tensor*> vae_tensors;
vae_runner->get_param_tensors(vae_tensors, "first_stage_model");
bool ok = model_loader.load_tensors(vae_tensors);
```

### T5 Tokenize and Encode
```cpp
// Source: src/t5.hpp:923-961 (T5Embedder::tokenize)
auto [tokens, weights, attention_mask] = t5_embedder->tokenize(prompt, 512, true);

auto input_ids_tensor = ggml_new_tensor_2d(work_ctx, GGML_TYPE_I32, tokens.size(), 1);
memcpy(input_ids_tensor->data, tokens.data(), tokens.size() * sizeof(int32_t));

auto attn_mask_tensor = ggml_new_tensor_2d(work_ctx, GGML_TYPE_F32, attention_mask.size(), 1);
memcpy(attn_mask_tensor->data, attention_mask.data(), attention_mask.size() * sizeof(float));

ggml_tensor* context = nullptr;
t5_embedder->model.compute(n_threads, input_ids_tensor, attn_mask_tensor, &context, output_ctx);
// context: [1, 512, 4096] -- pass to WanRunner::compute(context=context)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Bare gguf_init_from_file (metadata only) | ModelLoader::init_from_file_and_convert_name (weights) | Phase 7 | Actual model weights loaded |
| Global T5UniGramTokenizer* in wan_t2v.cpp | T5Embedder owned by wan_context | Phase 7 | Multi-context safe, proper lifecycle |
| encode_image returns empty vector | CLIPVisionModelProjection::forward() | Phase 7 | Real CLIP features for I2V |
| WAN_ERROR_UNSUPPORTED_OPERATION stubs | WanRunner::compute() called | Phase 7 | Inference actually runs |

**Deprecated/outdated:**
- `Wan::WanModel` (in wan-internal.hpp): Custom internal struct wrapping bare gguf_ctx. Replace with `WAN::WanRunner` from wan.hpp.
- `Wan::WanVAE` (in wan-internal.hpp): Declared but never implemented. Replace with `WAN::WanVAERunner` from wan.hpp.
- Global `T5UniGramTokenizer* t5_tokenizer` in wan_t2v.cpp: Move into wan_context as T5Embedder.
- Global `CLIPTokenizer* clip_tokenizer` in wan_i2v.cpp: Replace with CLIPVisionModelProjection owned by wan_context.

## Open Questions

1. **GGUF tensor prefix for T5/CLIP in WAN model files**
   - What we know: WanRunner uses `"model.diffusion_model."`, WanVAERunner uses `"first_stage_model"` (from wan.hpp test functions)
   - What is unclear: The exact prefix for T5 and CLIP weights in real WAN GGUF files
   - Recommendation: At load time, call `model_loader.get_tensor_names()` and scan for known T5/CLIP tensor patterns to detect the prefix dynamically

2. **Single GGUF vs separate model files**
   - What we know: ModelLoader supports multiple file paths; WAN models may ship DiT + VAE + encoders in one GGUF or separately
   - What is unclear: Whether real WAN GGUF files bundle all components or require separate files
   - Recommendation: Support both -- try single file first, fall back to separate file paths if T5/VAE tensors not found

3. **wan_context struct visibility**
   - What we know: `wan_context` is defined in wan-api.cpp, forward-declared in wan.h
   - What is unclear: Whether moving the definition to wan-internal.hpp will cause ODR issues
   - Recommendation: Move full struct definition to wan-internal.hpp; all API .cpp files already include wan-internal.hpp

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None detected -- Wave 0 must establish |
| Config file | none -- see Wave 0 |
| Quick run command | `cmake --build build 2>&1 \| tail -5` |
| Full suite command | Build + link check (zero undefined references) |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CORE-01 | WanRunner instantiated in wan_loader.cpp | build | `cmake --build build 2>&1 \| grep -c error` | Wave 0 |
| CORE-02 | preprocessing.hpp included in wan_i2v.cpp | build | same | Wave 0 |
| CORE-04 | ModelLoader called from wan_loader.cpp | build | same | Wave 0 |
| API-02 | wan_load_model loads weights not just metadata | smoke | build + manual inspection | Wave 0 |
| ENCODER-01 | T5 context tensor passed to WanRunner | build | build + grep for compute call | Wave 0 |
| ENCODER-02 | CLIP clip_fea tensor passed to WanRunner | build | build + grep for compute call | Wave 0 |

### Sampling Rate
- **Per task commit:** `cmake --build /home/jtzhuang/projects/stable-diffusion.cpp/wan/build 2>&1 | tail -10`
- **Per wave merge:** Full build + link check
- **Phase gate:** Zero linker errors, zero undefined references before /gsd:verify-work

### Wave 0 Gaps
- [ ] No test framework present -- build verification is the primary gate
- [ ] `cmake -B /home/jtzhuang/projects/stable-diffusion.cpp/wan/build` -- configure build if not present

## Sources

### Primary (HIGH confidence)
- src/wan.hpp:2009-2290 -- WanRunner struct, load_from_file_and_test pattern, compute() signature
- src/wan.hpp:1111-1289 -- WanVAERunner struct, load_from_file_and_test pattern
- src/t5.hpp:756-962 -- T5Runner, T5Embedder, tokenize(), compute() signature
- src/clip.hpp:868-906 -- CLIPVisionModelProjection, forward() signature
- src/model.h:292-343 -- ModelLoader class, init_from_file_and_convert_name, load_tensors
- src/api/wan_loader.cpp -- Current state: metadata-only load, no WanRunner instantiation
- src/api/wan_t2v.cpp -- Current state: T5 tokenizes but tokens discarded
- src/api/wan_i2v.cpp -- Current state: CLIP tokenizer present, encode_image returns empty
- include/wan-cpp/wan-internal.hpp -- Current wan_context definition, WanModel/WanVAE stubs
- .planning/v1.0-MILESTONE-AUDIT.md -- Gap analysis confirming all 6 requirements are partial

### Secondary (MEDIUM confidence)
- src/preprocessing.hpp -- preprocess_canny, sd_image_to_ggml_tensor usage pattern
- src/wan-types.h:174 -- sd_image_t layout (compatible with wan_image_t)
- src/ggml_extend.hpp:386-432 -- ggml_tensor_to_sd_image, sd_image_to_ggml_tensor

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all components verified by direct source inspection
- Architecture: HIGH -- wiring pattern extracted verbatim from wan.hpp test functions
- Pitfalls: HIGH -- derived from direct code analysis of current stubs and type mismatches

**Research date:** 2026-03-16
**Valid until:** 2026-04-16 (stable codebase, no external dependencies changing)
