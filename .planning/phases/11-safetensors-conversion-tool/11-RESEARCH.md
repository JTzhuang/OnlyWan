# Phase 11: Safetensors Conversion Tool - Research

**Researched:** 2026-03-17
**Domain:** C++ CLI tool, ModelLoader::save_to_gguf_file, GGUF metadata writing
**Confidence:** HIGH

## Summary

Phase 11 adds a standalone `wan-convert` CLI executable that converts WAN2.1/2.2 safetensors
sub-model files (DiT, VAE, T5, CLIP) into GGUF files loadable by `wan_load_model`. The
conversion pipeline already exists in full inside `src/model.cpp` — `ModelLoader::init_from_file`
parses safetensors, `convert_tensors_name` normalises tensor names, and `save_to_gguf_file`
quantises and writes GGUF. The only missing pieces are: (1) a thin CLI `main.cpp` that wires
these calls together, (2) GGUF metadata injection (`general.model_type`, `general.architecture`,
`general.wan.version`) so `is_wan_gguf()` can validate the output, and (3) CMake wiring for the
new `wan-convert` executable under `examples/convert/`.

The parent project (`stable-diffusion.cpp`) already ships a `convert` mode in its CLI that calls
`save_to_gguf_file` with a wtype argument — this is the exact pattern to replicate.

**Primary recommendation:** Add `examples/convert/main.cpp` + `examples/convert/CMakeLists.txt`,
wire into `examples/CMakeLists.txt`. The converter calls `ModelLoader::init_from_file` →
`convert_tensors_name` → inject GGUF metadata via `gguf_set_val_str` → `save_to_gguf_file`.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| SAFE-02 | 提供 safetensors → GGUF 转换工具（独立 CLI 可执行文件） | `wan-convert` executable built via `examples/convert/CMakeLists.txt`; links against `wan-cpp` which exposes `ModelLoader` via `src/model.h` |
| SAFE-03 | 转换工具支持 WAN2.1/2.2 所有子模型（DiT、VAE、T5、CLIP） | `ModelLoader::init_from_file` accepts any safetensors file regardless of sub-model type; `--type` flag selects sub-model for metadata injection; `save_to_gguf_file` handles all tensor shapes |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| ModelLoader (src/model.h) | project-local | safetensors parse + GGUF write | Already implements full pipeline; `init_from_file`, `convert_tensors_name`, `save_to_gguf_file` all present |
| gguf.h (ggml) | project ggml | GGUF metadata write API | `gguf_set_val_str` writes `general.*` keys read by `is_wan_gguf()` |
| ggml-cpu (ggml) | project ggml | CPU backend for quantisation | `save_to_gguf_file` calls `ggml_backend_cpu_init()` internally |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `<cstdio>` / `<string>` | C++17 stdlib | CLI arg parsing, path handling | Always — no external arg-parse library needed for simple flags |
| `src/util.h` | project-local | `LOG_INFO`, `LOG_ERROR` macros | Consistent logging output matching rest of codebase |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Hand-rolled arg parsing | CLI11 / argparse | Overkill — wan-cli already uses hand-rolled parsing; stay consistent |
| Calling `wan_load_model` + re-save | Direct ModelLoader | `wan_load_model` constructs runners (expensive); ModelLoader is the right level |

**Installation:** No new dependencies — `wan-convert` links against `wan-cpp` which already pulls in ggml and all required headers.

## Architecture Patterns

### Recommended Project Structure
```
examples/
├── cli/             # existing wan-cli
│   ├── main.cpp
│   ├── avi_writer.c
│   └── CMakeLists.txt
└── convert/         # new wan-convert
    ├── main.cpp     # CLI entry point (~150 lines)
    └── CMakeLists.txt
```

`examples/CMakeLists.txt` gains one line: `add_subdirectory(convert)`.

