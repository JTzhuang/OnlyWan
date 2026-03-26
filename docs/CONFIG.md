# Configuration Guide for OnlyWan

OnlyWan uses a two-layer configuration system to separate user-specific environment settings from model-specific architectural parameters.

## Configuration Files

### 1. Main Configuration (`config.json`)
This file defines the paths to model weights and runtime environment settings (like GPU usage).

```json
{
  "wan_config": "path/to/wan_config.json",
  "transformer_path": "path/to/transformer.bin",
  "vae_path": "path/to/vae.bin",
  "text_encoder_path": "path/to/text_encoder.bin",
  "clip_path": "path/to/clip.bin",
  "gpu_ids": [0, 1]
}
```

**Fields:**
- `wan_config`: (Required) Path to the model architecture configuration.
- `transformer_path`: (Required) Path to the Transformer model weights.
- `vae_path`: (Required) Path to the VAE model weights.
- `text_encoder_path`: (Required) Path to the Text Encoder model weights.
- `clip_path`: (Optional) Path to the CLIP model weights. If omitted, CLIP-based features will be disabled.
- `gpu_ids`: (Optional) List of GPU IDs to use for generation. Defaults to `[0]` if not specified.

### 2. Model Configuration (`wan_config.json`)
This file contains the architectural parameters of the Wan model (e.g., WAN2.1 T2V 1.3B).

```json
{
  "model_type": "t2v_1.3b",
  "t5_model_type": "umt5",
  "vae_model_type": "wan_vae",
  "dim": 1536,
  "n_layers": 30,
  "n_heads": 12,
  "max_seq_len": 512
}
```

**Key Fields:**
- `model_type`: Determines the generation mode (`t2v_1.3b`, `i2v_14b`, etc.).
- `t5_model_type`: Type of text encoder used (`t5` or `umt5`).

## CLI Usage

The `wan-cli` now accepts a single positional argument for the configuration file. The `-m` (model path) parameter has been removed.

```bash
# Run text-to-video generation
./wan-cli config.json --prompt "A beautiful sunset over the ocean" --output sunset.avi
```

## API Usage

The C-API has been updated to use `config_path`.

```c
#include <wan-cpp/wan.h>

wan_context_t* ctx = NULL;
wan_error_t err = wan_load_model("path/to/config.json", 0, "cuda", &ctx);

if (err != WAN_SUCCESS) {
    // Handle error
}
```

## Error Handling

OnlyWan uses detailed logging to help diagnose configuration issues:
- **Missing Files**: If `transformer_path` or other required files are missing, an `ERROR` is logged with the specific path.
- **CLIP Optional**: If `clip_path` is missing, a `WARN` is logged, and the model continues loading (some features may be limited).
- **JSON Parsing**: Syntax errors in configuration files will be reported with the line number.

## Multi-GPU Support

By specifying multiple IDs in `gpu_ids`, OnlyWan will automatically distribute the workload across the selected GPUs.

```json
"gpu_ids": [0, 1, 2, 3]
```
