# Phase 17: 模型单元测试与工厂模式 - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-03-27
**Phase:** 17-clip-text-encoder-vae-transformer
**Areas discussed:** 测试框架、工厂模式、测试覆盖范围、测试数据

---

## 测试框架选择

| Option | Description | Selected |
|--------|-------------|----------|
| Google Test (gtest) | C++ 标准测试框架，支持 fixtures、参数化测试、mock 对象 | |
| Catch2 | 现代 C++ 测试框架，语法简洁，支持 BDD 风格 | |
| 自定义轻量框架 | 最小化依赖，基于 assert 的简单框架 | ✓ |

**User's choice:** 自定义轻量框架
**Notes:** 用户倾向于最小化外部依赖，保持框架简洁

---

## 工厂模式设计

| Option | Description | Selected |
|--------|-------------|----------|
| 单一工厂 | 所有模型版本通过字符串或枚举注册和创建 | |
| 多工厂（按模型类型） | 每个模型类型一个工厂（CLIP工厂、T5工厂等） | |
| 通用模板工厂 | 支持任意模型类型和版本的动态注册 | ✓ |

**User's choice:** 通用模板工厂
**Notes:** 提供最大的灵活性和可扩展性，支持未来添加新模型

---

## 测试覆盖范围

| Option | Description | Selected |
|--------|-------------|----------|
| 初始化测试 | 模型初始化、权重加载、后端配置 | ✓ |
| 推理测试 | 单个输入的推理、输出形状验证、数值范围检查 | ✓ |
| 版本兼容性 | 多个版本在相同输入下的一致性验证 | ✓ |
| 鲁棒性测试 | 内存泄漏、资源释放、边界条件 | ✓ |

**User's choice:** 全选 + 数值一致性对比
**Notes:** 用户补充说明需要与标准输出进行对比验证，确保数值精度

---

## 测试数据来源

| Option | Description | Selected |
|--------|-------------|----------|
| 合成数据 | 使用小型随机张量，快速验证逻辑正确性 | |
| 真实数据 | 使用真实模型权重和标准输入/输出数据集 | |
| 混合方案 | 快速单元测试用合成数据，集成测试用真实数据 | ✓ |

**User's choice:** 混合方案
**Notes:** 平衡测试速度和准确性

---

## Claude's Discretion

- 测试用例的具体数量和粒度
- 性能基准测试的阈值设定
- 测试数据集的具体来源和格式

## Deferred Ideas

- 性能基准测试套件 — 可作为后续Phase
- 模型量化测试（FP8、INT8等） — 超出当前Phase范围
- 分布式推理测试（多GPU场景） — 超出当前Phase范围
