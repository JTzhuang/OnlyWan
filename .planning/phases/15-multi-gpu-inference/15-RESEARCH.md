# Phase 15: 多卡推理支持 - Research

**Researched:** 2026-03-18
**Domain:** Multi-GPU distributed inference for video generation models
**Confidence:** MEDIUM

## Summary

Multi-GPU inference for diffusion-based video generation models presents unique challenges compared to LLM inference. The WAN-CPP project can leverage GGML's existing backend scheduler API for multi-device support, which provides a foundation for data parallelism and tensor splitting. However, implementing full tensor parallelism or pipeline parallelism for diffusion transformers requires careful consideration of the denoising loop structure and communication patterns.

Research reveals that GGML already provides `ggml_backend_sched` API for multi-GPU orchestration and `ggml_backend_cuda_split_buffer_type` for tensor splitting across devices. The primary challenge is adapting the Euler denoising loop (which currently rebuilds graphs per step) to work efficiently with multi-GPU scheduling while maintaining numerical consistency.

**Primary recommendation:** Start with data parallelism (batch-level parallelism) as the simplest approach, then explore GGML's tensor split capabilities for model parallelism. Avoid NCCL integration initially—GGML's backend scheduler handles device communication internally.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **支持多种策略**：张量并行（Tensor Parallelism）、流水线并行（Pipeline Parallelism）、数据并行（Data Parallelism）、混合方案（Hybrid）
- **配置管理**：混合模式 — 默认自动配置，用户可手动指定
- **实现顺序**：优先实现数据并行（最简单），后续支持张量并行和流水线并行
- **扩展 wan_params_t**：添加多卡相关字段（gpu_ids 数组、num_gpus、distribution_strategy 等）
- **配置时机**：在 `wan_load_model()` 时应用配置，模型加载后配置不可变
- **向后兼容**：不指定多卡配置时自动回退到单卡推理
- **通信库**：使用 NCCL（NVIDIA Collective Communications Library）进行多卡间数据同步
- **执行模式**：异步执行 — GPU 不需要等待彼此，但需要管理依赖关系
- **通信优化**：最小化 AllReduce 操作，使用 P2P 通信当可能时
- **生成模式**：T2V（文本生成视频）和 I2V（图像生成视频）都支持
- **后端**：仅支持 CUDA 后端（多卡推理主要用于 NVIDIA GPU）
- **GPU 类型**：仅支持同一类型的 GPU（例如：所有 A100 或所有 H100）
- **基准对比**：与单卡推理性能对比，计算加速比
- **关键指标**：步骤延迟（ms/step）、内存使用（GB）、通信带宽利用率（%）
- **数值精度验证**：确保多卡推理结果与单卡一致（允许误差 < 0.01%）
- **可选性**：多卡推理是可选功能，不强制使用
- **默认行为**：当用户不指定多卡配置时，自动回退到单卡推理
- **API 稳定性**：现有单卡 API 不变，新增多卡配置字段不影响现有代码
- **故障策略**：当一个 GPU 失败时，立即中断生成并返回错误
- **错误信息**：返回错误码（如 WAN_ERROR_GPU_FAILURE）和详细错误信息（失败的 GPU ID、失败原因等）
- **用户通知**：通过返回值和错误码通知用户，不使用回调函数

### Claude's Discretion
- NCCL 初始化和管理的具体实现细节
- 张量并行和流水线并行的具体分割策略
- 性能基准测试的具体工具选择（NVIDIA Nsight Systems 等）
- 多卡间的负载均衡算法
- 通信和计算的重叠优化策略

