# wan-cpp Examples

This directory contains example programs demonstrating how to use the wan-cpp library.

## Building Examples

To build the examples, enable the `WAN_BUILD_EXAMPLES` option:

```bash
cmake -DWAN_BUILD_EXAMPLES=ON ..
make
```

The compiled executables will be located in the `build/bin/` directory.

## CLI Example (wan-cli)

The `wan-cli` is a command-line interface for video generation using the WAN models.

### Usage

```bash
./build/bin/wan-cli --help
```

### Text-to-Video (T2V)

Generate a video from a text prompt:

```bash
./build/bin/wan-cli \
  --model models/wan2.1-fp16.gguf \
  --prompt "A cat running in a garden" \
  --output output.avi \
  --width 640 \
  --height 480 \
  --frames 16 \
  --steps 30 \
  --cfg 7.0
```

### Image-to-Video (I2V)

Generate a video from an input image:

```bash
./build/bin/wan-cli \
  --model models/wan2.1-fp16.gguf \
  --input input.jpg \
  --prompt "Make the cat jump" \
  --output output.avi \
  --width 640 \
  --height 480 \
  --frames 16 \
  --steps 30 \
  --cfg 7.0
```

### Command-Line Options

#### Required Options

| Option | Description |
|--------|-------------|
| `-m, --model <path>` | Path to WAN model file (GGUF format) |
| `-p, --prompt <text>` | Text prompt for T2V or I2V |

#### Optional Options

| Option | Description | Default |
|--------|-------------|----------|
| `-i, --input <path>` | Input image for I2V mode | (none) |
| `-o, --output <path>` | Output video path | `output.avi` |
| `-b, --backend <type>` | Backend type (cpu, cuda, metal, vulkan) | `cpu` |
| `-t, --threads <num>` | Number of threads | Auto-detect |
| `-W, --width <pixels>` | Output width | `640` |
| `-H, --height <pixels>` | Output height | `480` |
| `-f, --frames <num>` | Number of frames | `16` |
| `--fps <num>` | Frames per second | `16` |
| `-s, --steps <num>` | Sampling steps | `30` |
| `--seed <num>` | Random seed | Random |
| `-c, --cfg <value>` | CFG scale | `7.0` |
| `-n, --negative <text>` | Negative prompt | (none) |
| `-v, --verbose` | Enable verbose output | (off) |
| `-h, --help` | Show help message | - |

### Backends

The CLI supports multiple GPU backends:

- **CPU**: CPU-only inference (default, no special flags needed)
- **CUDA**: NVIDIA GPU support (build with `-DWAN_CUDA=ON`)
- **Metal**: Apple Silicon GPU support (build with `-DWAN_METAL=ON`)
- **Vulkan**: Cross-platform GPU support (build with `-DWAN_VULKAN=ON`)

### Example Usage Patterns

#### High Quality Generation

```bash
./build/bin/wan-cli \
  --model models/wan2.1-fp16.gguf \
  --prompt "A serene ocean sunset" \
  --output high_quality.avi \
  --width 1024 \
  --height 768 \
  --frames 32 \
  --steps 50 \
  --cfg 10.0 \
  --verbose
```

#### Fast Generation

```bash
./build/bin/wan-cli \
  --model models/wan2.1-q4_0.gguf \
  --prompt "A quick demo" \
  --output fast_demo.avi \
  --width 480 \
  --height 360 \
  --frames 8 \
  --steps 15 \
  --cfg 5.0
```

#### Reproducible Generation

```bash
./build/bin/wan-cli \
  --model models/wan2.1-fp16.gguf \
  --prompt "A red flower" \
  --output flower.avi \
  --seed 12345 \
  --steps 30
```

Run the same command again to get identical results.

## Programmatic Usage

See the `examples/cli/main.cpp` source code for a complete example of using the wan-cpp API programmatically.

Key components:

1. **AVI Writer**: `avi_writer.h` - Helper for saving videos in MJPG AVI format
2. **CLI**: `main.cpp` - Complete command-line interface implementation
3. **API**: `wan.h` - Public C API for video generation

## Troubleshooting

### Build Errors

If you encounter build errors, ensure:
- CMake version is 3.20 or higher
- C++17 compiler is available
- Required backend dependencies are installed

### Runtime Errors

**Model not found:**
- Verify the model path is correct
- Ensure the model file is in GGUF format

**CUDA out of memory:**
- Reduce `--width` and `--height`
- Decrease `--frames` for shorter videos
- Use a quantized model (q4_0, q4_1, etc.)

**Slow inference:**
- Try a faster backend (CUDA, Metal, Vulkan)
- Use a quantized model
- Reduce `--steps` (quality vs speed tradeoff)

### Verbose Output

Use `--verbose` flag to see detailed logging:
```bash
./build/bin/wan-cli --model model.gguf --prompt "test" --verbose
```

## License

These examples are part of wan-cpp and follow the same license.
