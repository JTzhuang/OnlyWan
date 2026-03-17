# WAN-CPP Optimization TODO List

**Generated:** 2026-03-17
**Project:** wan-cpp (WAN video generation inference engine)
**Purpose:** Structured catalog of optimization opportunities across all sub-models

This document identifies concrete performance improvement opportunities across four dimensions:
1. CUDA Graph optimization
2. Operator implementation efficiency
3. Third-party library usage
4. Operator fusion opportunities

---

## Quick Wins

Top 5 items offering the best effort-to-impact ratio:

| ID | Description | Expected Gain | Difficulty |
|----|-------------|---------------|------------|
| CG-01 | Persist compute buffer across denoising steps | 2-5x for denoising loop | LOW |
| OP-01 | Enable flash attention by default for CUDA backend | 10-20% attention speedup | LOW |
| OP-02 | Move RoPE PE generation to GPU | 5-10% per-step speedup | MEDIUM |
| FUS-02 | Fuse Linear + GELU in FFN blocks | 5-10% FFN speedup | MEDIUM |
| CG-02 | Enable GGML_CUDA_USE_GRAPHS compile flag | 10-30% graph execution | LOW |

---

## 1. CUDA Graph Optimization

CUDA graphs allow capturing and replaying GPU command sequences, eliminating CPU overhead for graph construction and kernel launch.

| ID | Sub-model | Description | Priority | Expected Gain | Difficulty | Code Location |
|----|-----------|-------------|----------|---------------|------------|---------------|
| CG-01 | WAN DiT | **Persist compute buffer across denoising steps** — Currently `free_compute_buffer_immediately=true` causes 40 alloc/free cycles per 20-step generation (2x per step for CFG). Graph structure is stable across steps (same latent shape). | HIGH | 2-5x for denoising loop | LOW | `src/ggml_extend.hpp:2058-2105` (GGMLRunner::compute)<br>`src/api/wan-api.cpp:547-595` (Euler loop) |
| CG-02 | All | **Enable GGML_CUDA_USE_GRAPHS compile flag** — GGML already has CUDA graph capture/replay support via `GGML_CUDA_USE_GRAPHS`. Currently defeated by buffer reallocation. | HIGH | 10-30% graph execution | LOW | `ggml/src/ggml-cuda/ggml-cuda.cu:3918+` (CUDA graph support)<br>CMakeLists.txt (add compile definition) |
| CG-03 | T5/CLIP | **Stable graph structure detection for single-shot inference** — T5 and CLIP encoders run once per generation with fixed input shapes. Can cache graph after first run. | MEDIUM | 20-40% encoder speedup | MEDIUM | `src/t5.hpp` (T5 forward)<br>`src/clip.hpp` (CLIP forward) |
| CG-04 | VAE | **VAE partial decode graph caching** — VAE decoder runs once per generation. Graph structure is stable for fixed output resolution. | MEDIUM | 15-25% VAE speedup | MEDIUM | `src/vae.hpp:699-711` (AutoEncoderKL::build_graph) |
| CG-05 | WAN DiT | **Separate cond/uncond graph capture** — CFG requires 2 forward passes per step. Can capture separate graphs for conditional and unconditional paths. | LOW | 5-10% per-step speedup | HIGH | `src/api/wan-api.cpp:561-571` (CFG loop) |

---

## 2. Operator Implementation Efficiency

Optimizations targeting specific operator implementations and algorithmic choices.

