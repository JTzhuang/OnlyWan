# Codebase Concerns

**Analysis Date:** 2026-03-28

## Model Registration & Factory Pattern

**Issue: Implicit Model Dependencies**
- Files: `src/model_factory.cpp`, `src/ggml_extend.hpp`, `src/wan.hpp`
- Problem: Model registration uses lambda closures capturing backend, offload flags, and tensor_storage_map. No explicit dependency graph or initialization order validation. If a downstream model (e.g., WAN DiT) is instantiated before its encoder dependencies (CLIP/T5), runtime failures occur silently.
- Impact: Hard to debug model pipeline failures. No compile-time or runtime checks for required encoder availability.
- Fix approach: Add model dependency registry that validates encoder availability before VAE/DiT instantiation. Implement dependency resolution in factory.

**Issue: Tensor Storage Map Lifecycle**
- Files: `src/model_factory.cpp` (lines 15-79), `src/ggml_extend.hpp` (GGMLRunner constructor)
- Problem: `tensor_storage_map` is passed as const reference to factory lambda but its lifetime is not guaranteed. If map is destroyed before model inference, tensor lookups fail.
- Impact: Potential use-after-free in long-running applications where models are created/destroyed dynamically.
- Fix approach: Store tensor_storage_map as shared_ptr in GGMLRunner. Add lifetime validation in model initialization.

## Memory Management Architecture

**Issue: Three-Tier Context Lifecycle Management**
- Files: `src/ggml_extend.hpp` (lines 180-250: alloc_params_ctx, alloc_compute_ctx, alloc_cache_ctx)
- Problem: Three separate ggml_context allocations (params_ctx, compute_ctx, cache_ctx) with independent buffer management. No unified lifecycle tracking. If compute_ctx allocation fails mid-pipeline, params_ctx and cache_ctx remain allocated, causing memory leaks.
- Impact: Memory fragmentation in multi-model scenarios. Difficult to track total memory usage across all three contexts.
- Fix approach: Implement RAII wrapper (ModelContextManager) that manages all three contexts as single unit. Add rollback on partial allocation failure.

**Issue: Buffer Allocation Overhead**
- Files: `src/ggml_extend.hpp` (lines 200-220: alloc_compute_ctx with ggml_allocr_new_measure)
- Problem: Compute context uses two-pass allocation (measure pass + actual allocation). For models with dynamic graph sizes, this doubles allocation overhead. No caching of measured sizes.
- Impact: Slow model initialization, especially for large models (14B WAN DiT with 40 layers).
- Fix approach: Cache measured allocation sizes per model variant. Implement lazy allocation for cache_ctx (allocate only when needed).

**Issue: Backend Abstraction Complexity**
- Files: `src/model_factory.cpp` (backend parameter in all 12 registrations), `src/ggml_extend.hpp` (ggml_backend_t usage)
- Problem: Backend selection (CPU/GPU/multi-GPU) is hardcoded at model registration time. No runtime backend switching or fallback mechanism if GPU memory exhausted.
- Impact: Cannot dynamically offload models to CPU if GPU OOM occurs. Multi-GPU scheduling not exposed to caller.
- Fix approach: Add backend negotiation layer. Implement fallback chain: GPU → CPU. Expose multi-GPU scheduling hints.

## Model Call Flow & Dependencies

**CLIP/T5 Encoder Pipeline:**
```
1. CLIP/T5 model instantiated via factory
2. GGMLRunner::compute() called with input tokens
3. Encoder forward pass: embedding → transformer layers → output
4. Output cached for VAE encoder input
```
- Files: `src/model_factory.cpp` (lines 20-35: CLIP/T5 registrations)
- Dependency: None (standalone encoders)
- Memory: params_ctx (model weights) + compute_ctx (intermediate activations)

**VAE Encode Pipeline:**
```
1. VAE encoder instantiated (wan-vae-t2v or wan-vae-i2v)
2. Input: image/video frames (B, C, H, W)
3. Encoder3d forward: Conv3d → ResidualBlocks → AttentionBlocks → output
4. Output: latent codes (B, D, H', W')
```
- Files: `src/wan.hpp` (lines 1200-1400: WanVAE::Encoder3d)
- Dependency: None (standalone VAE)
- Memory: params_ctx (VAE weights) + compute_ctx (conv/attention intermediates)
- Issue: Conv2d with random weights crashes (see test_vae.cpp workaround using decode-only)

**VAE Decode Pipeline:**
```
1. VAE decoder instantiated (wan-vae-t2v-decode)
2. Input: latent codes from DiT output
3. Decoder3d forward: ResidualBlocks → AttentionBlocks → Conv3d → output
4. Output: reconstructed frames
```
- Files: `src/wan.hpp` (lines 1400-1600: WanVAE::Decoder3d)
- Dependency: None (standalone decoder)
- Memory: params_ctx (VAE weights) + compute_ctx (deconv/attention intermediates)

