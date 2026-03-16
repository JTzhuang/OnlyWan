---
phase: 09-api-fixes-vocab-mmap
verified: 2026-03-17T00:00:00Z
status: human_needed
score: 9/9 must-haves verified
re_verification: false
human_verification:
  - test: "Run wan_generate_video_t2v and wan_generate_video_i2v with the same seed/prompt as a known _ex run"
    expected: "Output AVI is byte-identical or visually equivalent to the _ex output — no regression in generation quality"
    why_human: "Cannot verify output equivalence programmatically without a live model and reference output"
---

# Phase 9: API Fixes + Vocab mmap Verification Report

**Phase Goal:** v1.0 遗留问题全部修复，API 行为与文档一致
**Verified:** 2026-03-17T00:00:00Z
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | `wan_generate_video_t2v` delegates to `wan_generate_video_t2v_ex` and returns its result | VERIFIED | `src/api/wan_t2v.cpp` line 35: `return wan_generate_video_t2v_ex(ctx, prompt, &p, output_path);` — all 8 flat args mapped to `wan_params_t p` |
| 2 | `wan_generate_video_i2v` delegates to `wan_generate_video_i2v_ex` and returns its result | VERIFIED | `src/api/wan_i2v.cpp` line 35: `return wan_generate_video_i2v_ex(ctx, image, prompt, &p, output_path);` — width/height sourced from `ctx->params` or defaults 832/480 |
| 3 | `progress_cb` is called once per Euler step in both T2V and I2V loops, with correct step/total/progress values | VERIFIED | `src/api/wan-api.cpp` lines 518-527 (T2V) and 795-804 (I2V): `params->progress_cb(i, steps, (float)(i+1)/(float)steps, params->user_data)` after `ggml_free(step_ctx)` in each loop; grep count = 4 |
| 4 | A non-zero `progress_cb` return value aborts generation and returns `WAN_ERROR_GENERATION_FAILED` | VERIFIED | Abort paths at lines 523-525 (T2V: frees `denoise_ctx` + `output_ctx`) and 800-803 (I2V: frees `denoise_ctx` + `img_enc_ctx` + `output_ctx`) before returning `WAN_ERROR_GENERATION_FAILED` |
| 5 | When `WAN_EMBED_VOCAB` is OFF (default), vocab.cpp does not `#include` the large .hpp files at compile time | VERIFIED | No top-level `#include "umt5.hpp"`, `#include "clip_t5.hpp"`, `#include "mistral.hpp"`, or `#include "qwen.hpp"` in `src/vocab/vocab.cpp`; all 8 `WAN_EMBED_VOCAB` guards are inside function bodies |
| 6 | All six `load_*` functions return data from external files via mmap | VERIFIED | `load_vocab_file()` helper calls `mmap_read(g_vocab_dir + "/" + filename)` for each of the 6 functions; mmap implementation uses `open`/`fstat`/`mmap`/`munmap` on POSIX |
| 7 | `wan_vocab_set_dir()` allows callers to specify the directory containing external vocab files | VERIFIED | `src/vocab/vocab.h` line 16 declares `void wan_vocab_set_dir(const std::string& dir);`; `src/vocab/vocab.cpp` lines 29-31 implement it setting `g_vocab_dir` |
| 8 | When external vocab files are absent and `WAN_EMBED_VOCAB=OFF`, `load_*` functions return empty string | VERIFIED | `load_vocab_file()` returns `{}` when `g_vocab_dir` is empty or file not found; each `load_*` falls through to `#else return {};` when `WAN_EMBED_VOCAB` not defined |
| 9 | Library builds successfully with `WAN_EMBED_VOCAB=OFF` (default) | VERIFIED | All 6 commits exist and build succeeded per SUMMARY (commit c512e68 adds CMake option; ae9cdbd replaces vocab.cpp; no top-level large .hpp includes); `WAN_EMBED_VOCAB` option defaults OFF at CMakeLists.txt line 45 |

