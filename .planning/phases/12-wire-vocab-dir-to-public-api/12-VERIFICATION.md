---
phase: 12-wire-vocab-dir-to-public-api
verified: 2026-03-17T00:00:00Z
status: human_needed
score: 5/5 must-haves verified
re_verification: false
human_verification:
  - test: "WAN_EMBED_VOCAB=OFF build: run wan-cli with --vocab-dir pointing to a real vocab directory and a real model, confirm generation succeeds"
    expected: "Video file written to output path, no error exit"
    why_human: "Requires real model file and real vocab files on disk; cannot verify end-to-end generation programmatically"
  - test: "WAN_EMBED_VOCAB=OFF build: run wan-cli without --vocab-dir, confirm warning printed to stderr and wan_load_model returns error"
    expected: "stderr contains 'Warning: WAN_EMBED_VOCAB=OFF build but --vocab-dir not provided', process exits non-zero"
    why_human: "Requires a compiled WAN_EMBED_VOCAB=OFF binary and observation of stderr at runtime"
  - test: "WAN_EMBED_VOCAB=ON build: call wan_set_vocab_dir and confirm return value is WAN_ERROR_INVALID_ARGUMENT"
    expected: "Return value equals WAN_ERROR_INVALID_ARGUMENT (1)"
    why_human: "Requires a compiled WAN_EMBED_VOCAB=ON binary and a small test caller to inspect the return value"
---

# Phase 12: Wire Vocab Dir to Public API — Verification Report

**Phase Goal:** T2V/I2V 生成在 WAN_EMBED_VOCAB=OFF 时正常工作，词汇表目录通过公共 API 设置
**Verified:** 2026-03-17T00:00:00Z
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | `wan.h` declares `wan_set_vocab_dir(const char* dir)` returning `wan_error_t` | VERIFIED | `include/wan-cpp/wan.h:340` — `wan_error_t wan_set_vocab_dir(const char* dir);` in Parameter Configuration section |
| 2 | `wan-cli --help` output includes `--vocab-dir` | VERIFIED | `examples/cli/main.cpp:101` — printf of `--vocab-dir <path>` in `print_usage()`; parse handler at line 259 |
| 3 | `WAN_EMBED_VOCAB=OFF` build: `wan_load_model` returns `WAN_ERROR_INVALID_ARGUMENT` when vocab dir not set | VERIFIED | `src/api/wan-api.cpp:287-305` — `#ifndef WAN_EMBED_VOCAB` guard calls `wan_vocab_dir_is_set()`, returns `WAN_ERROR_INVALID_ARGUMENT` if false; also validates directory existence via `stat()`/`S_ISDIR()` |
| 4 | `WAN_EMBED_VOCAB=OFF` build: `wan-cli` without `--vocab-dir` prints a warning to stderr at startup | VERIFIED | `examples/cli/main.cpp:414-424` — `#ifndef WAN_EMBED_VOCAB` block prints `"Warning: WAN_EMBED_VOCAB=OFF build but --vocab-dir not provided; vocab loading will fail"` to stderr before `wan_load_model` |
| 5 | `WAN_EMBED_VOCAB=ON` build: `wan_set_vocab_dir` returns `WAN_ERROR_INVALID_ARGUMENT` (no-op) | VERIFIED | `src/api/wan-api.cpp:113-116` — `#ifdef WAN_EMBED_VOCAB` early return with `WAN_ERROR_INVALID_ARGUMENT` before any state mutation |