| ID | Sub-model | Description | Priority | Expected Gain | Difficulty | Code Location |
|----|-----------|-------------|----------|---------------|------------|---------------|
| OP-01 | All | **Enable flash attention by default when CUDA backend detected** — Flash attention is opt-in via `flash_attn_enabled` flag. Should auto-enable for CUDA/Metal backends. | HIGH | 10-20% attention speedup | LOW | `src/ggml_extend.hpp:2107-2109` (set_flash_attention_enabled)<br>`src/vae.hpp:143` (ggml_ext_attention_ext call)<br>`src/wan.hpp` (WanSelfAttention forward) |
| OP-02 | WAN DiT / Flux | **Move RoPE PE generation to GPU** — `gen_wan_pe` and `gen_flux_pe` compute positional embeddings on CPU, then upload via `set_backend_tensor_data`. Should compute directly on GPU. | HIGH | 5-10% per-step speedup | MEDIUM | `src/rope.hpp:285-349` (gen_flux_pe)<br>`src/rope.hpp:450+` (gen_wan_pe)<br>`src/ggml_extend.hpp:2026-2028` (set_backend_tensor_data) |
| OP-03 | WAN DiT / Flux | **Fused QKV projection in attention blocks** — Q/K/V projections are separate Linear layers. Can fuse into single GEMM with 3x output channels. | MEDIUM | 10-15% attention speedup | MEDIUM | `src/wan.hpp:1300-1350` (WanSelfAttention)<br>`src/flux.hpp:85-100` (SelfAttention) |
| OP-04 | All | **Use ggml_mul_mat_set_prec selectively** — GGML supports reduced precision for GEMM. Can use FP16 accumulation for non-critical layers. | MEDIUM | 15-25% GEMM speedup | MEDIUM | `src/ggml_extend.hpp` (Linear forward)<br>GGML documentation for ggml_mul_mat_set_prec |
| OP-05 | WAN DiT | **Optimize modulation pattern** — Each WanAttentionBlock has 6 separate modulation ops (add/mul). Can fuse into fewer kernels. | MEDIUM | 5-10% per-block speedup | HIGH | `src/wan.hpp:1533-1581` (WanAttentionBlock::forward)<br>`src/wan.hpp:1558-1578` (modulate_add/mul calls) |
| OP-06 | VAE | **Conv3d optimization for video decoder** — AE3DConv and VideoResnetBlock use Conv3d for temporal mixing. Check if using efficient implementation. | LOW | 10-20% VAE video speedup | HIGH | `src/vae.hpp:162-207` (AE3DConv)<br>`src/vae.hpp:209-263` (VideoResnetBlock) |
| OP-07 | T5 | **Optimize tokenization path** — T5UniGramTokenizer uses Darts trie for tokenization. Consider caching tokenization results. | LOW | 5-10% T5 encode speedup | LOW | `src/t5.hpp:58-300` (T5UniGramTokenizer) |
| OP-08 | All | **Quantization-aware operator selection** — When using quantized weights (Q4_0, Q8_0), some operators have specialized fast paths. | LOW | 10-30% quantized inference | MEDIUM | `src/ggml_extend.hpp` (GGMLBlock weight loading) |

---

## 3. Third-Party Library Usage

Opportunities to leverage optimized third-party libraries for specific operations.

| ID | Sub-model | Description | Priority | Expected Gain | Difficulty | Code Location |
|----|-----------|-------------|----------|---------------|------------|---------------|
| LIB-01 | All | **Verify cuBLAS is active for GEMM operations** — GGML should use cuBLAS for matrix multiplication on CUDA backend. Verify no fallback to naive implementation. | HIGH | 2-5x GEMM speedup | LOW | GGML backend initialization<br>Runtime logging/profiling |
| LIB-02 | VAE / WAN DiT | **cuDNN for Conv2d/Conv3d operations** — Currently using `ggml_conv_2d` and `ggml_ext_conv_3d`. cuDNN provides highly optimized convolution kernels. | MEDIUM | 20-40% convolution speedup | HIGH | `src/common_block.hpp` (Conv2d/Conv3d)<br>`src/vae.hpp` (patch_embedding Conv3d)<br>`src/wan.hpp` (CausalConv3d) |
| LIB-03 | All | **Evaluate TensorRT for static DiT graph compilation** — Entire DiT forward pass has static graph structure. TensorRT can compile to optimized engine. | LOW | 30-50% end-to-end speedup | HIGH | Entire inference pipeline<br>Requires TensorRT integration layer |
| LIB-04 | All | **FP8 compute for attention (Hopper+ GPUs)** — NVIDIA Hopper (H100+) supports FP8 tensor cores. Can use for attention QK^T and softmax. | LOW | 20-40% attention speedup | HIGH | Attention implementations<br>Requires CUDA 12+ and Hopper GPU |
| LIB-05 | T5 / CLIP | **Optimized tokenizer libraries** — Consider using HuggingFace tokenizers (Rust-based) for faster tokenization. | LOW | 50-100% tokenization speedup | MEDIUM | `src/t5.hpp` (T5UniGramTokenizer)<br>`src/clip.hpp` (CLIPTokenizer) |

---

## 4. Operator Fusion Opportunities

Opportunities to fuse multiple operations into single kernels, reducing memory bandwidth and kernel launch overhead.

