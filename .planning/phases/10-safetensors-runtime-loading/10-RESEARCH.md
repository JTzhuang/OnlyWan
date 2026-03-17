# Phase 10: Safetensors Runtime Loading - Research

**Researched:** 2026-03-17
**Domain:** safetensors format parsing, GGML tensor allocation, C++ model loading integration
**Confidence:** HIGH

## Summary

Phase 10 adds direct `.safetensors` loading to `wan_load_model`. The safetensors format is a simple binary layout: 8-byte little-endian header size, followed by a JSON header describing each tensor (dtype, shape, byte offsets), followed by raw tensor data. The project already has a complete, battle-tested safetensors parser in `src/model.cpp` (`ModelLoader::init_from_safetensors_file` + `is_safetensors_file`). This parser handles all relevant dtypes (F16, BF16, F32, F8_E4M3, F8_E5M2, F64, I64), reverses dimension order to match GGML conventions, and populates `TensorStorage` entries with file offsets for lazy reading.

The critical gap is that `WanModel::load` in `wan-api.cpp` currently calls `is_wan_gguf()` to validate the file and extract `model_type`/`model_version` metadata — a check that hard-fails on safetensors input. The fix is a two-branch dispatch: detect the file format by extension or magic bytes, then either run the existing GGUF path or a new safetensors path. For safetensors, model type must be inferred from tensor names (the same heuristic already used in `ModelLoader::get_sd_version()` — look for `model.diffusion_model.blocks.0.cross_attn.norm_k.weight` to confirm WAN, then check `patch_embedding.weight` channel count and presence of `img_emb` to distinguish T2V / I2V / TI2V).

The tensor name mapping is the only non-trivial concern. HuggingFace WAN2.1/2.2 safetensors files use the same `model.diffusion_model.*` prefix that the GGUF files use after `convert_tensors_name()` is applied. The existing `init_from_file_and_convert_name` call in `WanModel::load` passes `"model.diffusion_model."` as prefix, which means safetensors tensors already named `model.diffusion_model.blocks.0.*` will match without any additional renaming. The `convert_tensors_name()` step handles any variant naming (e.g. diffusers-style) via `name_conversion.cpp`.

**Primary recommendation:** Add `is_safetensors_file()` branch in `WanModel::load` (in `wan-api.cpp`). Skip `is_wan_gguf()` check for safetensors; infer model type from tensor names via `ModelLoader::get_sd_version()`. All downstream tensor loading, runner construction, and generation code is unchanged.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| SAFE-01 | `wan_load_model` accepts `.safetensors` path, returns valid WanModel handle; safetensors-loaded model runs T2V generation equivalent to GGUF; invalid/corrupt file returns clear error code without crash | `ModelLoader::init_from_safetensors_file` fully implemented in `src/model.cpp`; `is_safetensors_file()` magic-byte check exists; `get_sd_version()` already detects WAN from tensor names; `WanModel::load` in `wan-api.cpp` is the single integration point |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `ModelLoader::init_from_safetensors_file` | in-tree | Parse safetensors header JSON, build TensorStorage map | Already implemented, tested, handles all dtypes including F8/F64/I64 conversions |
| `is_safetensors_file()` | in-tree | Detect safetensors by reading 8-byte header size + JSON validity | Already implemented in `src/model.cpp` |
| `ModelLoader::get_sd_version()` | in-tree | Infer WAN model type (T2V/I2V/TI2V) from tensor names | Already handles `VERSION_WAN2`, `VERSION_WAN2_2_I2V`, `VERSION_WAN2_2_TI2V` |
| `nlohmann::json` | in-tree (`src/json.hpp`) | Parse safetensors JSON header | Already used by `init_from_safetensors_file` |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `std::ifstream` | C++17 stdlib | Sequential read fallback for tensor data | Already used in `load_tensors` worker threads |
| `MmapWrapper` | in-tree | Memory-map safetensors file for zero-copy tensor reads | Already used in `load_tensors` when `enable_mmap=true` |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| In-tree `init_from_safetensors_file` | External safetensors-cpp library | External lib adds dependency; in-tree parser already handles all needed dtypes and integrates with TensorStorage/ModelLoader pipeline |
| Extension-based detection | Magic-byte detection only | Extension check is fast O(1) string op; magic-byte check (`is_safetensors_file`) is the authoritative fallback — use both |

**Installation:** No new dependencies. All required code is already in-tree.

## Architecture Patterns

### Recommended Project Structure

No new files needed. Changes are confined to:
```
src/api/wan-api.cpp     — WanModel::load: add safetensors branch
src/api/wan_loader.cpp  — add is_safetensors_wan() helper (optional, keeps wan-api.cpp clean)
```

### Pattern 1: Format-Dispatch in WanModel::load