**WAN DiT (Diffusion Transformer) Pipeline:**
```
1. DiT instantiated (wan-t2v, wan-i2v, or wan-ti2v)
2. Input: latent codes + timestep + text/image embeddings
3. Forward pass:
   a. Timestep embedding (ggml_timestep_embedding)
   b. Latent projection
   c. 30-40 transformer layers with cross-attention
   d. Output projection
4. Output: denoised latents
```
- Files: `src/wan.hpp` (lines 1700-2100: WanRunner)
- Dependencies: CLIP encoder (text embeddings for T2V/TI2V), image encoder (image embeddings for I2V/TI2V)
- Memory: params_ctx (DiT weights) + compute_ctx (attention/FFN intermediates) + cache_ctx (KV cache for cross-attention)
- Issue: Timestep embedding requires F32 input (see ggml_extend.hpp line 850: cast I32 to F32)

**Full T2V Pipeline:**
```
1. Text → CLIP encoder → text embeddings
2. Latent codes → WAN DiT (with text embeddings) → denoised latents
3. Denoised latents → VAE decoder → video frames
```
- Files: `src/model_factory.cpp` (lines 20-79), `src/wan.hpp`
- Dependency chain: CLIP → DiT → VAE decoder
- Memory: 3 × (params_ctx + compute_ctx) + 1 × cache_ctx (DiT only)
- Issue: No explicit ordering enforcement. Caller must instantiate in correct order.

## Identified Issues

**Issue: Positional Encoding Caching (OP-02)**
- Files: `src/wan.hpp` (lines 1750-1800: positional encoding computation)
- Problem: Positional encodings recomputed for every forward pass. For fixed sequence lengths, this is wasteful.
- Impact: ~5-10% overhead per forward pass in DiT.
- Fix approach: Cache PE tensors keyed by (seq_len, dim). Reuse across batches.

**Issue: Graph Structure Stability (CG-01)**
- Files: `src/ggml_extend.hpp` (lines 300-350: compute graph building)
- Problem: Compute graph structure may vary based on input shapes or dynamic conditions. No validation that graph structure is stable across calls.
- Impact: Potential memory corruption if graph structure changes mid-inference.
- Fix approach: Add graph structure fingerprinting. Validate consistency across calls.

**Issue: FFN Fusion Opportunity (FUS-02)**
- Files: `src/wan.hpp` (lines 1850-1900: FFN blocks in transformer layers)
- Problem: FFN implemented as separate matmul + activation + matmul operations. No kernel fusion.
- Impact: Memory bandwidth bottleneck, especially for 14B model.
- Fix approach: Implement fused FFN kernel or use ggml_mul_mat_id for dynamic routing.

**Issue: Multi-GPU Scheduling**
- Files: `src/model_factory.cpp` (backend parameter), `src/ggml_extend.hpp` (ggml_backend_t)
- Problem: No explicit multi-GPU load balancing. All models run on single backend.
- Impact: Cannot distribute CLIP + DiT + VAE across multiple GPUs.
- Fix approach: Add backend affinity hints. Implement model-to-GPU assignment strategy.

**Issue: Context Reuse Across Models**
- Files: `src/ggml_extend.hpp` (alloc_compute_ctx, alloc_cache_ctx)
- Problem: Each model allocates separate compute_ctx and cache_ctx. No sharing between models.
- Impact: Memory overhead when running multiple models sequentially (e.g., CLIP → DiT → VAE).
- Fix approach: Implement context pool. Reuse compute_ctx for sequential models with compatible shapes.

## Scaling Limits

**Memory Scaling:**
- Current: 14B DiT with 40 layers requires ~256MB compute_ctx (see test_transformer.cpp)
- Limit: GPU memory exhaustion at ~8B+ models on consumer GPUs (8GB VRAM)
- Scaling path: Implement gradient checkpointing (recompute activations instead of storing). Use activation quantization.

**Inference Latency:**
- Current: Single forward pass ~100-500ms depending on model size
- Limit: Real-time applications need <50ms per frame
- Scaling path: Implement model parallelism. Use tensor parallelism for 14B model across multiple GPUs.

## Test Coverage Gaps

**Untested Areas:**
- Files: `tests/cpp/test_*.cpp`
- What's not tested:
  - Multi-model pipeline (CLIP → DiT → VAE) end-to-end
  - Backend fallback (GPU OOM → CPU)
  - Concurrent model inference
  - Long-running inference (memory leak detection)
- Risk: Pipeline integration bugs, memory leaks in production
- Priority: High

**Fragile Areas:**
- Files: `src/wan.hpp` (WanCrossAttention variants for T2V/I2V/TI2V)
- Why fragile: Three separate attention implementations with subtle differences in embedding handling
- Safe modification: Add attention variant factory. Consolidate common logic.
- Test coverage: Only basic forward pass tested, no gradient/backprop

---

*Concerns audit: 2026-03-28*