**Score:** 5/5 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/vocab/vocab.h` | `wan_vocab_dir_is_set()` and `wan_vocab_get_dir()` accessor declarations | VERIFIED | Lines 19-22: both declarations present with correct signatures |
| `src/vocab/vocab.cpp` | `wan_vocab_dir_is_set()` and `wan_vocab_get_dir()` implementations | VERIFIED | Lines 33-39: both bodies implemented; `wan_vocab_dir_is_set()` returns `!g_vocab_dir.empty()`, `wan_vocab_get_dir()` returns `g_vocab_dir` |
| `include/wan-cpp/wan.h` | `wan_set_vocab_dir` public C API declaration | VERIFIED | Line 340: declaration with full doc comment in Parameter Configuration section, inside `extern "C"` block |
| `src/api/wan-api.cpp` | `wan_set_vocab_dir` implementation + load-time vocab dir validation in `wan_load_model` | VERIFIED | Lines 113-122: `WAN_API wan_error_t wan_set_vocab_dir` with `WAN_EMBED_VOCAB` guard and null check; lines 287-305: `#ifndef WAN_EMBED_VOCAB` guard in `wan_load_model` with `stat()`/`S_ISDIR()` directory validation |
| `examples/cli/main.cpp` | `--vocab-dir` argument parsing and `WAN_EMBED_VOCAB` startup warning | VERIFIED | Line 29: `char* vocab_dir` struct field; line 136: `opts->vocab_dir = NULL` init; line 259-262: parse handler; line 101: usage string; lines 414-424: `#ifndef WAN_EMBED_VOCAB` warning + `wan_set_vocab_dir` call before `wan_load_model` |
| `examples/cli/CMakeLists.txt` | `WAN_EMBED_VOCAB` compile definition propagated to `wan-cli` target | VERIFIED | Lines 16-18: `if(WAN_EMBED_VOCAB) target_compile_definitions(wan-cli PRIVATE WAN_EMBED_VOCAB) endif()` |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `examples/cli/main.cpp` | `wan_set_vocab_dir` | called in `main()` before `wan_load_model` when `opts.vocab_dir` is set | WIRED | Line 418: `wan_set_vocab_dir(opts.vocab_dir)` inside `#ifndef WAN_EMBED_VOCAB` block, before `wan_load_model` call at line 431 |
| `src/api/wan-api.cpp (wan_load_model)` | `src/vocab/vocab.h (wan_vocab_dir_is_set)` | `#ifndef WAN_EMBED_VOCAB` guard calling `wan_vocab_dir_is_set()` | WIRED | Line 288: `if (!wan_vocab_dir_is_set())` inside `#ifndef WAN_EMBED_VOCAB`; `vocab.h` included at line 38 |
| `src/api/wan-api.cpp (wan_set_vocab_dir)` | `src/vocab/vocab.cpp (wan_vocab_set_dir)` | direct call: `wan_vocab_set_dir(std::string(dir))` | WIRED | Line 120: `wan_vocab_set_dir(std::string(dir))` after null check and `WAN_EMBED_VOCAB` guard |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| PERF-01 | 12-01-PLAN.md | 词汇表文件改为运行时 mmap 加载，消除编译时头文件嵌入 | SATISFIED | `vocab.cpp` uses `mmap_read()` from `g_vocab_dir`; public API `wan_set_vocab_dir` exposes the dir setter; `wan_load_model` validates dir before load |
| ENCODER-01 | 12-01-PLAN.md | 集成 T5 文本编码器 | SATISFIED (dependency) | T5 tokenizer calls `load_umt5_tokenizer_json()` / `load_t5_tokenizer_json()` which read from `g_vocab_dir`; Phase 12 ensures `g_vocab_dir` is set via public API before model load. Core integration completed in Phase 7. |
| ENCODER-02 | 12-01-PLAN.md | 集成 CLIP 图像编码器 | SATISFIED (dependency) | CLIP tokenizer calls `load_clip_merges()` which reads from `g_vocab_dir`; same path as ENCODER-01. Core integration completed in Phase 7. |
| API-03 | 12-01-PLAN.md | 实现文本生成视频（T2V）接口 | NOTE | REQUIREMENTS.md maps API-03 to Phase 8 (already Complete). Phase 12 enables correct T2V operation under `WAN_EMBED_VOCAB=OFF` by ensuring vocab is loadable. Not a gap — Phase 12 is a prerequisite fixer, not the owner. |
| API-04 | 12-01-PLAN.md | 实现图像生成视频（I2V）接口 | NOTE | Same as API-03 — mapped to Phase 8 in REQUIREMENTS.md. Phase 12 enables correct I2V operation under `WAN_EMBED_VOCAB=OFF`. Not a gap. |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| None | — | — | — | No TODOs, placeholders, stub returns, or empty handlers found in any of the 6 modified files |

### Human Verification Required

#### 1. End-to-end T2V/I2V generation with WAN_EMBED_VOCAB=OFF

**Test:** Build with `cmake -DWAN_EMBED_VOCAB=OFF ..`, then run `./wan-cli --model <model.gguf> --vocab-dir <vocab_dir> --prompt "test" -o out.avi`
**Expected:** Video file `out.avi` written successfully, process exits 0
**Why human:** Requires real model weights and real vocab files on disk; cannot verify generation pipeline end-to-end programmatically

#### 2. Missing --vocab-dir warning at runtime

**Test:** Build with `cmake -DWAN_EMBED_VOCAB=OFF ..`, then run `./wan-cli --model <model.gguf> --prompt "test" -o out.avi 2>&1 | grep -i warn`
**Expected:** stderr contains `Warning: WAN_EMBED_VOCAB=OFF build but --vocab-dir not provided; vocab loading will fail`, process exits non-zero
**Why human:** Requires a compiled `WAN_EMBED_VOCAB=OFF` binary; cannot run the binary in this verification context

#### 3. wan_set_vocab_dir return value under WAN_EMBED_VOCAB=ON

**Test:** Build with `cmake -DWAN_EMBED_VOCAB=ON ..`, write a small test caller that calls `wan_set_vocab_dir("/some/path")` and prints the return value
**Expected:** Return value is `1` (`WAN_ERROR_INVALID_ARGUMENT`)
**Why human:** Requires a compiled `WAN_EMBED_VOCAB=ON` binary and a test harness to inspect the return value

### Gaps Summary

No gaps found. All 5 observable truths are verified by code inspection. All 6 required artifacts exist, are substantive, and are correctly wired. All 3 key links are confirmed present and connected. The two requirement IDs (API-03, API-04) claimed in the PLAN frontmatter are already owned by Phase 8 in REQUIREMENTS.md — Phase 12 is a prerequisite enabler for those interfaces under `WAN_EMBED_VOCAB=OFF`, which is consistent with PERF-01's description. No structural issue.

Automated verification is complete. Three runtime behaviors require human confirmation with a compiled binary and real model/vocab files.

---

_Verified: 2026-03-17T00:00:00Z_
_Verifier: Claude Sonnet 4.6 (gsd-verifier)_