| ID | Sub-model | Description | Priority | Expected Gain | Difficulty | Code Location |
|----|-----------|-------------|----------|---------------|------------|---------------|
| FUS-01 | WAN DiT / Flux | **LayerNorm + modulate fusion** — LayerNorm followed by modulate_add + modulate_mul (3 ops) can be fused into single kernel. | HIGH | 10-15% per-block speedup | HIGH | `src/wan.hpp:1557-1559` (norm1 + modulate)<br>`src/wan.hpp:1570-1572` (norm2 + modulate)<br>`src/flux.hpp` (similar patterns) |
| FUS-02 | All | **Linear + GELU fusion in FFN** — FFN pattern is Linear -> GELU -> Linear. First Linear + GELU can be fused. | HIGH | 5-10% FFN speedup | MEDIUM | `src/wan.hpp:1574-1575` (ffn_0 + gelu)<br>`src/flux.hpp:29-30` (MLPEmbedder)<br>`src/common_block.hpp` (FFN blocks) |
| FUS-03 | All | **Linear + SiLU fusion in embeddings** — Time/text embedding uses Linear + SiLU pattern. Can fuse activation into GEMM. | MEDIUM | 5-10% embedding speedup | MEDIUM | `src/flux.hpp:28-30` (MLPEmbedder)<br>`src/vae.hpp:42-43` (ResnetBlock) |
| FUS-04 | WAN DiT | **Residual connection + scale fusion** — Pattern `x = x + scale * y` appears frequently. Can fuse add + mul. | MEDIUM | 3-5% per-block speedup | LOW | `src/wan.hpp:1562` (residual + modulate_mul)<br>`src/wan.hpp:1578` (residual + modulate_mul) |
| FUS-05 | WAN DiT / Flux | **Patchify + reshape chain optimization** — Patch embedding involves Conv3d + multiple reshape/permute ops. Can optimize memory layout. | LOW | 5-10% patchify speedup | HIGH | `src/wan.hpp` (patch embedding)<br>`src/flux.hpp` (patch embedding) |
| FUS-06 | VAE | **GroupNorm + SiLU fusion** — VAE decoder uses GroupNorm32 + SiLU pattern. Can fuse normalization + activation. | LOW | 5-10% VAE speedup | MEDIUM | `src/vae.hpp:41-42` (ResnetBlock)<br>`src/vae.hpp:355-356` (Encoder) |
| FUS-07 | All | **Attention QK^T + softmax + dropout fusion** — Standard attention pattern can be fused into single kernel (similar to flash attention). | LOW | 15-25% attention speedup | HIGH | Already partially addressed by flash attention<br>Can optimize further for specific hardware |

---

## Implementation Notes

### CUDA Graph Prerequisites
- Requires stable graph structure (same shapes, same ops)
- Requires persistent compute buffer (no reallocation between captures)
- Requires GGML_CUDA_USE_GRAPHS compile flag
- May require graph warmup run before capture

### Flash Attention Requirements
- CUDA compute capability 7.0+ (Volta+)
- Metal Performance Shaders on Apple Silicon
- Requires contiguous Q/K/V tensors
- May have sequence length limitations

### Operator Fusion Challenges
- Requires custom CUDA kernel development
- May need to maintain multiple kernel variants for different shapes
- Fusion may reduce numerical precision (need validation)
- May conflict with CUDA graph capture (fused kernels change graph structure)

### Third-Party Library Integration
- cuDNN requires CUDA 11.0+
- TensorRT requires static graph structure and shape inference
- FP8 requires Hopper architecture (H100+) and CUDA 12+
- May increase binary size and dependency complexity

---

## Measurement Methodology

To validate expected gains:

1. **Baseline profiling**: Use NVIDIA Nsight Systems to profile current implementation
2. **Hotspot identification**: Identify top 10 time-consuming operations
3. **Isolated benchmarks**: Test each optimization in isolation with synthetic workloads
4. **End-to-end validation**: Measure impact on full generation pipeline
5. **Regression testing**: Ensure numerical accuracy within tolerance (< 0.01% error)

### Key Metrics
- **Denoising step latency** (ms per step)
- **Encoder latency** (ms for T5/CLIP)
- **VAE decode latency** (ms for video decode)
- **Memory bandwidth utilization** (GB/s)
- **GPU compute utilization** (%)
- **End-to-end generation time** (seconds for 20-step, 512x512x16 video)

---

## Priority Matrix

| Priority | Criteria |
|----------|----------|
| HIGH | Expected gain > 10% AND difficulty <= MEDIUM |
| MEDIUM | Expected gain > 5% OR difficulty = MEDIUM |
| LOW | Expected gain < 5% OR difficulty = HIGH |

---

**Document Status:** Initial analysis complete
**Next Steps:** Prioritize Quick Wins, create implementation plans for HIGH priority items
**Owner:** Performance optimization team
**Last Updated:** 2026-03-17

