# Milestones

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

