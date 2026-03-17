---
phase: 10
slug: safetensors-runtime-loading
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-17
---

# Phase 10 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None — no test directory exists; build + manual CLI |
| **Config file** | none |
| **Quick run command** | `cmake --build build --target wan-cpp 2>&1 \| tail -5` |
| **Full suite command** | Manual: build + run wan-cli with .safetensors model path, verify AVI output |
| **Estimated runtime** | ~30 seconds (build only) |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build --target wan-cpp 2>&1 | tail -5`
- **After every plan wave:** Full build + manual CLI smoke test with safetensors model
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** ~30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 10-01-01 | 01 | 1 | SAFE-01 | build | `cmake --build build --target wan-cpp` | N/A | ⬜ pending |
| 10-01-02 | 01 | 1 | SAFE-01 | grep | `grep -n "is_safetensors_file\|safetensors" src/api/wan-api.cpp` | N/A | ⬜ pending |
| 10-01-03 | 01 | 1 | SAFE-01 | grep | `grep -n "get_sd_version\|VERSION_WAN" src/api/wan-api.cpp` | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

None — Existing infrastructure covers all phase requirements. All validation is build + manual CLI execution.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| `wan_load_model` accepts .safetensors path and returns valid handle | SAFE-01 | No test harness | Run `wan-cli --model model.safetensors --mode t2v ...`; check exit code 0 |
| safetensors-loaded model produces T2V output equivalent to GGUF | SAFE-01 | No regression test | Run same prompt/seed with both formats; compare AVI output visually |
| Invalid safetensors file returns error code, no crash | SAFE-01 | No test harness | Pass truncated/corrupt .safetensors file; verify non-zero exit, no segfault |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