### Deferred Ideas (OUT OF SCOPE)
- **CPU 多进程推理** — 可作为未来扩展，当前仅支持 CUDA
- **跨机器分布式推理** — 需要网络通信，复杂度高，可作为 v2 功能
- **动态 GPU 分配** — 运行时动态增加/减少 GPU 数量，可作为未来优化
- **GPU 内存优化** — 激进的内存共享和重用策略，可作为单独的优化阶段
- **FP8 计算** — 多卡推理中的低精度计算，需要特定硬件支持
</user_constraints>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| GGML backend scheduler | (submodule) | Multi-backend orchestration | Already integrated, provides device scheduling and tensor placement |
| CUDA Runtime | 11.0+ | GPU device management | Required for multi-GPU enumeration and context management |
| NCCL | 2.26+ | Multi-GPU communication | Industry standard for GPU collective operations, optimized for NVIDIA hardware |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| CUDA Driver API | 11.0+ | Low-level device queries | For GPU topology detection and memory queries |
| cuBLAS | (bundled) | Matrix operations | Already used by GGML CUDA backend |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| NCCL | GGML internal communication | GGML backend scheduler already handles device-to-device copies; NCCL adds complexity but enables advanced collective operations |
| Explicit NCCL | GGML split buffers | GGML's `ggml_backend_cuda_split_buffer_type` provides tensor splitting without NCCL; simpler but less flexible |
| Data parallelism | Tensor parallelism | Data parallelism is simpler (independent batches) but requires batch size > 1; tensor parallelism works for batch=1 but requires model surgery |

**Installation:**
```bash
# NCCL installation (if needed beyond GGML)
# Ubuntu/Debian
sudo apt-get install libnccl2 libnccl-dev

# Or build from source
git clone https://github.com/NVIDIA/nccl.git
cd nccl
make -j src.build
```

**Version verification:**
```bash
# Check CUDA device count
nvidia-smi --query-gpu=count --format=csv,noheader

# Check NCCL version (if installed)
dpkg -l | grep nccl
```

**Note:** GGML already includes CUDA backend support. Multi-GPU support primarily requires using GGML's backend scheduler API rather than adding new dependencies.

## Architecture Patterns

### Recommended Project Structure
```
src/
├── api/
│   └── wan-api.cpp           # Extend with multi-GPU initialization
├── ggml_extend.hpp           # GGMLRunner already supports backend selection
└── multi_gpu/                # NEW: Multi-GPU coordination layer
    ├── scheduler.hpp         # Wrap ggml_backend_sched API
    ├── device_manager.hpp    # GPU enumeration and validation
    └── strategies.hpp        # Data/tensor/pipeline parallel strategies
```

### Pattern 1: GGML Backend Scheduler Usage
**What:** Use GGML's built-in multi-backend scheduler to distribute computation across GPUs
**When to use:** For all multi-GPU scenarios; GGML handles device placement and data movement
**Example:**
```cpp
// Source: ggml/include/ggml-backend.h (lines 8-30 from grep results)
// Initialize multiple CUDA backends
ggml_backend_t backends[4];
backends[0] = ggml_backend_cuda_init(0);  // GPU 0
backends[1] = ggml_backend_cuda_init(1);  // GPU 1
backends[2] = ggml_backend_cuda_init(2);  // GPU 2
backends[3] = ggml_backend_cuda_init(3);  // GPU 3

// Create scheduler with all backends
ggml_backend_sched_t sched = ggml_backend_sched_new(
    backends,
    nullptr,  // buffer types (auto-detect)
    4,        // num backends
    GGML_DEFAULT_GRAPH_SIZE,
    false,    // parallel execution
    true      // op offload
);

// Optionally assign specific tensors to specific backends
ggml_backend_sched_set_tensor_backend(sched, tensor, backends[0]);

// Reserve memory for graph
ggml_backend_sched_reserve(sched, measure_graph);

// Execute graph across multiple GPUs
ggml_backend_sched_graph_compute(sched, graph);
```

### Pattern 2: Tensor Split for Model Parallelism
**What:** Use GGML's split buffer type to distribute large tensors across GPUs by rows
**When to use:** When model doesn't fit in single GPU memory
**Example:**
```cpp
// Source: ggml/include/ggml-cuda.h (line 31)
// Define tensor split ratios (e.g., 50% GPU0, 50% GPU1)
float tensor_split[2] = {0.5f, 0.5f};

// Create split buffer type
ggml_backend_buffer_type_t split_buft =
    ggml_backend_cuda_split_buffer_type(0, tensor_split);

// Allocate model parameters using split buffer
ggml_backend_buffer_t params_buffer =
    ggml_backend_alloc_ctx_tensors_from_buft(params_ctx, split_buft);
```