### Pattern 1: ModelLoader conversion pipeline
**What:** Load safetensors → normalise names → inject metadata → write GGUF
**When to use:** Every sub-model conversion (DiT, VAE, T5, CLIP)
**Example:**
```cpp
// Source: src/model.cpp ModelLoader::save_to_gguf_file + wan_loader.cpp is_wan_gguf
ModelLoader loader;
if (!loader.init_from_file(input_path)) {
    fprintf(stderr, "error: failed to parse %s\n", input_path.c_str());
    return 1;
}
loader.convert_tensors_name();

// Inject GGUF metadata so is_wan_gguf() validates the output
// Must happen BEFORE save_to_gguf_file — but save_to_gguf_file creates its own
// gguf_context internally. We need a wrapper or post-process step.
// See "GGUF metadata gap" in Common Pitfalls below.

loader.save_to_gguf_file(output_path, quant_type, tensor_type_rules);
```

### Pattern 2: Sub-model type → metadata mapping
**What:** Map `--type` CLI flag to the three metadata keys `is_wan_gguf()` reads
**When to use:** Always — without these keys the output GGUF fails `is_wan_gguf()` validation
```cpp
// Keys read by is_wan_gguf() in src/api/wan_loader.cpp:
//   "general.model_type"    → "wan" or "WAN"
//   "general.architecture"  → contains "T2V" / "I2V" / "TI2V"
//   "general.wan.version"   → "WAN2.1" or "WAN2.2"
struct SubModelMeta {
    const char* arch;     // e.g. "WAN-T2V", "WAN-I2V", "WAN-TI2V"
    const char* version;  // "WAN2.1" or "WAN2.2"
};
```

### Pattern 3: CMake executable wiring (matches wan-cli pattern exactly)
```cmake
# examples/convert/CMakeLists.txt
add_executable(wan-convert main.cpp)
target_link_libraries(wan-convert PRIVATE wan-cpp)
target_include_directories(wan-convert PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/wan-cpp
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/thirdparty
)
target_link_libraries(wan-convert PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)
install(TARGETS wan-convert RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
```

### Anti-Patterns to Avoid
- **Calling `wan_load_model` then re-saving:** Constructs full runner graph (expensive, unnecessary). Use `ModelLoader` directly.
- **Skipping `convert_tensors_name()`:** Community checkpoints may use diffusers-style names; skipping breaks tensor name normalisation before GGUF write.
- **Passing prefix to `init_from_file` for HF checkpoints:** HF WAN safetensors already have `model.diffusion_model.*` names — doubling the prefix breaks all lookups (established in Phase 10).
- **Assuming `save_to_gguf_file` writes metadata:** It does NOT call `gguf_set_val_str` for any `general.*` keys — the output GGUF has tensors only. Metadata must be injected separately (see pitfall below).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| safetensors parsing | Custom parser | `ModelLoader::init_from_file` | Already handles header JSON, dtype mapping, mmap, reverse_ne |
| GGUF binary writing | Custom writer | `ModelLoader::save_to_gguf_file` | Handles quantisation, tensor_type_rules, gguf_write_to_file |
| Tensor name normalisation | Custom regex | `ModelLoader::convert_tensors_name` | Already handles all WAN diffusers-style variants |
| Quantisation logic | Custom cast | `save_to_gguf_file` + `tensor_should_be_converted` | Handles block-size alignment, bias/scale exclusions, embedding exclusions |

**Key insight:** The entire conversion pipeline is already implemented. This phase is ~90% plumbing (CLI + CMake) and ~10% new code (metadata injection).

## Common Pitfalls

### Pitfall 1: GGUF metadata gap in save_to_gguf_file
**What goes wrong:** `save_to_gguf_file` creates a fresh `gguf_context` internally and writes only tensors — no `general.model_type`, `general.architecture`, or `general.wan.version` keys. The output GGUF fails `is_wan_gguf()` validation, so `wan_load_model` rejects it (falls through to safetensors check, which also fails).
**Why it happens:** `save_to_gguf_file` is a generic method; it has no knowledge of WAN-specific metadata.
**How to avoid:** Two options:
  1. Add a `set_metadata` overload or pre-call to inject keys before `save_to_gguf_file` — but the method owns its own `gguf_context` so this doesn't work without modifying model.cpp.
  2. Write a thin `save_to_gguf_file_with_metadata` wrapper in the converter's `main.cpp` that replicates the save logic with metadata injection, OR post-process the file by re-opening with `gguf_init_from_file`, adding keys, and re-writing.
  3. Simplest: add an optional `std::map<std::string,std::string> metadata` parameter to `save_to_gguf_file` in `src/model.cpp` — one-line change, backward compatible (default empty map).