**Score:** 9/9 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/api/wan_t2v.cpp` | T2V stub delegation to `_ex` | VERIFIED | 39 lines; builds `wan_params_t p` from all flat args; calls `wan_generate_video_t2v_ex`; no `WAN_ERROR_UNSUPPORTED_OPERATION` |
| `src/api/wan_i2v.cpp` | I2V stub delegation to `_ex` | VERIFIED | 39 lines; builds `wan_params_t p`; width/height from `ctx->params` or 832/480; calls `wan_generate_video_i2v_ex`; no `WAN_ERROR_UNSUPPORTED_OPERATION` |
| `src/api/wan-api.cpp` | `progress_cb` invocation in both Euler loops | VERIFIED | 4 occurrences of `progress_cb` (2 null-checks + 2 invocations); 2 occurrences of `WAN_ERROR_GENERATION_FAILED` (one abort path per loop) |
| `src/vocab/vocab.h` | `wan_vocab_set_dir` declaration | VERIFIED | Declaration present at line 16; all 6 `load_*` declarations preserved; single include guard |
| `src/vocab/vocab.cpp` | mmap-based `load_*` implementations gated by `WAN_EMBED_VOCAB` | VERIFIED | 147 lines; `mmap_read` defined; `wan_vocab_set_dir` implemented; 8 `WAN_EMBED_VOCAB` guards; no top-level large .hpp includes |
| `CMakeLists.txt` | `WAN_EMBED_VOCAB` option wiring compile definition | VERIFIED | Line 45: `option(WAN_EMBED_VOCAB ... OFF)`; lines 157-162: `if(WAN_EMBED_VOCAB)` block with `target_compile_definitions(${WAN_LIB} PRIVATE WAN_EMBED_VOCAB)` |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `src/api/wan_t2v.cpp` | `src/api/wan-api.cpp` | `wan_generate_video_t2v_ex` call | WIRED | Line 35 calls `wan_generate_video_t2v_ex`; no `WAN_ERROR_UNSUPPORTED_OPERATION` present |
| `src/api/wan_i2v.cpp` | `src/api/wan-api.cpp` | `wan_generate_video_i2v_ex` call | WIRED | Line 35 calls `wan_generate_video_i2v_ex`; no `WAN_ERROR_UNSUPPORTED_OPERATION` present |
| `wan-api.cpp` T2V Euler loop (~line 518) | `params->progress_cb` | callback invocation after `ggml_free(step_ctx)` | WIRED | Lines 518-527: null-check + invocation with `(i, steps, (float)(i+1)/(float)steps, params->user_data)` + abort path |
| `wan-api.cpp` I2V Euler loop (~line 795) | `params->progress_cb` | callback invocation after `ggml_free(step_ctx)` | WIRED | Lines 795-804: identical pattern; abort path frees `denoise_ctx` + `img_enc_ctx` + `output_ctx` |
| `CMakeLists.txt` `WAN_EMBED_VOCAB` option | `src/vocab/vocab.cpp` `#ifdef WAN_EMBED_VOCAB` | `target_compile_definitions PRIVATE WAN_EMBED_VOCAB` | WIRED | CMakeLists.txt line 158 passes macro to compiler; vocab.cpp has 8 `#ifdef WAN_EMBED_VOCAB` guards |
| `src/t5.hpp` / `src/clip.hpp` call sites | `src/vocab/vocab.h` `load_*` functions | unchanged call sites — interface identical | WIRED | vocab.h signatures unchanged; `wan_vocab_set_dir` added without removing existing declarations |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| FIX-01 | 09-01-PLAN.md | 移除 `wan_generate_video_t2v` / `wan_generate_video_i2v` 遗留 stub，调用实际 `_ex` 实现 | SATISFIED | Both files delegate to `_ex`; `WAN_ERROR_UNSUPPORTED_OPERATION` absent from both; commits fb5078d + 8a76e8a verified |
| FIX-02 | 09-01-PLAN.md | `progress_cb` 在 Euler 去噪循环每步实际触发，传入当前步数和总步数 | SATISFIED | 4 `progress_cb` occurrences in wan-api.cpp; both loops fire callback with `(i, steps, fraction, user_data)`; commit c69a255 verified |
| PERF-01 | 09-02-PLAN.md | 词汇表文件（umt5/clip，~85MB）改为运行时 mmap 加载，消除编译时头文件嵌入 | SATISFIED | vocab.cpp uses `mmap_read`; no top-level large .hpp includes; `WAN_EMBED_VOCAB` defaults OFF; commits d2448d3 + ae9cdbd + c512e68 verified |

No orphaned requirements — REQUIREMENTS.md traceability table maps FIX-01, FIX-02, PERF-01 to Phase 9 and marks all three `[x]`.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| — | — | None found | — | — |

No `WAN_ERROR_UNSUPPORTED_OPERATION`, `TODO`, `FIXME`, `placeholder`, `return null`, or empty handler patterns found in any phase 9 modified file.

### Human Verification Required

#### 1. T2V/I2V regression check

**Test:** Run `wan_generate_video_t2v` (flat API) with the same seed, prompt, steps, and output path as a known `wan_generate_video_t2v_ex` run.
**Expected:** Output AVI is byte-identical or visually equivalent to the `_ex` output — no regression in generation quality or frame count.
**Why human:** Cannot verify output equivalence programmatically without a live model, reference output file, and pixel-level comparison. The delegation logic is correct in code but runtime behavior requires an actual inference run.

### Gaps Summary

No gaps. All 9 must-have truths verified, all 6 artifacts substantive and wired, all 3 key links confirmed, all 3 requirement IDs satisfied. One item deferred to human: runtime regression check confirming flat API output matches `_ex` output.

---

_Verified: 2026-03-17T00:00:00Z_
_Verifier: Claude Sonnet 4.6 (gsd-verifier)_