### Pattern 3: Data Parallelism for Batch Processing
**What:** Process multiple independent generation requests in parallel across GPUs
**When to use:** When batch size > 1 or processing multiple prompts simultaneously
**Example:**
```cpp
// Pseudo-code for data parallel inference
struct MultiGPUContext {
    std::vector<wan_context_t*> gpu_contexts;  // One context per GPU
    std::vector<std::thread> workers;
};

// Distribute batch across GPUs
void generate_batch_parallel(MultiGPUContext* ctx,
                             std::vector<GenerationRequest>& requests) {
    int gpus = ctx->gpu_contexts.size();
    for (int i = 0; i < requests.size(); i++) {
        int gpu_id = i % gpus;
        ctx->workers[gpu_id] = std::thread([&, gpu_id, i]() {
            wan_generate_video_t2v_ex(
                ctx->gpu_contexts[gpu_id],
                requests[i].params,
                // ... other args
            );
        });
    }
    // Wait for all workers
    for (auto& worker : ctx->workers) worker.join();
}
```

### Anti-Patterns to Avoid
- **Manual CUDA memory management across devices:** Let GGML backend scheduler handle device placement and data movement
- **Synchronous multi-GPU execution:** Use `ggml_backend_sched_graph_compute_async` for overlapping computation
- **Ignoring graph structure stability:** Multi-GPU scheduling benefits from stable graph structure (Phase 14's buffer persistence)
- **NCCL for simple cases:** GGML's backend scheduler handles device-to-device copies; only add NCCL if implementing advanced collective operations

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Multi-GPU scheduling | Custom device placement logic | `ggml_backend_sched` API | GGML scheduler handles tensor placement, memory allocation, and device synchronization automatically |
| Tensor splitting | Manual tensor partitioning | `ggml_backend_cuda_split_buffer_type` | GGML provides row-wise tensor splitting with automatic gradient handling |
| Device enumeration | Direct CUDA API calls | `ggml_backend_cuda_get_device_count()` | GGML abstracts device queries across backends |
| Cross-device copies | cudaMemcpyPeer | GGML backend buffer operations | GGML optimizes data movement based on topology (NVLink vs PCIe) |
| Collective operations (AllReduce) | Custom NCCL wrappers | GGML backend scheduler (for simple cases) | GGML handles common patterns; only add NCCL for advanced collectives |

**Key insight:** GGML already provides a comprehensive multi-backend infrastructure. The challenge is adapting WAN-CPP's denoising loop to work with GGML's scheduler rather than building a parallel system from scratch.

## Common Pitfalls

### Pitfall 1: Graph Rebuilding Per Step Breaks Multi-GPU Efficiency
**What goes wrong:** Current Euler loop rebuilds compute graph every step (lines 566-599 in wan-api.cpp). Multi-GPU scheduling overhead dominates when graph structure changes frequently.
**Why it happens:** Phase 14 enabled buffer persistence, but graph is still rebuilt per step for CFG (conditional + unconditional passes).
**How to avoid:** Cache graph structure across steps; use `ggml_backend_sched_reset()` + `ggml_backend_sched_alloc_graph()` pattern to reuse allocation.
**Warning signs:** Multi-GPU performance worse than single GPU; high CPU usage during denoising loop.

### Pitfall 2: Assuming NCCL is Required
**What goes wrong:** Adding NCCL dependency and custom communication code when GGML backend scheduler already handles device-to-device transfers.
**Why it happens:** LLM inference frameworks (vLLM, TensorRT-LLM) heavily use NCCL, creating assumption it's required for all multi-GPU work.
**How to avoid:** Start with GGML's built-in capabilities (`ggml_backend_sched`, split buffers). Only add NCCL if profiling shows communication bottleneck that GGML can't optimize.
**Warning signs:** Complex NCCL initialization code; manual ncclAllReduce calls; communication deadlocks.

### Pitfall 3: Ignoring Numerical Consistency Across Devices
**What goes wrong:** Floating-point operations on different GPUs produce slightly different results due to non-associativity and hardware differences.
**Why it happens:** Different GPUs may use different cuBLAS algorithms; tensor split boundaries affect reduction order.
**How to avoid:** Use deterministic cuBLAS mode; validate multi-GPU output against single-GPU baseline; set tolerance threshold (< 0.01% as specified).
**Warning signs:** Generated videos differ visually between single-GPU and multi-GPU runs; numerical tests fail.

### Pitfall 4: Inefficient Data Parallelism for Batch=1
**What goes wrong:** Implementing data parallelism when typical use case is batch=1 (single video generation).
**Why it happens:** Data parallelism is simplest to implement, but provides no benefit for single-request workloads.
**How to avoid:** For batch=1, prioritize tensor parallelism (split model across GPUs) or pipeline parallelism (split denoising steps). Data parallelism only helps with batch > 1 or concurrent requests.
**Warning signs:** Single GPU fully utilized while other GPUs idle; no speedup for single video generation.

### Pitfall 5: Not Validating GPU Homogeneity
**What goes wrong:** Mixing different GPU types (e.g., A100 + V100) causes load imbalance and synchronization issues.
**Why it happens:** User constraint specifies "same GPU type" but implementation doesn't validate.
**How to avoid:** Query GPU compute capability and memory size at initialization; reject mixed configurations with clear error message.
**Warning signs:** One GPU finishes much faster than others; out-of-memory errors on smaller GPU.

## Code Examples

Verified patterns from GGML source:

### Multi-Backend Initialization
```cpp
// Source: ggml/include/ggml-cuda.h + ggml/include/ggml-backend.h
#include <ggml-cuda.h>
#include <ggml-backend.h>

// Query available CUDA devices
int n_devices = ggml_backend_cuda_get_device_count();
if (n_devices < 2) {
    return WAN_ERROR_INVALID_ARGUMENT;  // Multi-GPU requires 2+ devices
}

// Validate GPU homogeneity
char desc0[256], desc1[256];
ggml_backend_cuda_get_device_description(0, desc0, sizeof(desc0));
ggml_backend_cuda_get_device_description(1, desc1, sizeof(desc1));
if (strcmp(desc0, desc1) != 0) {
    LOG_ERROR("Mixed GPU types not supported: %s vs %s", desc0, desc1);
    return WAN_ERROR_INVALID_ARGUMENT;
}

// Initialize backends for each device
std::vector<ggml_backend_t> backends;
for (int i = 0; i < n_devices; i++) {
    ggml_backend_t backend = ggml_backend_cuda_init(i);
    if (!backend) {
        LOG_ERROR("Failed to initialize CUDA backend for device %d", i);
        return WAN_ERROR_BACKEND_FAILED;
    }
    backends.push_back(backend);
}
```

### Backend Scheduler Setup
```cpp
// Source: ggml/include/ggml-backend.h (scheduler API)
// Create scheduler with multiple backends
ggml_backend_sched_t sched = ggml_backend_sched_new(
    backends.data(),
    nullptr,  // buffer types (nullptr = auto-detect)
    backends.size(),
    GGML_DEFAULT_GRAPH_SIZE,
    false,    // parallel = false (sequential execution for now)
    true      // op_offload = true (enable operation offloading)
);

if (!sched) {
    LOG_ERROR("Failed to create backend scheduler");
    return WAN_ERROR_OUT_OF_MEMORY;
}

// Reserve memory for largest expected graph
ggml_backend_sched_reserve(sched, measure_graph);
```

### Tensor Split Configuration
```cpp
// Source: ggml/include/ggml-cuda.h (line 31)
// Configure tensor split ratios (equal distribution)
std::vector<float> tensor_split(n_devices, 1.0f / n_devices);

// Create split buffer type (main device = 0)
ggml_backend_buffer_type_t split_buft =
    ggml_backend_cuda_split_buffer_type(0, tensor_split.data());

// Use split buffer for model parameters
// This distributes large weight matrices across GPUs by rows
ggml_backend_buffer_t params_buffer =
    ggml_backend_alloc_ctx_tensors_from_buft(params_ctx, split_buft);
```

### Multi-GPU Graph Execution
```cpp
// Source: ggml/include/ggml-backend.h (scheduler compute API)
// Build graph (same as single-GPU)
ggml_cgraph* graph = build_denoising_graph(ctx, x, timestep, context);

// Allocate graph on scheduler (distributes across backends)
if (!ggml_backend_sched_alloc_graph(sched, graph)) {
    LOG_ERROR("Failed to allocate graph on multi-GPU scheduler");
    return false;
}

// Execute graph (scheduler handles device placement and synchronization)
ggml_status status = ggml_backend_sched_graph_compute(sched, graph);
if (status != GGML_STATUS_SUCCESS) {
    LOG_ERROR("Multi-GPU graph compute failed: %s",
              ggml_status_to_string(status));
    return false;
}

// For async execution (overlap with CPU work):
// ggml_backend_sched_graph_compute_async(sched, graph);
// ... do other work ...
// ggml_backend_sched_synchronize(sched);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual CUDA multi-GPU | GGML backend scheduler | GGML 2024+ | Unified API across backends; automatic device placement |
| NCCL for all communication | GGML internal transfers + optional NCCL | GGML 2024+ | Simpler for common cases; NCCL only when needed |
| Naive patch parallelism | PipeFusion/xDiT patch-level pipeline | 2024-2025 | 2-4x speedup for diffusion models; reduces communication overhead |
| Synchronous multi-GPU | Async compute + overlapping | GGML 2024+ | Better GPU utilization; hides communication latency |
| LLM-style tensor parallel | Diffusion-specific strategies | 2024-2025 | Diffusion models benefit more from patch/pipeline parallel than pure tensor parallel |

**Deprecated/outdated:**
- **Direct cudaSetDevice() calls:** GGML backend API abstracts device selection
- **Manual graph splitting:** GGML scheduler handles automatic graph partitioning
- **GGML format:** Superseded by GGUF (but not relevant to multi-GPU)

**Emerging patterns (2025-2026):**
- **Patch-level pipeline parallelism:** Partition image/video into patches, pipeline across GPUs (PipeFusion approach)
- **Hybrid parallelism:** Combine data + tensor + pipeline for 8+ GPU setups (xDiT approach)
- **Unified Sequence Parallelism (USP):** Combines attention and FFN parallelism for diffusion transformers

## Open Questions

1. **Does GGML backend scheduler support pipeline parallelism for denoising loops?**
   - What we know: GGML scheduler supports sequential graph execution across backends
   - What's unclear: Whether scheduler can pipeline multiple denoising steps (step N on GPU0 while step N+1 on GPU1)
   - Recommendation: Start with tensor split (model parallelism) for batch=1; investigate pipeline parallel as optimization

2. **What is the communication overhead of GGML's device-to-device transfers vs NCCL?**
   - What we know: GGML uses cudaMemcpyPeer for NVLink-connected GPUs
   - What's unclear: Performance comparison with NCCL's optimized collectives for large tensors
   - Recommendation: Profile GGML transfers first; add NCCL only if bottleneck identified

3. **How to handle CFG (classifier-free guidance) in multi-GPU setup?**
   - What we know: CFG requires 2 forward passes per step (conditional + unconditional)
   - What's unclear: Best strategy—run both on same GPU, split across GPUs, or alternate GPUs per step
   - Recommendation: Run both passes on same GPU set initially (simpler); explore split as optimization

4. **What is the minimum batch size for data parallelism to be effective?**
   - What we know: Data parallelism requires batch > 1; typical use case is batch=1
   - What's unclear: At what batch size does data parallel outperform tensor parallel
   - Recommendation: Implement tensor parallel first (works for batch=1); add data parallel for batch inference later

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | pytest 1.0.0 (detected) + C++ unit tests |
| Config file | None — create tests/ directory in Wave 0 |
| Quick run command | `pytest tests/test_multi_gpu.py -x` |
| Full suite command | `pytest tests/ -v` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| MGPU-01 | Multi-GPU initialization validates device count and homogeneity | unit | `pytest tests/test_device_manager.py::test_gpu_validation -x` | ❌ Wave 0 |
| MGPU-02 | Data parallel: batch of 4 on 2 GPUs produces same results as single GPU | integration | `pytest tests/test_data_parallel.py::test_batch_consistency -x` | ❌ Wave 0 |
| MGPU-03 | Tensor split: model distributed across 2 GPUs produces same output as single GPU | integration | `pytest tests/test_tensor_split.py::test_numerical_consistency -x` | ❌ Wave 0 |
| MGPU-04 | Single-GPU fallback when num_gpus=1 or multi-GPU unavailable | unit | `pytest tests/test_fallback.py::test_single_gpu_fallback -x` | ❌ Wave 0 |
| MGPU-05 | Error handling: GPU failure during generation returns WAN_ERROR_GPU_FAILURE | unit | `pytest tests/test_error_handling.py::test_gpu_failure -x` | ❌ Wave 0 |
| MGPU-06 | Performance: 2-GPU inference faster than 1-GPU for batch > 1 | manual | Manual benchmark with `wan-cli --num-gpus 2 --batch 4` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `pytest tests/test_multi_gpu.py -x` (quick smoke test)
- **Per wave merge:** `pytest tests/ -v` (full suite)
- **Phase gate:** Full suite green + manual performance validation before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `tests/test_device_manager.py` — covers MGPU-01 (device validation)
- [ ] `tests/test_data_parallel.py` — covers MGPU-02 (data parallel consistency)
- [ ] `tests/test_tensor_split.py` — covers MGPU-03 (tensor split consistency)
- [ ] `tests/test_fallback.py` — covers MGPU-04 (single-GPU fallback)
- [ ] `tests/test_error_handling.py` — covers MGPU-05 (error handling)
- [ ] `tests/conftest.py` — shared fixtures (mock GPU contexts, test models)
- [ ] Framework setup: Create `tests/` directory and pytest configuration

## Sources

### Primary (HIGH confidence)
- GGML source code: `ggml/include/ggml-backend.h` — Backend scheduler API documentation and usage examples
- GGML source code: `ggml/include/ggml-cuda.h` — CUDA backend API including split buffer type
- WAN-CPP source: `src/api/wan-api.cpp` — Current denoising loop implementation (lines 560-610)
- WAN-CPP source: `src/ggml_extend.hpp` — GGMLRunner implementation with backend management (lines 2000-2150)

### Secondary (MEDIUM confidence)
- [Scaling LLM Inference: Data, Pipeline & Tensor Parallelism in vLLM](https://docs.jarvislabs.ai/blog/scaling-llm-inference-dp-pp-tp) — Clear definitions of parallelism strategies
- [NCCL 2.26 Release](https://developer.nvidia.com/blog/improved-performance-and-monitoring-capabilities-with-nvidia-collective-communications-library-2-26/) — Latest NCCL features and performance improvements
- [Multi-GPU Inference with GGML](https://cran.csail.mit.edu/web/packages/ggmlR/vignettes/multi-gpu.html) — GGML backend scheduler usage patterns
- [NCCL Examples](https://docs.nvidia.com/deeplearning/nccl/user-guide/docs/examples.html) — Official NCCL API usage examples

### Tertiary (LOW confidence - research papers, not production-ready)
- [Patch-level Pipeline Parallelism for Diffusion Transformers](https://arxiv.org/html/2405.14430v4) — PipeFusion approach for diffusion models
- [xDiT: Inference Engine for Diffusion Transformers with Massive Parallelism](https://arxiv.org/html/2411.01738) — Hybrid parallelism strategies for DiTs
- [Distributed Parallel Inference for High-Resolution Diffusion Models](https://arxiv.org/html/2402.19481v3) — DistriFusion patch-based parallelism
- [Multi-Level Collaborative Acceleration for Distributed Diffusion](https://arxiv.org/html/2602.10940v1) — Unified Sequence Parallelism (USP) for large diffusion models

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - GGML backend scheduler is already integrated and documented
- Architecture: MEDIUM - GGML patterns are clear, but adapting denoising loop requires experimentation
- Pitfalls: MEDIUM - Based on GGML source analysis and general multi-GPU experience, not WAN-specific testing
- Diffusion-specific strategies: LOW - Research papers describe advanced techniques but lack production implementations

**Research date:** 2026-03-18
**Valid until:** 2026-04-18 (30 days - GGML API is relatively stable)

**Key uncertainties:**
- Performance characteristics of GGML backend scheduler for diffusion workloads (requires profiling)
- Optimal parallelism strategy for batch=1 video generation (tensor split vs pipeline)
- Whether NCCL integration provides meaningful benefit over GGML's internal transfers
- Numerical consistency validation methodology (tolerance thresholds, test cases)