**Warning signs:** `wan_load_model` returns `WAN_ERROR_MODEL_LOAD_FAILED` with "Not a valid WAN GGUF or safetensors file" on the converted output.

### Pitfall 2: Sub-model type detection without metadata
**What goes wrong:** VAE, T5, and CLIP safetensors files do NOT contain DiT tensors — `get_sd_version()` may return a non-WAN version or wrong variant.
**Why it happens:** `get_sd_version` infers type from tensor names; VAE/T5/CLIP have different name patterns.
**How to avoid:** Require `--type` flag (dit-t2v | dit-i2v | dit-ti2v | vae | t5 | clip) and use it to set metadata directly rather than relying on `get_sd_version` for non-DiT sub-models.

### Pitfall 3: Missing `--type` leads to wrong architecture metadata
**What goes wrong:** Without explicit sub-model type, the converter cannot know whether a DiT file is T2V or I2V — both have similar tensor names.
**How to avoid:** Make `--type` a required argument (or default to `dit-t2v` with a warning).

### Pitfall 4: wan-convert not built by default
**What goes wrong:** `WAN_BUILD_EXAMPLES` defaults to `ON` only when standalone (`WAN_STANDALONE`). If user builds as subdirectory, `wan-convert` is silently skipped.
**How to avoid:** Document that `wan-convert` requires `-DWAN_BUILD_EXAMPLES=ON` when used as a subdirectory. No code change needed — existing CMake logic handles this correctly.

## Code Examples

### Minimal converter main.cpp skeleton
```cpp
// Source: pattern from stable-diffusion.cpp/examples/cli/main.cpp convert mode
// + src/model.h ModelLoader interface
#include "model.h"
#include "util.h"
#include <string>
#include <cstdio>

int main(int argc, char** argv) {
    // Parse: --input, --output, --type, --quant (default f16), --help
    std::string input, output, submodel_type = "dit-t2v", quant = "f16";

    // ... arg parsing ...

    ModelLoader loader;
    if (!loader.init_from_file(input)) {
        LOG_ERROR("failed to load %s", input.c_str());
        return 1;
    }
    loader.convert_tensors_name();

    // Map quant string to ggml_type
    ggml_type wtype = GGML_TYPE_F16;
    if (quant == "f32") wtype = GGML_TYPE_F32;
    else if (quant == "q8_0") wtype = GGML_TYPE_Q8_0;
    // ... etc

    // save_to_gguf_file needs metadata injection — see pitfall 1
    // Option: extend save_to_gguf_file with metadata map parameter
    std::map<std::string, std::string> meta;
    meta["general.model_type"]   = "wan";
    meta["general.architecture"] = arch_for_type(submodel_type);  // e.g. "WAN-T2V"
    meta["general.wan.version"]  = version_for_type(submodel_type); // "WAN2.1" or "WAN2.2"

    if (!loader.save_to_gguf_file(output, wtype, "")) {
        LOG_ERROR("conversion failed");
        return 1;
    }
    LOG_INFO("converted %s -> %s", input.c_str(), output.c_str());
    return 0;
}
```

### Metadata injection via save_to_gguf_file extension (minimal model.cpp change)
```cpp
// In src/model.h — add overload:
bool save_to_gguf_file(const std::string& file_path,
                       ggml_type type,
                       const std::string& tensor_type_rules,
                       const std::map<std::string, std::string>& metadata = {});

// In src/model.cpp — after gguf_ctx = gguf_init_empty():
for (const auto& kv : metadata) {
    gguf_set_val_str(gguf_ctx, kv.first.c_str(), kv.second.c_str());
}
```

