---
phase: 8
slug: implement-generation
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-16
---

# Phase 8 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CMake + make (C++ build verification) |
| **Config file** | CMakeLists.txt |
| **Quick run command** | `cmake --build . --target wan-cpp 2>&1 | tail -5` |
| **Full suite command** | `cmake --build . 2>&1 | tail -20` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build . --target wan-cpp 2>&1 | tail -5`
- **After every plan wave:** Run `cmake --build . 2>&1 | tail -20`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 8-01-01 | 01 | 1 | EX-02 | build | `cmake --build . 2>&1 \| grep -c error` | ✅ | ⬜ pending |
| 8-01-02 | 01 | 1 | API-03 | build | `cmake --build . 2>&1 \| grep -c error` | ✅ | ⬜ pending |
| 8-01-03 | 01 | 1 | API-04 | build | `cmake --build . 2>&1 \| grep -c error` | ✅ | ⬜ pending |
| 8-01-04 | 01 | 2 | API-03,API-04 | manual | Run CLI T2V, check AVI file size > 0 | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. No new test files needed — verification is build-based and manual CLI execution.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| T2V produces valid AVI | API-03, EX-02 | Requires model weights at runtime | `./wan-cli -m model.gguf -p "a cat" -o out.avi` → check `out.avi` size > 1KB and opens in video player |
| I2V produces valid AVI | API-04, EX-02 | Requires model weights + input image | `./wan-cli -m model.gguf -i frame.jpg -p "make it move" -o out.avi` → check `out.avi` size > 1KB |
| wan_load_image loads PNG/JPG | API-04 | Requires image file at runtime | Verify `wan_load_image` returns `WAN_SUCCESS` and non-null image with correct width/height |
| AVI frame count correct | EX-02 | Requires video player inspection | Frame count = `((num_frames-1)/4*4)+1` due to WAN temporal expansion |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
