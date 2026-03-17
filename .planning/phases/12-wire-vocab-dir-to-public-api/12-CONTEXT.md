# Phase 12: Wire Vocab Dir to Public API - Context

**Gathered:** 2026-03-17
**Status:** Ready for planning

<domain>
## Phase Boundary

将内部 `wan_vocab_set_dir()` 桥接为公共 C API 函数 `wan_set_vocab_dir()`，并在 `wan-cli` 中添加 `--vocab-dir` 参数，使 `WAN_EMBED_VOCAB=OFF` 构建下 T2V/I2V 生成可正常运行。

不包括：词汇表文件格式变更、多 context 独立词汇表、自动发现词汇表目录。

</domain>

<decisions>
## Implementation Decisions

### API 函数签名
- 函数签名：`wan_error_t wan_set_vocab_dir(const char* dir)`
- 全局函数，不绑定到 context（与内部 `wan_vocab_set_dir(const std::string&)` 对称）
- 必须在 `wan_load_model` / `wan_load_model_from_file` 之前调用
- 在 `wan.h` 的 "Parameter Configuration" 区块中声明

### 错误处理
- `wan_load_model` 时校验词汇表目录是否存在；目录不存在返回 `WAN_ERROR_INVALID_ARGUMENT`
- `wan_set_vocab_dir` 本身不校验目录（仅存储路径），校验推迟到 load_model

### WAN_EMBED_VOCAB=ON 时的行为
- 调用 `wan_set_vocab_dir` 时返回警告级错误码（`WAN_ERROR_INVALID_ARGUMENT` 或新增 `WAN_WARN_IGNORED`，由 planner 决定具体码值）
- 嵌入词汇表仍然生效，外部目录被忽略

### CLI 参数设计
- 参数名：`--vocab-dir <path>`（与 `--model`、`--output` 风格一致）
- 可选参数，不强制要求
- `WAN_EMBED_VOCAB=OFF` 构建中未提供 `--vocab-dir` 时，在启动时打印警告提示用户
- 调用顺序：解析参数 → 调用 `wan_set_vocab_dir` → 调用 `wan_load_model`

### Claude's Discretion
- `wan_set_vocab_dir` 在 `wan.h` 中的具体注释措辞
- 警告信息的具体文本
- `WAN_EMBED_VOCAB=ON` 时返回的具体警告码值（可复用现有码或新增）

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### 公共 API
- `include/wan-cpp/wan.h` — 现有 C API 声明，新函数需遵循同一风格（extern "C"、WAN_API 宏、注释格式）

### 词汇表内部实现
- `src/vocab/vocab.h` — 声明 `wan_vocab_set_dir(const std::string&)`，Phase 12 的桥接目标
- `src/vocab/vocab.cpp` — `g_vocab_dir` 全局变量及 `load_*` 函数的 mmap 路径逻辑

### API 实现
- `src/api/wan-api.cpp` — 所有公共 API 函数的实现入口，新函数在此实现
- `src/api/wan_config.cpp` — 现有 `wan_params_*` 实现参考，了解 API 实现模式

### CLI
- `examples/cli/wan-cli.cpp` — 现有参数解析逻辑，`--vocab-dir` 需遵循同一模式

### 构建系统
- `CMakeLists.txt` — `WAN_EMBED_VOCAB` 选项定义，了解如何在 CLI 中检测该标志

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `wan_vocab_set_dir(const std::string& dir)`（`src/vocab/vocab.cpp:29`）：内部实现已完整，Phase 12 只需在公共 API 层调用它
- `g_vocab_dir` 全局变量（`src/vocab/vocab.cpp:27`）：`load_*` 函数已检查此变量，路径正确即可工作
- `wan_params_set_*` 系列函数（`src/api/wan_config.cpp`）：新 API 函数的实现模板

### Established Patterns
- 公共 API 函数通过 `WAN_API` 宏导出，`extern "C"` 包裹
- `wan-api.cpp` 是唯一包含内部头文件的翻译单元（ODR 规则）
- CLI 参数解析使用 `getopt_long` 风格，长参数名加 `--` 前缀

### Integration Points
- `wan.h` 需新增 `wan_set_vocab_dir` 声明（"Parameter Configuration" 区块末尾）
- `wan-api.cpp` 需实现该函数，调用 `wan_vocab_set_dir`
- `wan-api.cpp` 的 `wan_load_model` 需在加载前校验 `g_vocab_dir` 目录存在性（当 `WAN_EMBED_VOCAB=OFF` 时）
- `wan-cli.cpp` 需添加 `--vocab-dir` 参数解析，并在 `WAN_EMBED_VOCAB` 未定义时检查是否提供

</code_context>

<specifics>
## Specific Ideas

- `wan_set_vocab_dir` 应在 `wan.h` 的 "Parameter Configuration" 区块中声明，紧跟现有 `wan_params_set_*` 函数之后
- CLI 警告信息示例：`"Warning: WAN_EMBED_VOCAB=OFF build but --vocab-dir not provided; vocab loading will fail"`

</specifics>

<deferred>
## Deferred Ideas

- 每个 context 独立词汇表目录 — 需要 context 绑定 API，超出本阶段范围
- 自动发现词汇表目录（如模型文件同目录）— 未来便利性改进

</deferred>

---

*Phase: 12-wire-vocab-dir-to-public-api*
*Context gathered: 2026-03-17*
