# Phase 17: 模型单元测试与工厂模式 - Context

**Gathered:** 2026-03-27
**Status:** Ready for planning

<domain>
## Phase Boundary

为四个核心模型（CLIP、T5、VAE、Transformer/Flux）建立完整的单元测试框架，使用通用模板工厂模式管理多个模型版本的注册和创建。

**四个模型及其版本：**
- CLIP: 3个版本（OPENAI_CLIP_VIT_L_14、OPEN_CLIP_VIT_H_14、OPEN_CLIP_VIT_BIGG_14）
- T5: 2个版本（标准T5、UMT5多语言）
- VAE: 4+个版本（SD1、SD2、FLUX、FLUX2等）
- Transformer: 5+个版本（FLUX、FLUX_FILL、FLEX_2、CHROMA_RADIANCE、OVIS_IMAGE等）

</domain>

<decisions>
## Implementation Decisions

### 测试框架
- **D-01:** 使用自定义轻量级测试框架（基于assert），最小化依赖
- **D-02:** 框架应支持基本的测试用例组织、断言、fixture管理

### 工厂模式设计
- **D-03:** 采用通用模板工厂（Template Factory Pattern）
- **D-04:** 支持任意模型类型和版本的动态注册和创建
- **D-05:** 工厂应提供统一的注册接口：`register<ModelType, VersionEnum>(creator_func)`
- **D-06:** 工厂应提供统一的创建接口：`create<ModelType>(version, backend, params)`

### 测试覆盖范围
- **D-07:** 初始化测试 - 模型初始化、权重加载、后端配置
- **D-08:** 推理测试 - 单个输入的推理、输出形状验证、数值范围检查
- **D-09:** 版本兼容性 - 多个版本在相同输入下的一致性验证
- **D-10:** 鲁棒性测试 - 内存泄漏、资源释放、边界条件
- **D-11:** 数值一致性对比 - 与标准输出进行对比验证

### 测试数据策略
- **D-12:** 混合方案：快速单元测试用合成数据，集成测试用真实数据
- **D-13:** 单元测试使用小型随机张量（快速验证逻辑）
- **D-14:** 集成测试使用真实模型权重和标准输入/输出数据集

### Claude's Discretion
- 测试用例的具体数量和粒度
- 性能基准测试的阈值设定
- 测试数据集的具体来源和格式

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### 模型实现
- `src/clip.hpp` — CLIP模型实现（CLIPTextModelRunner、CLIPVisionModelProjectionRunner、3个版本枚举）
- `src/t5.hpp` — T5模型实现（T5Runner、T5Embedder、UMT5支持）
- `src/vae.hpp` — VAE模型实现（AutoEncoderKL、多版本支持）
- `src/flux.hpp` — Transformer/Flux模型实现（FluxRunner、5+个版本）

### 基础设施
- `src/ggml_extend.hpp` — GGMLRunner基类、后端管理
- `include/wan.h` — 公共API接口
- `CMakeLists.txt` — 构建系统配置

### 现有测试结构
- `tests/` — 现有测试目录（如果存在）
- `.planning/REQUIREMENTS.md` — 项目需求文档

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **GGMLRunner基类** (`src/ggml_extend.hpp`) — 所有模型继承此基类，提供统一的后端管理和参数卸载接口
- **模型Runner类** — 每个模型都有对应的Runner类（CLIPTextModelRunner、T5Runner等），实现了compute()接口
- **版本枚举** — 每个模型都定义了版本枚举（CLIPVersion、SDVersion等）

### Established Patterns
- **后端管理** — 所有模型支持多后端（CPU/CUDA），通过ggml_backend_t参数传递
- **参数卸载** — 支持offload_params_to_cpu优化，减少GPU内存占用
- **张量存储** — 使用String2TensorStorage映射管理权重

### Integration Points
- 新的测试框架应与现有CMake构建系统集成
- 工厂模式应与wan.h公共API兼容
- 测试应能访问所有四个模型的Runner类

</code_context>

<specifics>
## Specific Ideas

- 工厂模式应支持链式配置：`factory.create<CLIP>().withVersion(OPENAI_CLIP_VIT_L_14).withBackend(CUDA).build()`
- 测试框架应生成可读的测试报告，包括通过/失败统计、性能指标
- 考虑为每个模型版本创建独立的测试套件文件（test_clip_versions.cpp、test_t5_versions.cpp等）

</specifics>

<deferred>
## Deferred Ideas

- 性能基准测试套件（可作为Phase 18）
- 模型量化测试（FP8、INT8等）
- 分布式推理测试（多GPU场景）

</deferred>

---

*Phase: 17-clip-text-encoder-vae-transformer*
*Context gathered: 2026-03-27*
