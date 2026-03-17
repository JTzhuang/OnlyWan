# Phase 13: Document wan-convert Sub-model Scope - Context

**Gathered:** 2026-03-17
**Status:** Ready for planning

<domain>
## Phase Boundary

在 `wan-convert --help` 输出和 `examples/convert/README.md` 中明确说明哪些 `--type` 值（dit-t2v/dit-i2v/dit-ti2v）产生可被 `wan_load_model` 加载的文件，哪些（vae/t5/clip）为未来多文件加载预留。同时更新 REQUIREMENTS.md 中 SAFE-03 的追踪性注释。

不包括：实现 vae/t5/clip 的实际加载支持、修改转换逻辑、添加新 --type 值。

</domain>

<decisions>
## Implementation Decisions

### --help 输出标注
- 在 `print_usage()` 中为每个 `--type` 值添加内联标注
- dit-t2v/dit-i2v/dit-ti2v 后加：`(loadable by wan_load_model)`
- vae/t5/clip 后加：`(reserved: future multi-file loading)`
- 语气中性，不使用警告语气

### 文档位置
- 新建 `examples/convert/README.md`（不修改 `examples/README.md`）
- 内容包含：用法示例、类型说明表格（含可加载/预留区分）、限制说明段落

### SAFE-03 追踪性
- `REQUIREMENTS.md` 中 SAFE-03 保留 `[ ]` 未勾选状态
- 在条目旁添加注释，说明：dit-* 类型已满足，vae/t5/clip 为未来预留，Phase 13 添加文档说明

### Claude's Discretion
- `examples/convert/README.md` 的具体 Markdown 结构和措辞
- 类型说明表格的列设计
- 限制说明段落的具体文字

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### 转换工具实现
- `examples/convert/main.cpp` — `print_usage()` 函数和 `SUBMODEL_META` 映射，需在此修改 --help 输出

### 需求追踪
- `.planning/REQUIREMENTS.md` — SAFE-03 条目位置，需添加注释说明部分满足状态

### 现有文档参考
- `examples/README.md` — 现有文档风格参考，新 README 应保持一致的 Markdown 风格

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `print_usage()` in `examples/convert/main.cpp:28-47`：当前已列出所有 6 种类型，只需在每行末尾添加标注字符串
- `SUBMODEL_META` map：明确定义了 6 种类型，可作为文档表格的数据来源

### Established Patterns
- `examples/README.md` 使用标准 Markdown，代码块用 ```bash，表格用 | 分隔
- `examples/cli/` 目录下没有独立 README，但 `examples/README.md` 有完整的 CLI 文档节

### Integration Points
- `examples/convert/main.cpp` 的 `print_usage()` 是唯一需要修改的 C++ 文件
- `REQUIREMENTS.md` 的 SAFE-03 行需要添加注释（保持 `[ ]` 状态）

</code_context>

<specifics>
## Specific Ideas

- 类型说明表格建议列：`--type 值` | `子模型` | `状态` | `说明`
- 限制说明示例措辞：`vae/t5/clip types produce valid GGUF files but wan_load_model currently expects a single DiT checkpoint. Multi-file loading is planned for a future release.`

</specifics>

<deferred>
## Deferred Ideas

- 实现 vae/t5/clip 的实际 wan_load_model 多文件加载支持 — 未来里程碑
- 为 wan-convert 添加 --dry-run 或验证模式 — 超出本阶段范围

</deferred>

---

*Phase: 13-document-wan-convert-sub-model-scope*
*Context gathered: 2026-03-17*
