---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
current_phase: 19
status: Milestone complete
last_updated: "2026-03-27T13:40:00Z"
progress:
  total_phases: 7
  completed_phases: 5
  total_plans: 12
  completed_plans: 12
---

# Project State: wan-cpp

**Last Updated:** 2026-03-27
**Current Phase:** 19

## Project Reference

**Core Value:** Provide independent, lightweight, cross-platform WAN video generation inference capabilities
**Current Focus:** Phase 19 — I/O: .npy/.pt goal: .npy/.pt (Python)

## Current Position

Phase: 19 (i-o-npy-pt-goal-npy-pt-python) — Plan 01 COMPLETED
Plan: 19-01 complete

## Phase Progress

| Phase | Status | Plans | Completed |
|-------|--------|-------|------------|
| 1 - Foundation | Completed | 1/1 | 01-foundation |
| 2 - Build System | Completed | 1/1 | 02-build-system |
| 3 - Public API | Completed | 1/1 | 03-public-api |
| 4 - Examples | Completed | 1/1 | 04-examples |
| 5 - Encoders | Completed | 1/1 | 05-encoders |
| 6 - Fix Duplicate Symbols | Completed | 1/1 | 06-01 |
| 7 - Wire Core Model to API | Completed | 3/3 | 07-01, 07-02, 07-03 |
| 8 - Implement Generation + AVI Output | Completed | 2/2 | 08-01, 08-02 |
| 9 - API Fixes + Vocab + Mmap | Completed | 2/2 | 09-01, 09-02 |
| 10 - Safetensors Runtime Loading | Completed | 1/1 | 10-01 |
| 11 - Safetensors Conversion Tool | Completed | 1/1 | 11-01 |
| 12 - Wire Vocab Dir to Public API | Completed | 1/1 | 12-01 |
| 13 - Document wan-convert Sub-model Scope | Completed | 1/1 | 13-01 |
| 14 - 性能优化 - CUDA Graph 和算子融合 | Completed | 2/2 | 14-01, 14-02 |
| 15 - 多卡推理支持 | In Progress | 4/5 | 15-00, 15-01, 15-02, 15-03, 15-04 |
| 17 - 单元测试 | Completed | 2/2 | 17-01, 17-02 |
| 18 - 模型注册机制重构 | Completed | 1/1 | 18-01 |
| 19 - I/O .npy/.pt | Completed | 1/1 | 19-01 |

## Performance Metrics

| Phase | Plan | Duration | Tasks | Files | Started | Completed |
|-------|------|----------|-------|---------|-----------|-----------|
| 1 - Foundation | 01-foundation | 16 min | 10 | 30+ | 2026-03-12T04:43:06Z | 2026-03-12T04:57:08Z |
| 2 - Build System | 02-build-system | 20 min | 6 | 5 | 2026-03-12T13:05:00Z | 2026-03-12T13:25:00Z |
| 3 - Public API | 03-public-api | 45 min | 5 | 12 | 2026-03-12T14:12:00Z | 2026-03-12T14:57:00Z |
| 4 - Examples | 04-examples | 5 min | 4 | 4 | 2026-03-15T07:32:16Z | 2026-03-15T07:37:00Z |
| 6 - Fix Duplicate Symbols | 06-01 | 15 min | 3 | 4 | 2026-03-16T03:06:00Z | 2026-03-16T03:21:18Z |
| 7 - Wire Core Model | 07-02 | 5 min | 1 | 3 | 2026-03-16T05:54:39Z | 2026-03-16T05:59:53Z |
| 7 - Wire Core Model | 07-03 | 5 min | 2 | 3 | 2026-03-16T06:02:56Z | 2026-03-16T06:07:53Z |
| 8 - Implement Generation | 08-02 | 15 min | 2 | 3 | 2026-03-16T11:38:00Z | 2026-03-16T11:53:00Z |
| 9 - API Fixes + Vocab + Mmap | 09-01 | 3 min | 3 | 3 | 2026-03-16T16:07:00Z | 2026-03-16T16:10:00Z |
| 9 - API Fixes + Vocab + Mmap | 09-02 | 4 min | 3 | 3 | 2026-03-16T16:06:08Z | 2026-03-17T00:10:00Z |
| 10 - Safetensors Runtime Loading | 10-01 | 10 min | 2 | 2 | 2026-03-17T01:52:00Z | 2026-03-17T02:02:50Z |
| 11 - Safetensors Conversion Tool | 11-01 | 6 min | 2 | 5 | 2026-03-17T02:57:54Z | 2026-03-17T03:03:56Z |
| 12 - Wire Vocab Dir to Public API | 12-01 | 3 min | 2 | 6 | 2026-03-17T04:51:40Z | 2026-03-17T04:54:47Z |
| 13 - Document wan-convert Sub-model Scope | 13-01 | 2 min | 3 | 2 | 2026-03-17T06:30:19Z | 2026-03-17T06:32:01Z |
| 15 - Multi-GPU Inference Support | 15-04 | 278 | 4 | 5 | 2026-03-18T05:23:54Z | 2026-03-18T05:28:32Z |
| 17 - 单元测试 | 17-02 | 9 min | 2 | 4 | 2026-03-27T07:45:00Z | 2026-03-27T07:54:00Z |
| 18 - 模型注册机制重构 | 18-01 | 45 min | 6 | 10 | 2026-03-27T09:00:00Z | 2026-03-27T09:45:00Z |

## Accumulated Context

### Key Decisions

| Decision | Context | Rationale |
|----------|---------|-----------|
| Macro-based Model Registry | Phase 18 plan 01 | Enables global model registration at compile-time with string identifiers, decoupling version identity from implementation. |
| extern "C" DCE Guard | Phase 18 plan 01 | Prevents linker from discarding translation units that only contain static registration initializers. |
| Dummy Tensors in Unit Tests | Phase 18 plan 01 | Ensures model runners with dynamic configuration (like layer count) can be initialized in tests without actual model files. |
| libnpy header-only (custom) | Phase 19 plan 01 | Written from scratch as a self-contained header with only the dtypes needed (F32/F16/I32/I64), zero external dependencies. |
| DType enum not string | Phase 19 plan 01 | npy::DType enum over string dtype descriptors in test_io_utils.hpp avoids parsing and is type-safe at compile time. |
| ggml ne[] reversed from NumPy | Phase 19 plan 01 | ggml ne[0] is innermost/fastest dim (NumPy's last axis); load_npy reverses shape to map correctly without transposition. |

## Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 260326-h31 | 合并多个 safetensors 为单文件并将所有浮点张量统一为 bf16，重复键直接报错退出 | 2026-03-26 | 06660b9 | [260326-h31](./quick/260326-h31-safetensors-dtype-bf16/) |
| 260328-m62 | 结合相关的测试工具，生成 API 调用文档 | 2026-03-28 | 697f3e2 | [260328-m62-api](./quick/260328-m62-api/) |
| 260328-tuc | 帮我写一下各个模型推理效率的测试工具 | 2026-03-28 | 5373779 | [260328-tuc](./quick/260328-tuc/) |

## Session Continuity

**Last Action:** Completed Phase 19 Plan 01 (NPY I/O library + ggml bridge + tests)
**Next Action:** Proceed with next planned task or phase.
**Context:** libnpy integrated, test_io_utils.hpp provides load_npy/save_npy, test_io_npy.cpp passes 10/10 tests, Python generate_test_data.py verified.

---
*State updated: 2026-03-27 — Phase 19 Plan 01 complete (.npy I/O for C++ tests)*
