# wan-convert

`wan-convert` converts WAN sub-model safetensors files to GGUF format for use with wan-cpp.

## Building

Enable `WAN_BUILD_EXAMPLES` when configuring:

```bash
cmake -DWAN_BUILD_EXAMPLES=ON ..
make
```

The binary is placed at `build/bin/wan-convert`.

## Usage

```bash
./build/bin/wan-convert --input <file.safetensors> --output <file.gguf> --type <submodel> [--quant <type>]
```

### Example: convert a WAN2.1 DiT checkpoint

```bash
./build/bin/wan-convert \
  --input wan2.1-t2v-1.3B/diffusion_pytorch_model.safetensors \
  --output wan2.1-t2v-1.3B.gguf \
  --type dit-t2v \
  --quant f16
```

## Sub-model Types

| `--type` value | Sub-model | Status | Notes |
|----------------|-----------|--------|-------|
| `dit-t2v` | WAN2.1 DiT | Loadable | Accepted by `wan_load_model` |
| `dit-i2v` | WAN2.2 DiT | Loadable | Accepted by `wan_load_model` |
| `dit-ti2v` | WAN2.2 DiT | Loadable | Accepted by `wan_load_model` |
| `vae` | VAE encoder/decoder | Reserved | Future multi-file loading |
| `t5` | T5 text encoder | Reserved | Future multi-file loading |
| `clip` | CLIP image encoder | Reserved | Future multi-file loading |

## Limitations

`vae/t5/clip` types produce valid GGUF files but `wan_load_model` currently expects a single DiT checkpoint. Multi-file loading is planned for a future release.

## Quantisation Types

| `--quant` value | Description |
|-----------------|-------------|
| `f32` | 32-bit float (largest, highest precision) |
| `f16` | 16-bit float (default) |
| `q8_0` | 8-bit quantised |
| `q4_0` | 4-bit quantised |
| `q4_1` | 4-bit quantised (variant) |
