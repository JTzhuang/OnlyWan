---
phase: 17
slug: clip-text-encoder-vae-transformer
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-27
---

# Phase 17 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | C++ custom assert-based framework (no external deps) |
| **Config file** | `tests/cpp/CMakeLists.txt` (Wave 0 creates) |
| **Quick run command** | `cmake --build build --target test_factory_basic && ./build/tests/cpp/test_factory_basic` |
| **Full suite command** | `cmake --build build --target run_all_tests && ctest --test-dir build -V` |
| **Estimated runtime** | ~10 seconds (no real weights needed) |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build --target test_factory_basic && ./build/tests/cpp/test_factory_basic`
- **After every plan wave:** Run `cmake --build build --target run_all_tests && ctest --test-dir build -V`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 17-01-01 | 01 | 1 | FACTORY | unit | `./build/tests/cpp/test_factory_basic` | ❌ W0 | ⬜ pending |
| 17-01-02 | 01 | 1 | CLIP | unit | `./build/tests/cpp/test_clip` | ❌ W0 | ⬜ pending |
| 17-01-03 | 01 | 2 | T5 | unit | `./build/tests/cpp/test_t5` | ❌ W0 | ⬜ pending |
| 17-01-04 | 01 | 2 | VAE | unit | `./build/tests/cpp/test_vae` | ❌ W0 | ⬜ pending |
| 17-01-05 | 01 | 2 | TRANSFORMER | unit | `./build/tests/cpp/test_flux` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/cpp/CMakeLists.txt` — 测试构建配置，定义 `wan_add_test()` 函数
- [ ] `tests/cpp/test_helpers.hpp` — 共享测试工具（T5Version枚举、断言宏、RandomTensorStorage辅助）
- [ ] `tests/cpp/model_factory.hpp` — 通用模板工厂核心实现
- [ ] `CMakeLists.txt` 修改 — 增加 `WAN_BUILD_TESTS` 选项和 `tests/cpp/` 子目录

*Wave 0 必须先建立基础设施，后续任务才能编译通过。*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| 真实模型权重推理 | D-14 | 需要实际模型文件(数GB) | 使用真实权重运行集成测试套件，验证输出数值范围 |
| GPU后端测试 | D-07 | 需要CUDA环境 | 在有GPU的机器上运行 `ctest -R gpu` |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
