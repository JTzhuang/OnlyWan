---
phase: 12
slug: wire-vocab-dir-to-public-api
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-17
---

# Phase 12 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual build + CLI smoke test (no unit test framework in project) |
| **Config file** | `CMakeLists.txt` |
| **Quick run command** | `cmake --build . --target wan-cpp 2>&1 | tail -5` |
| **Full suite command** | `cmake --build . && ./examples/cli/wan-cli --help` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build . --target wan-cpp 2>&1 | tail -5`
- **After every plan wave:** Run `cmake --build . && ./examples/cli/wan-cli --help`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 12-01-01 | 01 | 1 | API-03, API-04 | build | `grep -n "wan_set_vocab_dir" include/wan-cpp/wan.h` | ✅ | ⬜ pending |
| 12-01-02 | 01 | 1 | PERF-01 | build | `grep -n "wan_set_vocab_dir" src/api/wan-api.cpp` | ✅ | ⬜ pending |
| 12-01-03 | 01 | 1 | PERF-01 | build | `grep -n "wan_vocab_dir_is_set\|wan_vocab_get_dir" src/vocab/vocab.h` | ✅ | ⬜ pending |
| 12-01-04 | 01 | 2 | ENCODER-01, ENCODER-02 | build+run | `cmake --build . && ./examples/cli/wan-cli --help \| grep vocab-dir` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements — no new test files needed. All verification is via `grep` on source files and `cmake --build` compilation checks.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| WAN_EMBED_VOCAB=OFF + --vocab-dir 生成成功 | PERF-01 | 需要真实模型文件和词汇表文件 | `cmake -DWAN_EMBED_VOCAB=OFF .. && make && ./wan-cli --model m.gguf --vocab-dir ./vocab --prompt "test" -o out.avi` |
| WAN_EMBED_VOCAB=ON 时 wan_set_vocab_dir 返回警告码 | API-03 | 需要运行时验证返回值 | 编写小型测试程序调用 wan_set_vocab_dir 并检查返回值 |
| WAN_EMBED_VOCAB=OFF 未提供 --vocab-dir 时打印警告 | PERF-01 | 需要观察 stderr 输出 | `./wan-cli --model m.gguf --prompt "test" -o out.avi 2>&1 \| grep -i warn` |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
