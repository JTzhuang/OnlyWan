---
phase: 11
slug: safetensors-conversion-tool
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-17
---

# Phase 11 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None — manual smoke tests + build verification |
| **Config file** | none |
| **Quick run command** | `cmake --build build --target wan-convert 2>&1 | tail -5` |
| **Full suite command** | `./build/bin/wan-convert --help` |
| **Estimated runtime** | ~30 seconds (build), ~1 second (smoke) |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build --target wan-convert 2>&1 | tail -5`
- **After every plan wave:** Run `./build/bin/wan-convert --help`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 11-01-01 | 01 | 1 | SAFE-02 | build | `cmake --build build --target wan-convert 2>&1 | tail -5` | ❌ W0 | ⬜ pending |
| 11-01-02 | 01 | 1 | SAFE-02 | smoke | `./build/bin/wan-convert --help` | ❌ W0 | ⬜ pending |
| 11-01-03 | 01 | 1 | SAFE-03 | smoke | `./build/bin/wan-convert --input x --output y --type dit-t2v 2>&1 | grep -E "error|usage"` | ❌ W0 | ⬜ pending |
| 11-01-04 | 01 | 1 | SAFE-02 | build | `cmake --build build --target wan-convert && echo BUILD_OK` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `examples/convert/main.cpp` — wan-convert CLI entry point
- [ ] `examples/convert/CMakeLists.txt` — build target for wan-convert
- [ ] `examples/CMakeLists.txt` — add `add_subdirectory(convert)` line
- [ ] `src/model.h` — extend `save_to_gguf_file` with optional metadata map parameter
- [ ] `src/model.cpp` — implement metadata injection in `save_to_gguf_file`

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Convert DiT safetensors → GGUF loadable by wan_load_model | SAFE-03 | Requires actual WAN2.1/2.2 model files not in repo | Run `./build/bin/wan-convert --input dit.safetensors --output dit.gguf --type dit-t2v`, then `./build/bin/wan-cli --model dit.gguf --mode t2v --prompt "test"` |
| Converted GGUF output matches safetensors direct-load output | SAFE-03 | Requires visual/perceptual comparison of generated frames | Generate video from converted GGUF and from original safetensors; compare frame content |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