**What:** Detect file format at the top of `WanModel::load`, branch to GGUF path or safetensors path. Both paths converge at the same runner construction code.

**When to use:** Single entry point `wan_load_model` must transparently handle both formats.

**Example:**
```cpp
// Source: src/api/wan-api.cpp — WanModel::load
WanModelLoadResult Wan::WanModel::load(const std::string& file_path) {
    WanModelLoadResult result;
    result.success = false;

    // Validate file exists
    { std::ifstream f(file_path); if (!f.good()) { result.error_message = "File not found: " + file_path; return result; } }

    std::string model_type, model_version;
    ModelLoader model_loader;

    bool is_st = is_safetensors_file(file_path);
    bool is_gguf_wan = !is_st && Wan::is_wan_gguf(file_path, model_type, model_version);

    if (!is_st && !is_gguf_wan) {
        result.error_message = "Not a valid Wan GGUF or safetensors file: " + file_path;
        return result;
    }

    if (is_st) {
        // safetensors path: load tensors, infer version from names
        if (!model_loader.init_from_file(file_path)) {
            result.error_message = "Failed to parse safetensors: " + file_path;
            return result;
        }
        SDVersion sv = model_loader.get_sd_version();
        if (!sd_version_is_wan(sv)) {
            result.error_message = "safetensors file does not contain a WAN model: " + file_path;
            return result;
        }
        // map SDVersion -> model_type string
        if (sv == VERSION_WAN2_2_I2V)  model_type = "i2v";
        else if (sv == VERSION_WAN2_2_TI2V) model_type = "ti2v";
        else model_type = "t2v";
        model_version = (model_type == "i2v" || model_type == "ti2v") ? "WAN2.2" : "WAN2.1";
    } else {
        // GGUF path: existing logic
        if (!model_loader.init_from_file_and_convert_name(file_path, "model.diffusion_model.")) { ... }
    }
    // ... runner construction unchanged ...
}
```

### Pattern 2: Safetensors Tensor Name Alignment

**What:** HuggingFace WAN safetensors files use `model.diffusion_model.*` prefixes natively. `init_from_file` with no prefix argument loads them as-is. `convert_tensors_name()` handles any variant naming.

**When to use:** Always call `convert_tensors_name()` after `init_from_file` for safetensors, same as GGUF path uses `init_from_file_and_convert_name`.

**Example:**
```cpp
// Safetensors tensors arrive as: "model.diffusion_model.blocks.0.cross_attn.norm_k.weight"
// get_param_tensors() expects:   "model.diffusion_model.blocks.0.cross_attn.norm_k.weight"
// No renaming needed for standard HF WAN2.1/2.2 checkpoints.
// For non-standard naming, convert_tensors_name() applies name_conversion.cpp rules.
model_loader.init_from_file(file_path);   // no prefix — names already canonical
model_loader.convert_tensors_name();      // normalize any variant names
```

### Pattern 3: WAN Version Detection from Tensor Names

**What:** `ModelLoader::get_sd_version()` already implements the full WAN detection heuristic. It checks for `model.diffusion_model.blocks.0.cross_attn.norm_k.weight` (confirms WAN), then reads `patch_embedding.weight` channel count and `img_emb` presence to distinguish subtypes.

**Detection logic (from `src/model.cpp:1075-1136`):**
```cpp
// is_wan = true when: name contains "model.diffusion_model.blocks.0.cross_attn.norm_k.weight"
// VERSION_WAN2_2_I2V  when: patch_embedding_channels == 184320 && !has_img_emb
// VERSION_WAN2_2_TI2V when: patch_embedding_channels == 147456 && !has_img_emb
// VERSION_WAN2        otherwise (T2V)
```

### Anti-Patterns to Avoid

- **Calling `is_wan_gguf()` on a safetensors file:** `gguf_init_from_file` will fail or return null — always check format first.
- **Adding a prefix to `init_from_file` for safetensors:** HF WAN safetensors already have `model.diffusion_model.` prefix in tensor names. Adding it again doubles the prefix and breaks all name lookups.
- **Skipping `convert_tensors_name()`:** Some community safetensors checkpoints use diffusers-style names. Always normalize.
- **Assuming safetensors = T2V:** Always run `get_sd_version()` to detect I2V/TI2V variants.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| safetensors header parsing | Custom JSON parser for header | `ModelLoader::init_from_safetensors_file` | Already handles 8-byte size prefix, JSON parse, all dtype conversions, dimension reversal, F8/F64/I64 special cases |
| File format detection | Extension-only check | `is_safetensors_file()` + extension check | Magic-byte check validates actual file content, not just name |
| WAN model type inference | Custom tensor name scan | `ModelLoader::get_sd_version()` | Already implements the exact heuristic for all three WAN variants |
| Tensor data reading | Custom file I/O | `ModelLoader::load_tensors()` | Handles mmap, multithreaded reads, type conversion, backend upload |

