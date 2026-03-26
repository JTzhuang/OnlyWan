# Milestones

## v1.2 性能优化与多卡推理 (Shipped: 2026-03-26)

**Phases completed:** 8 phases, 14 plans, 22 tasks

**Key accomplishments:**

- Two-branch format dispatch in WanModel::load: safetensors via init_from_file + get_sd_version, GGUF path unchanged, both converging at runner construction
- One-liner:
- One-liner:
- RoPE PE cached per-dimension to eliminate redundant CPU computation, inplace GELU documented for CUDA graph fusion
- One-liner:
- One-liner:
- One-liner:
- One-liner:
- One-liner:
- One-liner:

---

## v1.2 v1.2 (Shipped: 2026-03-18)

**Phases completed:** 7 phases, 13 plans, 14 tasks

**Key accomplishments:**

- (none recorded)

---

## v1.1 模型格式扩展 (Shipped: 2026-03-17)

**Phases completed:** 5 phases, 6 plans

**Key accomplishments:**

- Safetensors runtime loading — users can load .safetensors WAN models directly via `wan_load_model` without pre-conversion
- Standalone `wan-convert` CLI tool for safetensors → GGUF conversion with metadata injection
- API fixes — T2V/I2V flat API delegates to _ex implementations; progress_cb properly wired on each Euler step
- Vocab optimization — vocab files loaded via mmap at runtime, not embedded at compile time (~85MB reduction)
- Public API extension — `wan_set_vocab_dir()` exposed for runtime vocab directory configuration
- Documentation — sub-model conversion scope clearly documented with loadability annotations

**Stats:** 32 commits, 1 day (2026-03-16 → 2026-03-17)

**Requirements:** 6/6 satisfied (SAFE-01, SAFE-02, SAFE-03, FIX-01, FIX-02, PERF-01)

---

## v1.0 MVP (Shipped: 2026-03-16)

**Phases completed:** 8 phases, 11 plans, 6 tasks

**Key accomplishments:**

- Extracted WAN video generation core (wan.hpp, 109K lines) from stable-diffusion.cpp into standalone C++ library
- Created multi-platform CMake build system with CUDA/Metal/Vulkan/CPU backend support
- Implemented C-style public API (wan.h) with model loading, T2V, I2V, and config interfaces
- Integrated T5 text encoder and CLIP image encoder with full weight loading from GGUF
- Resolved duplicate symbol linker failures via explicit CMake source list and single-TU architecture
- Implemented complete Euler flow-matching denoising loop with CFG, latent normalization, VAE decode, and AVI output
- CLI example (wan-cli) supports both T2V and I2V generation with full argument parsing

**Stats:** 179 files changed, ~44,500 LOC, 63 commits, 4 days (2026-03-12 → 2026-03-16)

---
