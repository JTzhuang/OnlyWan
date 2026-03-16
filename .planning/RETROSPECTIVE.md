# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.0 — MVP

**Shipped:** 2026-03-16
**Phases:** 8 | **Plans:** 11 | **Sessions:** ~8

### What Was Built
- Standalone C++ library extracting WAN video generation from stable-diffusion.cpp monorepo
- Complete T2V and I2V generation pipeline: Euler flow-matching loop, CFG, T5/CLIP encoders, VAE decode, AVI output
- C-style public API (wan.h) with model loading, generation, and config interfaces
- Multi-platform CMake build system with CUDA/Metal/Vulkan/CPU backend support
- CLI example (wan-cli) supporting both T2V and I2V modes

### What Worked
- **Single-TU architecture decision (Phase 6/7):** Moving all runner headers into wan-api.cpp as the sole owner cleanly resolved ODR violations that would have been painful to debug incrementally
- **Explicit CMake source list:** Replacing GLOB_RECURSE with an explicit list immediately fixed duplicate symbol linker failures — simple, reliable fix
- **Phase 8 verification 7/7:** The denoising pipeline implementation was clean on first pass — following stable-diffusion.cpp reference exactly paid off
- **Audit-driven gap closure:** The v1.0 milestone audit (Phases 6-8) correctly identified all critical gaps from the early phases and drove targeted fixes

### What Was Inefficient
- **Phases 1-5 lacked VERIFICATION.md:** Early phases had no formal verification, requiring the milestone audit to retroactively assess them. Running gsd-verifier after each phase would have caught integration gaps earlier
- **Stale REQUIREMENTS.md checkboxes:** BUILD-01 and API-05 remained `[ ]` after Phase 6 completed them — traceability table not updated atomically with phase completion
- **Researcher agent write failures:** The Phase 8 researcher agent completed research but failed to write RESEARCH.md, requiring manual intervention. Retry logic or fallback to orchestrator write would save time
- **Milestone audit was stale at completion time:** The audit ran before Phases 6-8 existed, requiring a full re-audit at milestone completion. Running audit as part of execute-phase would keep it current
- **WanBackend dead weight:** A 256MB ggml backend buffer allocated in wan_load_model but never used in generation — caught only at integration check, not during phase execution

### Patterns Established
- **Single-TU ODR pattern:** When multiple headers have non-inline definitions, designate one .cpp file as the sole owner. Forward-declare in internal headers. Document the constraint explicitly.
- **Explicit CMake source lists:** Never use GLOB_RECURSE in a project where source files may exist at multiple directory depths. Always enumerate sources explicitly.
- **Follow reference implementation exactly:** For algorithm-heavy code (samplers, latent normalization), copying constants and patterns from the reference (stable-diffusion.cpp) verbatim avoids subtle numerical bugs.
- **Audit before milestone completion:** Run `/gsd:audit-milestone` after the last phase executes, not before gap-closure phases are planned.

### Key Lessons
1. **Verify integration at each phase, not just at milestone.** Phases 1-5 had no VERIFICATION.md. The milestone audit found 12 partial requirements — all of which were actually fixed by Phases 6-8, but the lack of per-phase verification made the audit harder to interpret.
2. **ODR violations are silent until link time.** The duplicate symbol failures (Phase 6) were introduced in Phase 3 and only surfaced at link time. Compile-check each TU independently during development.
3. **Dead declarations accumulate.** WanImage/WanVideo methods declared but never implemented, legacy stubs permanently returning UNSUPPORTED — these are low-cost to add but create confusion for future consumers. Delete or implement at creation time.
4. **progress_cb wiring is not the same as progress_cb invocation.** The callback was wired through the API correctly but never called inside the generation loop. Test the full signal path, not just the plumbing.

### Cost Observations
- Model mix: ~100% sonnet (all agents)
- Sessions: ~8 sessions across 4 days
- Notable: Phase 7 (Wire Core Model) was the most complex — 3 plans, multiple ODR-driven architectural deviations from the original plan. The executor correctly identified and fixed the ODR issues autonomously.

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Sessions | Phases | Key Change |
|-----------|----------|--------|------------|
| v1.0 | ~8 | 8 | Initial project — established single-TU ODR pattern, explicit CMake source lists |

### Cumulative Quality

| Milestone | VERIFICATION.md | Coverage | Notes |
|-----------|----------------|----------|-------|
| v1.0 | 3/8 phases | Phases 6-8 only | Phases 1-5 unverified; retroactive audit covered gaps |

### Top Lessons (Verified Across Milestones)

1. Run gsd-verifier after every phase, not just at milestone audit time.
2. Explicit CMake source lists prevent an entire class of duplicate symbol bugs.