**Key insight:** The entire safetensors loading pipeline already exists in `src/model.cpp`. Phase 10 is purely a dispatch/integration task in `WanModel::load`.

## Common Pitfalls

### Pitfall 1: Dimension Order Reversal
**What goes wrong:** safetensors stores shapes in row-major (PyTorch) order; GGML uses column-major. A tensor stored as `[768, 3072]` in safetensors must become `[3072, 768]` in GGML `ne[]`.
**Why it happens:** PyTorch and GGML have opposite dimension conventions.
**How to avoid:** `init_from_safetensors_file` already calls `tensor_storage.reverse_ne()` on every tensor. Do not reverse again.
**Warning signs:** Shape mismatch errors in `load_tensors` callback comparing `real->ne[]` vs `tensor_storage.ne[]`.

### Pitfall 2: BF16 Dtype Handling
**What goes wrong:** WAN2.1/2.2 HuggingFace checkpoints are typically stored in BF16. `str_to_ggml_type("BF16")` returns `GGML_TYPE_BF16`. GGML supports BF16 natively since late 2024.
**Why it happens:** BF16 is the default HF training dtype for WAN models.
**How to avoid:** No special handling needed — `GGML_TYPE_BF16` is in the dtype map and `convert_tensor` handles BF16↔F32 conversion. Verify GGML version in the symlinked `ggml/` supports `GGML_TYPE_BF16`.
**Warning signs:** `str_to_ggml_type` returning `GGML_TYPE_COUNT` for BF16 tensors.

### Pitfall 3: Missing model_version for safetensors
**What goes wrong:** `is_wan_gguf()` reads `general.wan.version` from GGUF metadata. safetensors has no equivalent metadata field.
**Why it happens:** safetensors JSON header only contains tensor metadata, not model metadata.
**How to avoid:** Infer `model_version` from `model_type`: T2V → "WAN2.1", I2V/TI2V → "WAN2.2". This matches the fallback already in `is_wan_gguf()` (line 88-90 of `wan_loader.cpp`).
**Warning signs:** Empty `model_version` string causing issues in `wan_get_model_info`.

### Pitfall 4: ODR violation if is_safetensors_file called from wan_loader.cpp
**What goes wrong:** `is_safetensors_file()` is defined in `src/model.cpp` (not declared in `model.h`). Calling it from `wan_loader.cpp` requires a declaration.
**Why it happens:** `is_safetensors_file` is a file-scope function in `model.cpp` with no header declaration.
**How to avoid:** Either add `bool is_safetensors_file(const std::string&);` to `model.h`, or call it only from `wan-api.cpp` which already includes `model.h` and links `model.cpp`. The safest approach: keep all format detection in `WanModel::load` (wan-api.cpp) which already has full access.
**Warning signs:** Linker error "undefined reference to is_safetensors_file".

### Pitfall 5: Safetensors data offset calculation
**What goes wrong:** The tensor data region starts at `ST_HEADER_SIZE_LEN (8) + header_size_`. The `data_offsets` in the JSON are relative to the start of the data region, not the file start. `init_from_safetensors_file` already adds `ST_HEADER_SIZE_LEN + header_size_ + begin` as the absolute file offset in `TensorStorage::offset`.
**Why it happens:** safetensors spec defines `data_offsets` as relative to the data section start.
**How to avoid:** Use `init_from_safetensors_file` as-is — offset calculation is already correct. Do not re-add the header size.

## Code Examples

### Detecting safetensors by extension + magic bytes
```cpp
// Source: src/model.cpp — is_safetensors_file() (already exists)
// Fast path: check extension first, then validate with magic-byte check
static bool has_safetensors_extension(const std::string& path) {
    return path.size() >= 12 &&
           path.substr(path.size() - 12) == ".safetensors";
}

// In WanModel::load:
bool is_st = has_safetensors_extension(file_path) || is_safetensors_file(file_path);
```

### Inferring WAN model type from SDVersion
```cpp
// Source: src/model.cpp — get_sd_version() returns VERSION_WAN2 / VERSION_WAN2_2_I2V / VERSION_WAN2_2_TI2V
SDVersion sv = model_loader.get_sd_version();
if (!sd_version_is_wan(sv)) {
    result.error_message = "safetensors does not contain a WAN model: " + file_path;
    return result;
}
std::string model_type;
if (sv == VERSION_WAN2_2_I2V)       model_type = "i2v";
else if (sv == VERSION_WAN2_2_TI2V) model_type = "ti2v";
else                                 model_type = "t2v";
std::string model_version = (model_type != "t2v") ? "WAN2.2" : "WAN2.1";
```

