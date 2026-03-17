---
phase: 10-safetensors-runtime-loading
verified: 2026-03-17T03:00:00Z
status: human_needed
score: 3/4 must-haves verified
re_verification: false
human_verification:
  - test: "Load a real .safetensors WAN2.1 T2V checkpoint via wan_load_model"
    expected: "Returns WAN_SUCCESS, non-null context handle, model_type=t2v, model_version=WAN2.1"
    why_human: "No test harness; requires an actual .safetensors model file on disk"
  - test: "Run T2V generation with safetensors-loaded model, compare output to GGUF-loaded model"
    expected: "AVI output produced; visual quality equivalent to GGUF path"
    why_human: "Output equivalence requires visual comparison; no automated regression baseline exists"
  - test: "Pass a truncated/corrupt .safetensors file to wan_load_model"
    expected: "Returns WAN_ERROR_MODEL_LOAD_FAILED, no segfault, no crash"
    why_human: "Requires runtime execution; is_safetensors_file reads 8 bytes — a file truncated after the header size field but before valid JSON may behave differently at runtime"
---

# Phase 10: Safetensors Runtime Loading — Verification Report

**Phase Goal:** 用户可直接用 .safetensors 文件调用 wan_load_model，无需预转换
**Verified:** 2026-03-17T03:00:00Z
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | `wan_load_model` accepts a .safetensors path and returns a valid non-null context handle | ? HUMAN | Code path exists and is wired; runtime behavior requires actual .safetensors file |
| 2 | safetensors-loaded model runs T2V generation without crashing | ? HUMAN | Runner construction is identical for both paths; actual execution requires runtime test |
| 3 | Passing an invalid/corrupt safetensors file returns WAN_ERROR_MODEL_LOAD_FAILED, no segfault | ? HUMAN | `wan_load_model` returns `WAN_ERROR_MODEL_LOAD_FAILED` when `WanModel::load` fails (line 282); error path is wired; runtime behavior with corrupt input needs human test |
| 4 | GGUF loading path is unchanged and still works | ✓ VERIFIED | `is_wan_gguf` + `init_from_file_and_convert_name("model.diffusion_model.")` preserved verbatim in else-branch (lines 169-177); no modifications to GGUF logic |

**Score:** 1/4 automated + 3/4 pending human = all code paths verified, runtime behavior needs human

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/api/wan-api.cpp` | WanModel::load with safetensors dispatch branch | ✓ VERIFIED | `is_safetensors_file` called at line 129; `is_st` branch at line 146 |
| `src/api/wan-api.cpp` | SDVersion inference from tensor names for safetensors | ✓ VERIFIED | `get_sd_version()` called at line 158; all three WAN variants mapped (lines 165-167) |
| `src/model.h` | `is_safetensors_file` declaration | ✓ VERIFIED | Declared at line 292, before `class ModelLoader` |
| `src/model.cpp` | `is_safetensors_file` definition | ✓ VERIFIED | Defined at line 318; also used internally at line 367 |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `WanModel::load` | `is_safetensors_file()` | format detection at line 129 | ✓ WIRED | `bool is_st = is_safetensors_file(file_path);` |
| `WanModel::load` | `ModelLoader::init_from_file` | safetensors branch, line 149 | ✓ WIRED | `model_loader.init_from_file(file_path)` — no prefix, correct per research |
| `WanModel::load` | `ModelLoader::convert_tensors_name` | after init_from_file, line 155 | ✓ WIRED | Called unconditionally in safetensors branch |
| `WanModel::load` | `ModelLoader::get_sd_version` | version inference, line 158 | ✓ WIRED | `SDVersion sv = model_loader.get_sd_version();` |
| `WanModel::load` | `sd_version_is_wan` | WAN validation, line 159 | ✓ WIRED | Non-WAN safetensors returns error with clear message |
| `WanModel::load` | `Wan::is_wan_gguf` | GGUF branch, line 130 | ✓ WIRED | Only called when `!is_st` — anti-pattern avoided |
| `wan_load_model` | `WanModel::load` | line 279 | ✓ WIRED | `WanModelLoadResult result = Wan::WanModel::load(ctx->model_path)` |
| `WanModel::load` failure | `WAN_ERROR_MODEL_LOAD_FAILED` | line 282 | ✓ WIRED | `return WAN_ERROR_MODEL_LOAD_FAILED` when `!result.success` |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| SAFE-01 | 10-01-PLAN.md | 用户可直接加载 .safetensors 格式的 WAN 模型文件（无需预转换） | ? HUMAN | Code fully wired; runtime acceptance requires human test with actual file |
| SAFE-02 | — | safetensors → GGUF 转换工具 | ORPHANED | Not in Phase 10 scope; REQUIREMENTS.md marks it unchecked; no plan claims it |
| SAFE-03 | — | 转换工具支持所有子模型 | ORPHANED | Not in Phase 10 scope; REQUIREMENTS.md marks it unchecked; no plan claims it |

Note: SAFE-02 and SAFE-03 are not Phase 10 requirements — they are future-phase items. No orphan gap for this phase.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| None | — | — | — | No TODO/FIXME/placeholder/stub patterns found in modified files |

Key anti-patterns from research explicitly avoided:
- `is_wan_gguf` is NOT called on safetensors input (guarded by `!is_st`)
- `init_from_file` called with NO prefix for safetensors (correct — HF names already canonical)
- `convert_tensors_name()` IS called in safetensors branch
- Dimension reversal NOT duplicated (`init_from_safetensors_file` handles it internally)

### Human Verification Required

#### 1. Safetensors Load — Happy Path

**Test:** Run `./build/examples/cli/wan-cli --model /path/to/wan2.1-t2v.safetensors --mode t2v --prompt "a cat" --output /tmp/test.avi`
**Expected:** Exit code 0, `/tmp/test.avi` created, no error messages
**Why human:** No test harness; requires an actual WAN2.1 .safetensors checkpoint file

#### 2. T2V Output Equivalence

**Test:** Run same prompt + seed with both a .safetensors and equivalent .gguf model; compare AVI outputs
**Expected:** Visually equivalent video output
**Why human:** Output quality comparison requires visual inspection; no automated pixel-diff baseline exists

#### 3. Corrupt File Error Handling

**Test:** Pass a file with `.safetensors` extension but truncated/random content to `wan_load_model`
**Expected:** Returns `WAN_ERROR_MODEL_LOAD_FAILED`, no segfault, `wan_get_last_error` returns a non-empty message
**Why human:** Requires runtime execution; edge cases in `is_safetensors_file` (8-byte read + JSON parse) need live testing

### Gaps Summary

No automated gaps found. All code artifacts exist, are substantive, and are correctly wired. The three success criteria from ROADMAP.md map directly to runtime behaviors that cannot be verified without an actual `.safetensors` model file. The implementation matches the plan exactly — two-branch dispatch, correct API surface, error propagation wired to `WAN_ERROR_MODEL_LOAD_FAILED`.

---

_Verified: 2026-03-17T03:00:00Z_
_Verifier: Claude Sonnet 4.6 (gsd-verifier)_
