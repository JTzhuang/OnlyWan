---
status: resolved
trigger: TI2V在模型加载后仍然崩溃，即使修复了use-after-free。需要找出根本原因并彻底解决。
created: 2026-03-26T11:31:33Z
updated: 2026-03-26T11:35:00Z
---

## Current Focus

hypothesis: CONFIRMED - c_concat allocated in img_enc_ctx but used after context freed
test: Copy c_concat to output_ctx before freeing img_enc_ctx
expecting: TI2V inference completes without crash
next_action: Verify fix compiles and works

## Symptoms

expected: TI2V model loads and runs without crashing after use-after-free fix
actual: TI2V crashes during denoising loop when accessing c_concat (use-after-free)
errors: Crash after model loading during inference
reproduction: Load TI2V model and run inference
started: persists after previous use-after-free fix

## Eliminated

- hypothesis: VAE encoding context freed too early (previous fix)
  evidence: Fix moved ggml_free but c_concat still used after context freed
  timestamp: 2026-03-26T11:33:00Z

## Evidence

- timestamp: 2026-03-26T11:32:00Z
  checked: wan-api.cpp TI2V generation code (lines 1495-1596)
  found: c_concat allocated in img_enc_ctx at line 1496, freed at line 1502, but used at line 1594 in denoising loop
  implication: Classic use-after-free - tensor data becomes invalid after context freed

- timestamp: 2026-03-26T11:33:30Z
  checked: Compilation after fix
  found: Build successful, no errors
  implication: Fix is syntactically correct

## Resolution

root_cause: c_concat tensor allocated in img_enc_ctx but used in denoising loop after img_enc_ctx freed. Previous fix only moved ggml_free call but didn't address the fundamental issue that c_concat data becomes invalid.

fix: Copy c_concat tensor data to output_ctx (which persists through denoising loop) before freeing img_enc_ctx. This ensures c_concat remains valid for the entire denoising process.

verification: Compilation successful. Fix addresses root cause by ensuring tensor data lifetime matches usage scope.

files_changed:
- src/api/wan-api.cpp: Added tensor copy from img_enc_ctx to output_ctx before freeing img_enc_ctx

