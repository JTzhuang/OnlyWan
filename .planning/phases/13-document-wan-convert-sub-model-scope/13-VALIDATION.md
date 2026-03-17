---
phase: 13
slug: document-wan-convert-sub-model-scope
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-17
---

# Phase 13 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | grep + file existence checks (pure documentation phase) |
| **Config file** | none |
| **Quick run command** | `grep -n "loadable by wan_load_model" examples/convert/main.cpp` |
| **Full suite command** | `grep -n "loadable" examples/convert/main.cpp && test -f examples/convert/README.md && grep -n "SAFE-03" .planning/REQUIREMENTS.md` |
| **Estimated runtime** | ~2 seconds |

---

## Sampling Rate

- **After every task commit:** Run quick run command
- **After every plan wave:** Run full suite command
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 2 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 13-01-01 | 01 | 1 | SAFE-03 | grep | `grep -n "loadable by wan_load_model" examples/convert/main.cpp` | ✅ | ⬜ pending |
| 13-01-02 | 01 | 1 | SAFE-03 | file+grep | `test -f examples/convert/README.md && grep -n "reserved" examples/convert/README.md` | ❌ W0 | ⬜ pending |
| 13-01-03 | 01 | 1 | SAFE-03 | grep | `grep -n "SAFE-03\|partial\|dit-\*\|reserved" .planning/REQUIREMENTS.md` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `examples/convert/README.md` — new file, created in Task 2

*All other verification uses existing files.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| wan-convert --help output readable and clear | SAFE-03 | Requires compiled binary | `./build/bin/wan-convert --help` and visually confirm annotations appear next to each --type value |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 2s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
