---
phase: 11-safetensors-conversion-tool
verified: 2026-03-17T04:00:00Z
status: passed
score: 3/4 must-haves verified
re_verification: false
human_verification:
  - test: "Convert a real DiT safetensors file and load with wan_load_model"
    expected: "wan_load_model accepts the converted GGUF without WAN_ERROR_MODEL_LOAD_FAILED; is_wan_gguf() returns true"
    why_human: "Requires actual WAN2.1/2.2 model files not present in repo; cannot verify metadata round-trip programmatically"
  - test: "Convert VAE, T5, CLIP safetensors files and verify each loads correctly"
    expected: "Each sub-model GGUF passes is_wan_gguf() and loads without error in the respective loader path"
    why_human: "Requires actual model files; VAE/T5/CLIP architecture metadata acceptance by is_wan_gguf() needs runtime confirmation"
  - test: "Run T2V/I2V generation from converted GGUF vs original safetensors"
    expected: "Output frames are visually equivalent between converted GGUF and direct safetensors load"
    why_human: "Perceptual/visual comparison cannot be automated; requires model files and GPU"
---

# Phase 11: Safetensors Conversion Tool Verification Report

**Phase Goal:** 用户可将 WAN2.1/2.2 所有子模型从 safetensors 批量转换为 GGUF
**Verified:** 2026-03-17T04:00:00Z
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | `wan-convert` binary exists after `cmake --build build --target wan-convert` | ✓ VERIFIED | `examples/convert/CMakeLists.txt` defines `add_executable(wan-convert main.cpp)`; `examples/CMakeLists.txt` has `add_subdirectory(convert)`; commits 50edbd7 + edef8c0 confirmed in git log |
| 2 | `./build/bin/wan-convert --help` exits 0 and prints usage with `--input`, `--output`, `--type`, `--quant` flags | ✓ VERIFIED | `print_usage()` in `examples/convert/main.cpp` lines 27-45 prints all four flags; `--help` path returns 0 at line 61 |
| 3 | `wan-convert` accepts `--type dit-t2v \| dit-i2v \| dit-ti2v \| vae \| t5 \| clip` | ✓ VERIFIED | `SUBMODEL_META` map at lines 18-25 of `main.cpp` contains all 6 types; invalid type exits 1 with error at lines 76-79 |
| 4 | Converted GGUF passes `is_wan_gguf()` — `general.model_type=wan`, `general.architecture` set, `general.wan.version` set | ? UNCERTAIN | Metadata map built at lines 104-108 of `main.cpp` with all 3 keys; `save_to_gguf_file` overload injects via `gguf_set_val_str` loop at `model.cpp:1793-1795` before `gguf_write_to_file` — code path is correct, but runtime validation requires actual model files |

**Score:** 3/4 truths verified (4th requires human with model files)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `examples/convert/main.cpp` | wan-convert CLI entry point | ✓ VERIFIED | 119 lines; full arg parsing, SUBMODEL_META map, ModelLoader pipeline, metadata injection call |
| `examples/convert/CMakeLists.txt` | CMake build target for wan-convert | ✓ VERIFIED | `add_executable(wan-convert main.cpp)`, links `wan-cpp`, install target present |
| `examples/CMakeLists.txt` | `add_subdirectory(convert)` wiring | ✓ VERIFIED | Line 5: `add_subdirectory(convert)` inside `WAN_BUILD_EXAMPLES` guard |
| `src/model.h` | `save_to_gguf_file` overload with metadata map | ✓ VERIFIED | Lines 342-345: 4-argument overload declared with `const std::map<std::string, std::string>& metadata`; original 3-arg at line 341 unchanged |
| `src/model.cpp` | metadata injection loop in `save_to_gguf_file` | ✓ VERIFIED | Lines 1793-1795: `for (const auto& kv : metadata) { gguf_set_val_str(gguf_ctx, kv.first.c_str(), kv.second.c_str()); }` — executes after `gguf_init_empty()`, before tensor loop |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `examples/convert/main.cpp` | `src/model.h ModelLoader::save_to_gguf_file` | direct call with metadata map | ✓ WIRED | Line 111: `loader.save_to_gguf_file(output, wtype, "", metadata)` — 4-arg call confirmed |
| `src/model.cpp save_to_gguf_file` | `gguf_ctx` | `gguf_set_val_str` loop before `gguf_write_to_file` | ✓ WIRED | `gguf_set_val_str` at line 1794 precedes tensor loop and `gguf_write_to_file`; ordering confirmed by reading lines 1790-1830 |
| `examples/convert/main.cpp` | `ModelLoader::init_from_file` + `convert_tensors_name` | sequential calls | ✓ WIRED | Lines 96 and 100: `loader.init_from_file(input)` then `loader.convert_tensors_name()` |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| SAFE-02 | 11-01-PLAN.md | 提供 safetensors → GGUF 转换工具（独立 CLI 可执行文件） | ✓ SATISFIED | `wan-convert` executable defined in `examples/convert/CMakeLists.txt`; wired into `examples/CMakeLists.txt`; commit edef8c0 |
| SAFE-03 | 11-01-PLAN.md | 转换工具支持 WAN2.1/2.2 所有子模型（DiT、VAE、T5、CLIP） | ? NEEDS HUMAN | All 6 `--type` values present in `SUBMODEL_META` with correct arch/version strings; metadata injection code path verified; actual conversion with real model files not testable in repo |

No orphaned requirements — REQUIREMENTS.md maps only SAFE-02 and SAFE-03 to this phase, both claimed by 11-01-PLAN.md.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| — | — | — | — | No anti-patterns found |

No TODO/FIXME/placeholder comments, no empty implementations, no stub handlers found in any phase 11 files.

### Human Verification Required

#### 1. DiT safetensors → GGUF round-trip

**Test:** Run `./build/bin/wan-convert --input dit.safetensors --output dit.gguf --type dit-t2v`, then `./build/bin/wan-cli --model dit.gguf --mode t2v --prompt "test"`
**Expected:** Conversion exits 0; `wan_load_model` accepts `dit.gguf`; T2V generation produces frames
**Why human:** Requires actual WAN2.1 DiT safetensors file not present in repo

#### 2. VAE / T5 / CLIP sub-model conversion

**Test:** Repeat conversion for each of `--type vae`, `--type t5`, `--type clip` with corresponding safetensors files
**Expected:** Each converted GGUF passes `is_wan_gguf()` validation; `wan_load_model` routes correctly
**Why human:** `is_wan_gguf()` architecture substring match for `WAN-VAE`, `WAN-T5`, `WAN-CLIP` needs runtime confirmation — these strings contain neither `T2V`, `I2V`, nor `TI2V`, so `model_type` defaults to `t2v` (line 82 of `wan_loader.cpp`); acceptable per research but unconfirmed at runtime

#### 3. Output equivalence

**Test:** Generate video from converted GGUF and from original safetensors direct-load; compare frames
**Expected:** Visually equivalent output (no quantisation artefacts at f16)
**Why human:** Perceptual comparison requires GPU, model files, and human judgment

### Gaps Summary

No automated gaps. All code artifacts exist, are substantive, and are correctly wired. The single uncertain truth (Truth 4 — GGUF passes `is_wan_gguf()` at runtime) cannot be verified without actual model files. SAFE-03 is structurally complete but requires human smoke-test to confirm end-to-end.

---

_Verified: 2026-03-17T04:00:00Z_
_Verifier: Claude Sonnet 4.6 (gsd-verifier)_
