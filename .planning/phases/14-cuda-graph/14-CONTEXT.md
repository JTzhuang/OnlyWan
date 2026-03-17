# Phase 14: 性能优化 - CUDA Graph 和算子融合 - Context

**Gathered:** 2026-03-17
**Status:** Ready for planning

<domain>
## Phase Boundary

实现 WAN-CPP 推理引擎的性能优化，重点关注 5 个 Quick Wins：
1. CUDA Graph 优化（缓冲区持久化、编译标志启用）
2. 算子效率优化（Flash Attention、RoPE GPU 化）
3. 算子融合（Linear + GELU、LayerNorm + modulate）

目标：通过这些优化实现 2-5x 的去噪循环加速和 10-20% 的整体推理加速。

</domain>

<decisions>
## Implementation Decisions

### 优化优先级和范围
- **第一阶段（Phase 14）**：实现 5 个 Quick Wins（CG-01、OP-01、OP-02、FUS-02、CG-02）
- **第二阶段（未来）**：实现中等优先级优化（CG-03、CG-04、OP-03、OP-04 等）
- **第三阶段（未来）**：评估第三方库集成（cuDNN、TensorRT）

### CUDA Graph 优化策略
- **CG-01（缓冲区持久化）**：修改 `free_compute_buffer_immediately` 标志，在去噪循环中保持缓冲区分配
  - 当前：每 20 步生成有 40 次分配/释放（CFG 每步 2 次）
  - 目标：单次分配，跨所有步重用
  - 预期收益：2-5x 去噪循环加速

- **CG-02（GGML CUDA Graph 编译标志）**：启用 `GGML_CUDA_USE_GRAPHS` 编译定义
  - 在 CMakeLists.txt 中添加编译标志
  - 预期收益：10-30% 图执行加速
  - 依赖：CG-01 完成（缓冲区持久化）

### 算子效率优化策略
- **OP-01（Flash Attention）**：为 CUDA/Metal 后端自动启用 Flash Attention
  - 当前：通过 `flash_attn_enabled` 标志手动启用
  - 目标：在后端初始化时自动检测并启用
  - 预期收益：10-20% attention 加速
  - 影响范围：所有包含 attention 的子模型（WAN DiT、Flux、VAE）

- **OP-02（RoPE PE GPU 化）**：将位置编码计算从 CPU 移到 GPU
  - 当前：`gen_wan_pe` 和 `gen_flux_pe` 在 CPU 计算，然后上传 GPU
  - 目标：直接在 GPU 上计算位置编码
  - 预期收益：5-10% 每步加速
  - 实现方式：使用 GGML 的 GPU 计算能力

### 算子融合策略
- **FUS-02（Linear + GELU 融合）**：融合 FFN 块中的 Linear 和 GELU 操作
  - 当前：3 个独立操作（Linear -> GELU -> Linear）
  - 目标：第一个 Linear + GELU 融合为单个内核
  - 预期收益：5-10% FFN 加速
  - 影响范围：WAN DiT、Flux DiT 的所有 FFN 块

### 测试和验证策略
- **基准测试**：在优化前后测量关键指标
  - 去噪步延迟（ms/step）
  - 编码器延迟（T5/CLIP）
  - VAE 解码延迟
  - 端到端生成时间（20 步，512x512x16 视频）

- **数值精度验证**：确保优化不影响生成质量
  - 允许误差范围：< 0.01%
  - 对比优化前后的生成结果

- **回归测试**：确保优化不破坏现有功能
  - 所有支持的后端（CUDA、Metal、Vulkan 等）
  - 所有支持的模型（WAN、Flux、SD 等）

### 实现顺序
1. **CG-01**：缓冲区持久化（基础，其他优化依赖）
2. **OP-01**：Flash Attention 自动启用（独立，低风险）
3. **CG-02**：GGML CUDA Graph 编译标志（依赖 CG-01）
4. **OP-02**：RoPE PE GPU 化（中等复杂度）
5. **FUS-02**：Linear + GELU 融合（最复杂，可能需要自定义 CUDA 内核）

### Claude's Discretion
- 具体的 CUDA 内核实现细节
- 融合操作的精确数值精度处理
- 性能基准测试的具体工具选择（Nsight Systems vs 其他）
- 回归测试的具体测试用例

</decisions>

<specifics>
## Specific Ideas

- 优化应该对用户透明，不改变 API 或行为
- 优化应该是可选的（通过编译标志或运行时配置）
- 优化应该有明确的性能指标来验证效果
- 优化应该支持所有后端，不仅仅是 CUDA

</specifics>

<canonical_refs>
## Canonical References

### 优化分析
- `.planning/OPTIMIZATION_TODOS.md` — 完整的优化机会清单，包含所有 28 个优化项、优先级、预期收益、难度、代码位置

### 核心代码位置
- `src/ggml_extend.hpp` — GGML 扩展、GGMLRunner::compute、Flash Attention 配置
- `src/api/wan-api.cpp` — 去噪循环、CFG 实现、模型初始化
- `src/wan.hpp` — WAN DiT 模型、attention 块、FFN 块
- `src/flux.hpp` — Flux DiT 模型、attention 块、FFN 块
- `src/rope.hpp` — 位置编码生成（gen_wan_pe、gen_flux_pe）
- `src/vae.hpp` — VAE 解码器、attention 块
- `CMakeLists.txt` — 编译配置、GGML 后端选项

### 性能测量
- `.planning/OPTIMIZATION_TODOS.md` §"Measurement Methodology" — 基准测试方法、关键指标

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `GGMLRunner::compute()` — 已有缓冲区管理逻辑，可扩展为持久化模式
- `set_flash_attention_enabled()` — 已有 Flash Attention 支持，只需自动启用
- `ggml_ext_attention_ext()` — 已有 attention 实现，可用于融合优化
- `gen_wan_pe()` 和 `gen_flux_pe()` — 已有位置编码生成，可移到 GPU

### Established Patterns
- 后端检测模式：`ggml_backend_is_cuda()` 等已在代码中使用
- 编译标志模式：`WAN_CUDA`、`WAN_METAL` 等已有先例
- 性能测量模式：`ggml_time_ms()` 已在代码中使用

### Integration Points
- 缓冲区管理：`GGMLRunner` 中的 `compute_buffer` 管理
- 后端初始化：`wan_context_t` 的初始化流程
- 去噪循环：`src/api/wan-api.cpp` 中的 Euler 采样器
- 模型加载：`WanModel::load()` 中的模型初始化

</code_context>

<deferred>
## Deferred Ideas

- **第三方库集成**（cuDNN、TensorRT）— 需要单独的阶段，复杂度高
- **FP8 计算**（Hopper+ GPU）— 需要特定硬件，可作为未来优化
- **高级算子融合**（LayerNorm + modulate、QKV 投影融合）— 可作为 Phase 15
- **量化感知优化** — 需要单独的阶段

</deferred>

---

*Phase: 14-cuda-graph*
*Context gathered: 2026-03-17*
