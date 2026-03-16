---
phase: 7
slug: wire-core-model
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-16
---

# Phase 7 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CMake build verification (no unit test framework present) |
| **Config file** | CMakeLists.txt |
| **Quick run command** | `cmake --build /home/jtzhuang/projects/stable-diffusion.cpp/wan/build 2>&1 \| tail -10` |
| **Full suite command** | `cmake --build /home/jtzhuang/projects/stable-diffusion.cpp/wan/build 2>&1 \| grep -E "error:|undefined reference"` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build /home/jtzhuang/projects/stable-diffusion.cpp/wan/build 2>&1 | tail -10`
- **After every plan wave:** Run full build + link check (zero undefined references)
- **Before `/gsd:verify-work`:** Full suite must be green (zero errors, zero undefined references)
- **Max feedback latency:** ~30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 7-01-01 | 01 | 0 | CORE-01, CORE-04 | build | `cmake --build build 2>&1 \| grep -c error` | ❌ W0 | ⬜ pending |
| 7-01-02 | 01 | 1 | CORE-01, API-02 | build | `cmake --build build 2>&1 \| grep -c error` | ❌ W0 | ⬜ pending |
| 7-01-03 | 01 | 1 | API-02 | build+grep | `grep -n "load_tensors" src/api/wan_loader.cpp` | ❌ W0 | ⬜ pending |
| 7-01-04 | 01 | 2 | ENCODER-01 | build+grep | `grep -n "WanRunner::compute\|wan_runner->compute" src/api/wan_t2v.cpp` | ❌ W0 | ⬜ pending |
| 7-01-05 | 01 | 2 | ENCODER-02, CORE-02 | build+grep | `grep -n "CLIPVisionModelProjection\|clip_fea" src/api/wan_i2v.cpp` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `cmake -B /home/jtzhuang/projects/stable-diffusion.cpp/wan/build` — configure build directory if not present
- [ ] Verify build system compiles before any code changes: `cmake --build build 2>&1 | tail -5`

*No unit test framework — build verification is the primary gate for this phase.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Model weights actually loaded (not just metadata) | API-02 | Requires real WAN GGUF model file | Run wan-cli with a real model file; verify non-zero tensor count in logs |
| T5 context tensor has correct shape [1, 512, 4096] | ENCODER-01 | Requires real model + prompt | Add debug log of context->ne[0..2] in wan_t2v.cpp and verify output |
| CLIP clip_fea tensor has correct shape [N, 257, 1280] | ENCODER-02 | Requires real model + image | Add debug log of clip_fea->ne[0..2] in wan_i2v.cpp and verify output |
| GGUF tensor prefix detection works for T5/CLIP | API-02 | Requires real WAN GGUF file | Scan tensor names and verify T5/CLIP prefix detected correctly |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
