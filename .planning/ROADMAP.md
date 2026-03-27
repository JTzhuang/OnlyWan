# Roadmap: wan-cpp

**Project:** Standalone C++ Library for WAN Video Generation
**Created:** 2018-03-12

## Milestones

- ✅ **v1.0 MVP** — Phases 1-8 (shipped 2018-03-16)
- ✅ **v1.1 模型格式扩展** — Phases 9-13 (completed 2018-03-17)
- ✅ **v1.2 性能优化与模型工程** — Phases 14-18 (completed 2026-03-27)
- 🔄 **v1.3 分布式推理与生产环境增强** — Phase 15 Plan 05+ (in progress)

## Phases

<details>
<summary>✅ v1.0 MVP (Phases 1-8) — SHIPPED 2018-03-16</summary>

- [x] Phase 1: Foundation (1/1 plans) — completed 2018-03-12
- [x] Phase 2: Build System (1/1 plans) — completed 2018-03-12
- [x] Phase 3: Public API (1/1 plans) — completed 2018-03-15
- [x] Phase 4: Examples (1/1 plans) — completed 2018-03-15
- [x] Phase 5: Encoders (1/1 plans) — completed 2018-03-16
- [x] Phase 6: Fix Duplicate Symbols (1/1 plans) — completed 2018-03-16
- [x] Phase 7: Wire Core Model to API (3/3 plans) — completed 2018-03-16
- [x] Phase 8: Implement Generation + AVI Output (2/2 plans) — completed 2018-03-16

See `.planning/milestones/v1.0-ROADMAP.md` for full phase details.

</details>

<details>
<summary>✅ v1.1 模型格式扩展 (Phases 9-13) — SHIPPED 2018-03-17</summary>

- [x] Phase 9: API Fixes + Vocab mmap (2/2 plans) — completed 2018-03-16
- [x] Phase 10: Safetensors Runtime Loading (1/1 plans) — completed 2018-03-17
- [x] Phase 11: Safetensors Conversion Tool (1/1 plans) — completed 2018-03-17
- [x] Phase 12: Wire Vocab Dir to Public API (1/1 plans) — completed 2018-03-17
- [x] Phase 13: Document wan-convert Sub-model Scope (1/1 plans) — completed 2018-03-17

</details>

<details>
<summary>✅ v1.2 性能优化与模型工程 (Phases 14, 16-18) — SHIPPED 2026-03-27</summary>

- [x] Phase 14: 性能优化 - CUDA Graph 和算子融合 (2/2 plans) — completed 2018-03-17
- [x] Phase 16: spdlog 日志系统集成 (1/1 plans) — completed 2026-03-26
- [x] Phase 17: 单元测试 (2/2 plans) — completed 2026-03-27
- [x] Phase 18: 模型注册机制重构 (1/1 plans) — completed 2026-03-27

</details>

## Phase Details

### Phase 14: 性能优化 - CUDA Graph 和算子融合
**Goal:** 实现 5 个 Quick Wins 优化，达成 2-5x 去噪循环加速和 10-20% 整体推理加速
- [x] 14-01-PLAN.md — 缓冲区持久化 + Flash Attention 自动启用 + CUDA Graph 编译标志 (CG-01, OP-01, CG-02)
- [x] 14-02-PLAN.md — RoPE PE GPU 化 + Linear+GELU 算子融合 (OP-02, FUS-02)

### Phase 15: 多卡推理支持
**Goal:** 支持多 GPU 分布式推理，通过张量并行和数据并行提升吞吐量
- [x] 15-00-PLAN.md — Wave 0 测试基础设施 (MGPU-01)
- [x] 15-01-PLAN.md — Wave 1 多卡 API 类型定义 + CMake NCCL 集成 (MGPU-02, MGPU-03)
- [x] 15-02-PLAN.md — Wave 3 多卡后端初始化 + 张量并行模型加载 (MGPU-04, MGPU-05)
- [x] 15-03-PLAN.md — Wave 2 数据并行批量生成实现 (MGPU-06)
- [x] 15-04-PLAN.md — Wave 4 CLI 多卡参数 + GPU 信息查询 (MGPU-08, MGPU-09, MGPU-10)

### Phase 16: spdlog 日志系统集成
**Goal:** 集成 spdlog 日志系统，支持级别控制并保持 C API 兼容
- [x] 16-01-PLAN.md — 基础 spdlog 集成与 util 日志宏重写 (LOG-01, LOG-02, LOG-03, LOG-04)

### Phase 17: 单元测试
**Goal:** 为核心模型建立 C++ 单元测试框架，使用模板工厂管理版本
- [x] 17-01-PLAN.md — 测试基础设施 + 模板工厂 + 工厂单元测试 (TEST-01, TEST-02)
- [x] 17-02-PLAN.md — 四个模型的版本初始化单元测试 (TEST-03)

### Phase 18: 模型注册机制重构 - 宏注册 + 字符串版本
**Goal:** 模型版本注册迁移到 src/，使用宏全局注册，字符串区分版本
- [x] 18-01-PLAN.md — PerTypeRegistry 基础设施 + 9 个 WAN 模型注册 + 5 个测试重构 (REG-01, REG-02, REG-03, REG-04, REG-05)

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1-8 | v1.0 | 11/11 | Complete | 2018-03-16 |
| 9-13 | v1.1 | 6/6 | Complete | 2018-03-17 |
| 14, 16-18 | v1.2 | 6/6 | Complete | 2026-03-27 |
| 15 | v1.3 | 5/5 | Complete | 2018-03-18 |
