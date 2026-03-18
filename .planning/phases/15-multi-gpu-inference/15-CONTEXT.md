# Phase 15: 多卡推理支持 - Context

**Gathered:** 2026-03-18
**Status:** Ready for planning

<domain>
## Phase Boundary

实现 WAN-CPP 推理引擎的多 GPU 分布式推理支持。用户可以在多个 CUDA GPU 上并行执行 T2V 和 I2V 生成任务，通过配置参数选择分布式策略（张量并行、流水线并行、数据并行或混合方案）。

目标：支持多卡推理，提升吞吐量和处理能力，同时保持 API 简洁和向后兼容。

</domain>

<decisions>
## Implementation Decisions

### 分布式策略
- **支持多种策略**：张量并行（Tensor Parallelism）、流水线并行（Pipeline Parallelism）、数据并行（Data Parallelism）、混合方案（Hybrid）
- **配置管理**：混合模式 — 默认自动配置，用户可手动指定
- **实现顺序**：优先实现数据并行（最简单），后续支持张量并行和流水线并行

### API 设计
- **扩展 wan_params_t**：添加多卡相关字段（gpu_ids 数组、num_gpus、distribution_strategy 等）
- **配置时机**：在 `wan_load_model()` 时应用配置，模型加载后配置不可变
- **向后兼容**：不指定多卡配置时自动回退到单卡推理

### 通信开销
- **通信库**：使用 NCCL（NVIDIA Collective Communications Library）进行多卡间数据同步
- **执行模式**：异步执行 — GPU 不需要等待彼此，但需要管理依赖关系
- **通信优化**：最小化 AllReduce 操作，使用 P2P 通信当可能时

### 支持范围
- **生成模式**：T2V（文本生成视频）和 I2V（图像生成视频）都支持
- **后端**：仅支持 CUDA 后端（多卡推理主要用于 NVIDIA GPU）
- **GPU 类型**：仅支持同一类型的 GPU（例如：所有 A100 或所有 H100）

### 性能测量与验证
- **基准对比**：与单卡推理性能对比，计算加速比
- **关键指标**：
  - 步骤延迟（ms/step）— 去噪循环中每个 Euler 步骤的执行时间
  - 内存使用（GB）— 每个 GPU 的内存占用
  - 通信带宽利用率（%）— GPU 间通信的带宽使用效率
- **数值精度验证**：确保多卡推理结果与单卡一致（允许误差 < 0.01%）

### 向后兼容性与可选性
- **可选性**：多卡推理是可选功能，不强制使用
- **默认行为**：当用户不指定多卡配置时，自动回退到单卡推理
- **API 稳定性**：现有单卡 API 不变，新增多卡配置字段不影响现有代码

### 错误处理与故障转移
- **故障策略**：当一个 GPU 失败时，立即中断生成并返回错误
- **错误信息**：返回错误码（如 WAN_ERROR_GPU_FAILURE）和详细错误信息（失败的 GPU ID、失败原因等）
- **用户通知**：通过返回值和错误码通知用户，不使用回调函数

### Claude's Discretion
- NCCL 初始化和管理的具体实现细节
- 张量并行和流水线并行的具体分割策略
- 性能基准测试的具体工具选择（NVIDIA Nsight Systems 等）
- 多卡间的负载均衡算法
- 通信和计算的重叠优化策略

</decisions>

<specifics>
## Specific Ideas

- 多卡推理应该对用户透明，不改变生成结果的语义
- 多卡推理应该支持动态 GPU 数量（用户可以在运行时指定使用 1、2、4 或 8 个 GPU）
- 应该提供清晰的性能指标，让用户了解多卡推理的实际加速效果
- 应该支持在 wan-cli 中通过 `--gpu-ids` 或 `--num-gpus` 参数指定多卡配置
- 多卡推理应该与现有的性能优化（Phase 14 的 CUDA Graph 和算子融合）兼容

</specifics>

<canonical_refs>
## Canonical References

### 多卡推理设计
- `.planning/OPTIMIZATION_TODOS.md` — 优化机会清单，包含多卡推理相关的优化项
- `src/ggml_extend.hpp` — GGML 扩展、GGMLRunner 实现、后端管理
- `src/api/wan-api.cpp` — 模型加载、生成循环、API 实现
- `include/wan.h` — 公共 C API 定义

### NCCL 和分布式计算
- NCCL 官方文档：https://docs.nvidia.com/deeplearning/nccl/user-guide/
- CUDA 多进程服务（MPS）文档：https://docs.nvidia.com/deploy/pdf/CUDA_Multi_Process_Service_Overview.pdf

### 性能测量
- NVIDIA Nsight Systems 文档：https://docs.nvidia.com/nsight-systems/
- CUDA 性能分析工具：https://docs.nvidia.com/cuda/profiler-users-guide/

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `GGMLRunner` — 已有后端管理和计算缓冲区管理，可扩展为多卡支持
- `ggml_backend_is_cuda()` — 已有后端检测逻辑，可用于多卡初始化
- `wan_params_t` — 已有参数结构体，可扩展新增多卡配置字段
- `wan_context_t` — 已有上下文管理，可扩展为多卡上下文

### Established Patterns
- 后端检测模式：`ggml_backend_is_cuda()` 等已在代码中使用
- 编译标志模式：`WAN_CUDA`、`WAN_METAL` 等已有先例
- 错误处理模式：`WAN_ERROR_*` 错误码已有完整体系

### Integration Points
- 模型加载：`WanModel::load()` 中的模型初始化
- 生成循环：`src/api/wan-api.cpp` 中的 Euler 采样器
- 参数管理：`wan_params_t` 的初始化和传递
- 后端初始化：`wan_context_t` 的初始化流程

</code_context>

<deferred>
## Deferred Ideas

- **CPU 多进程推理** — 可作为未来扩展，当前仅支持 CUDA
- **跨机器分布式推理** — 需要网络通信，复杂度高，可作为 v2 功能
- **动态 GPU 分配** — 运行时动态增加/减少 GPU 数量，可作为未来优化
- **GPU 内存优化** — 激进的内存共享和重用策略，可作为单独的优化阶段
- **FP8 计算** — 多卡推理中的低精度计算，需要特定硬件支持

</deferred>

---

*Phase: 15-multi-gpu-inference*
*Context gathered: 2026-03-18*
