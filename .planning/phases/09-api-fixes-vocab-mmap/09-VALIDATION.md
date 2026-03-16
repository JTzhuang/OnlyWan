---
phase: 9
slug: api-fixes-vocab-mmap
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-16
---

# Phase 9 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None — no test directory exists; build + manual CLI |
| **Config file** | none |
| **Quick run command** | `cmake --build build --target wan-cpp 2>&1 \| tail -5` |
| **Full suite command** | Manual: build + run wan-cli T2V + I2V, verify AVI output |
| **Estimated runtime** | ~30 seconds (build only) |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build --target wan-cpp 2>&1 | tail -5`
- **After every plan wave:** Full build + manual CLI smoke test (T2V + I2V)
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** ~30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 9-01-01 | 01 | 1 | FIX-01 | build | `cmake --build build --target wan-cpp` | N/A | ⬜ pending |
| 9-01-02 | 01 | 1 | FIX-02 | build | `cmake --build build --target wan-cpp` | N/A | ⬜ pending |
| 9-02-01 | 02 | 1 | PERF-01 | build+size | `cmake --build build --target wan-cpp && ls -lh build/libwan-cpp.a` | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

None — Existing infrastructure covers all phase requirements. All validation is build + manual CLI execution. No test framework exists in this project.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| `wan_generate_video_t2v` returns WAN_SUCCESS | FIX-01 | No test harness | Run `wan-cli --mode t2v ...`; check exit code 0 and AVI output exists |
| `wan_generate_video_i2v` returns WAN_SUCCESS | FIX-01 | No test harness | Run `wan-cli --mode i2v ...`; check exit code 0 and AVI output exists |
| `progress_cb` called N times for N steps | FIX-02 | No test harness | Add debug printf to callback in CLI; verify N prints for N steps |
| `libwan-cpp.a` size reduced ~85MB | PERF-01 | Build metric | `ls -lh build/libwan-cpp.a` before and after; expect ~85MB reduction |
| Tokenization output identical to embedded path | PERF-01 | No regression test | Run T2V with same prompt; compare AVI frame checksums |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