### Full safetensors branch in WanModel::load
```cpp
// Source: integration point in src/api/wan-api.cpp
ModelLoader model_loader;

if (is_st) {
    // safetensors: init without prefix (names already canonical), then normalize
    if (!model_loader.init_from_file(file_path)) {
        result.error_message = "Failed to parse safetensors header: " + file_path;
        return result;
    }
    model_loader.convert_tensors_name();

    SDVersion sv = model_loader.get_sd_version();
    if (!sd_version_is_wan(sv)) {
        result.error_message = "Not a WAN safetensors model: " + file_path;
        return result;
    }
    // ... set model_type, model_version, sd_version from sv ...
} else {
    // GGUF: existing path unchanged
    if (!Wan::is_wan_gguf(file_path, model_type, model_version)) { ... }
    if (!model_loader.init_from_file_and_convert_name(file_path, "model.diffusion_model.")) { ... }
}
// Runner construction is identical for both paths from here
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| GGUF-only loading | GGUF + safetensors dispatch | Phase 10 | Users can load HF checkpoints directly without conversion |
| `is_wan_gguf()` as sole validator | Format detection before validation | Phase 10 | Prevents null-deref crash on safetensors input |

**Deprecated/outdated:**
- None — GGUF path is preserved unchanged; safetensors is additive.

## Open Questions

1. **Multi-file safetensors (sharded checkpoints)**
   - What we know: Large WAN models (e.g. 14B) may be split across `model-00001-of-00003.safetensors` etc.
   - What's unclear: Whether Phase 10 scope includes sharded loading or single-file only.
   - Recommendation: Scope to single-file only for Phase 10. `ModelLoader` supports multiple files via repeated `init_from_file` calls — sharded support can be Phase 11 or later.

2. **BF16 GGML support version**
   - What we know: `GGML_TYPE_BF16` is in `str_to_ggml_type` map in `model.cpp`. The symlinked `ggml/` is from the parent `stable-diffusion.cpp` repo.
   - What's unclear: Exact GGML commit version and whether BF16 backend ops are fully supported for CPU inference.
   - Recommendation: Verify `GGML_TYPE_BF16` is defined in the linked ggml headers before assuming BF16 tensors load correctly. If not, add F32 conversion fallback.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None detected — no test config files or test directories found |
| Config file | none — Wave 0 gap |
| Quick run command | manual build + CLI smoke test |
| Full suite command | manual build + CLI smoke test |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SAFE-01a | `wan_load_model` accepts `.safetensors` path, returns non-null handle | smoke | `./bin/wan-cli --model test.safetensors --prompt "test" --output /tmp/out.avi` | ❌ Wave 0 |
| SAFE-01b | Invalid/corrupt safetensors returns `WAN_ERROR_MODEL_LOAD_FAILED`, no crash | smoke | manual test with truncated file | ❌ Wave 0 |
| SAFE-01c | safetensors-loaded model T2V output equivalent to GGUF-loaded model | integration | manual visual comparison | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** build with `cmake --build . && ./bin/wan-cli --help`
- **Per wave merge:** full smoke test with actual safetensors file
- **Phase gate:** SAFE-01a and SAFE-01b pass before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] No automated test infrastructure exists — manual CLI smoke tests are the validation path for this phase
- [ ] `is_safetensors_file` needs declaration in `model.h` if called outside `model.cpp` / `wan-api.cpp`

## Sources

### Primary (HIGH confidence)
- `src/model.cpp` — `init_from_safetensors_file`, `is_safetensors_file`, `get_sd_version`, `load_tensors` — read in full
- `src/api/wan-api.cpp` — `WanModel::load`, `wan_load_model` — read in full
- `src/api/wan_loader.cpp` — `is_wan_gguf`, `WanBackend::create` — read in full
- `include/wan-cpp/wan-internal.hpp` — `WanModelLoadResult`, `wan_context` — read in full
- `src/model.h` — `ModelLoader`, `TensorStorage`, `SDVersion` — read in full
- `src/wan.hpp` — WAN block structure, tensor name patterns — grep verified
- `CMakeLists.txt` — build system, source list — read in full

### Secondary (MEDIUM confidence)
- HuggingFace safetensors format spec (https://huggingface.co/docs/safetensors/index) — referenced in `model.cpp` comment at line 500; format confirmed by reading the parser implementation

### Tertiary (LOW confidence)
- WAN2.1/2.2 HuggingFace checkpoint tensor naming — inferred from `get_sd_version()` heuristics and `wan.hpp` block names; not verified against an actual safetensors file

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all code is in-tree and read directly
- Architecture: HIGH — integration point is clear, pattern is established by existing GGUF path
- Pitfalls: HIGH — dimension reversal, BF16, offset calculation all verified from source
- Tensor name mapping: MEDIUM — inferred from heuristics; actual HF checkpoint not available to verify

**Research date:** 2026-03-17
**Valid until:** 2026-04-17 (stable domain — safetensors format is frozen spec)
