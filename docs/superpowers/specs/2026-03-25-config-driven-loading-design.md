# WanModel 配置文件加载功能设计规范 (Design Spec)

## 1. 概述 (Overview)
本文档定义了 WanModel 加载逻辑的重构方案。目标是将单一 GGUF 文件加载模式转变为由 JSON 配置文件驱动的分布式加载模式。模型路径、运行环境设置（backend, threads, GPU）以及模型架构参数将通过配置文件指定。

## 2. 核心变更 (Core Changes)

### 2.1 配置文件结构
加载过程涉及两个主要的 JSON 配置文件：

#### 2.1.1 主配置文件 (config.json)
用于指定运行环境和各子模型的权重文件路径。
```json
{
  "backend": "cuda",
  "n_threads": 4,
  "gpu_ids": [0, 1],
  "models": {
    "transformer_path": "models/transformer.gguf",
    "vae_path": "models/vae.gguf",
    "text_encoder_path": "models/text_encoder.gguf",
    "clip_path": "models/clip.gguf"
  },
  "wan_config_file": "models/wan_config.json"
}
```

#### 2.1.2 模型架构配置 (wan_config.json)
直接定义 Wan 模型的超参数，不再从权重文件 metadata 中提取。
```json
{
  "_class_name": "WanModel",
  "model_type": "ti2v",
  "dim": 3072,
  "num_heads": 24,
  "num_layers": 30,
  "in_dim": 48,
  "out_dim": 48,
  "text_len": 512,
  "eps": 1e-06,
  "ffn_dim": 14336,
  "freq_dim": 256
}
```

### 2.2 加载逻辑重构 (WanModel::load)
`Wan::WanModel::load(const std::string& config_path)` 的行为将发生根本改变：
1.  **解析主配置**：使用 JSON 库（如 nlohmann/json）解析 `config_path`。
2.  **加载架构参数**：解析 `wan_config_file`，并据此配置 `WanRunner`。
3.  **分布式权重加载**：
    *   分别初始化各组件（Transformer, VAE, Text Encoder, CLIP）。
    *   **必须项**：Transformer, VAE, Text Encoder。如果对应路径缺失或加载失败，则整体加载失败。
    *   **可选项**：CLIP。如果路径缺失，`clip_runner` 将被设为 `nullptr`，系统记录警告但继续运行。
4.  **Backend 初始化**：根据 `config.json` 中的 `backend` 和 `gpu_ids` 初始化计算后端。

### 2.3 CLI 接口更新
CLI 程序将不再支持 `-m` 或 `--model` 参数。
*   **用法变更为**：`./wan-cli <config_json_path> [options]`
*   第一个非选项参数将被解析为配置文件路径。
*   如果 `config.json` 中已指定 `n_threads` 或 `backend`，CLI 参数（如有）将覆盖配置文件的设定。

## 3. 依赖项 (Dependencies)
*   **JSON 库**：使用已有的 `nlohmann/json` 库解析配置文件。✓（已满足）
*   **模型文件格式**：支持 GGUF 和 Safetensors 格式的子模型文件。

## 4. 错误处理 (Error Handling)
*   如果 `config.json` 或 `wan_config.json` 格式错误，返回 `WAN_ERROR_INVALID_ARGUMENT`。
*   如果 VAE 或 Transformer 权重加载失败，返回 `WAN_ERROR_MODEL_LOAD_FAILED`。
*   CLIP 缺失不视为致命错误，仅在 log 中发出警告。

## 5. 验收标准 (Acceptance Criteria)
1.  用户通过 `./wan-cli config.json -p "..."` 能够成功启动并生成视频。
2.  模型类型（t2v/i2v/ti2v）由 `wan_config.json` 决定。
3.  能够分别指定不同路径下的 VAE 和 Transformer 文件。
4.  多 GPU 配置能够通过 `config.json` 正确传递。