### Sub-model type → metadata table
```cpp
// Source: src/api/wan_loader.cpp is_wan_gguf() — these are the exact keys it reads
struct SubModelInfo { const char* arch; const char* version; };
static const std::map<std::string, SubModelInfo> SUBMODEL_META = {
    {"dit-t2v",  {"WAN-T2V",  "WAN2.1"}},
    {"dit-i2v",  {"WAN-I2V",  "WAN2.2"}},
    {"dit-ti2v", {"WAN-TI2V", "WAN2.2"}},
    {"vae",      {"WAN-VAE",  "WAN2.1"}},
    {"t5",       {"WAN-T5",   "WAN2.1"}},
    {"clip",     {"WAN-CLIP", "WAN2.2"}},
};
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Embed vocab in binary | mmap at runtime | Phase 9 | Converter binary stays small |
| GGUF-only loading | safetensors + GGUF dispatch | Phase 10 | Converter output must still pass is_wan_gguf() |

## Open Questions

1. **Metadata for VAE/T5/CLIP sub-models**
   - What we know: `is_wan_gguf()` checks `general.model_type == "wan"` and `general.architecture` for T2V/I2V/TI2V
   - What's unclear: Does `wan_load_model` use architecture metadata to route VAE/T5/CLIP loading, or does it rely solely on tensor names? If the latter, any `general.architecture` value containing "WAN" may suffice.
   - Recommendation: Use distinct architecture strings per sub-model (e.g. "WAN-VAE") and verify `is_wan_gguf()` accepts them. If not, extend `is_wan_gguf()` to accept VAE/T5/CLIP architecture strings.

2. **save_to_gguf_file metadata injection approach**
   - What we know: Current signature has no metadata parameter; method owns its gguf_context
   - What's unclear: Whether to extend `save_to_gguf_file` in model.cpp (touches shared code) or write a post-process step in converter main.cpp
   - Recommendation: Extend `save_to_gguf_file` with optional `metadata` map — minimal change, backward compatible, cleanest approach.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None detected — manual smoke tests only |
| Config file | none |
| Quick run command | `./build/bin/wan-convert --help` |
| Full suite command | `./build/bin/wan-convert --input <file.safetensors> --output <file.gguf> --type dit-t2v && ./build/bin/wan-cli --model <file.gguf> --mode t2v --prompt "test"` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SAFE-02 | wan-convert --help exits 0 with usage | smoke | `./build/bin/wan-convert --help` | ❌ Wave 0 |
| SAFE-03 | Convert DiT safetensors → GGUF loadable by wan_load_model | integration | manual — requires model files | ❌ Wave 0 |

### Sampling Rate
- Per task commit: `cmake --build build --target wan-convert 2>&1 | tail -5`
- Per wave merge: `./build/bin/wan-convert --help`
- Phase gate: Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `examples/convert/main.cpp` — wan-convert entry point
- [ ] `examples/convert/CMakeLists.txt` — build target
- [ ] `examples/CMakeLists.txt` update — add_subdirectory(convert)
- [ ] `src/model.cpp` + `src/model.h` — extend save_to_gguf_file with metadata parameter

## Sources

### Primary (HIGH confidence)
- `src/model.cpp` lines 1709-1777 — `save_to_gguf_file` full implementation, verified by direct read
- `src/model.h` lines 294-344 — `ModelLoader` class declaration, all method signatures
- `src/api/wan_loader.cpp` lines 36-94 — `is_wan_gguf()` exact metadata keys read
- `ggml/include/gguf.h` lines 132-143 — `gguf_set_val_str` API
- `examples/cli/CMakeLists.txt` — CMake pattern for wan-cli executable
- `CMakeLists.txt` — WAN_BUILD_EXAMPLES, WAN_STANDALONE, source list

### Secondary (MEDIUM confidence)
- `stable-diffusion.cpp/examples/cli/main.cpp` lines 522-535 — convert mode calling `save_to_gguf_file` with wtype

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all APIs verified by direct source read
- Architecture: HIGH — CMake pattern copied from existing wan-cli
- Pitfalls: HIGH — GGUF metadata gap verified by reading save_to_gguf_file source (no gguf_set_val_str calls present)

**Research date:** 2026-03-17
**Valid until:** 2026-04-17 (stable codebase, no external dependencies)
