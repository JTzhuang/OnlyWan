# wan-cpp

A standalone C++ library for WAN video generation inference.

## Overview

This library provides independent, lightweight, cross-platform inference capabilities for WAN2.1 and WAN2.2 series video generation models. It was extracted from the [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp) monorepo to enable easier integration into other projects without the overhead of the full stable-diffusion.cpp ecosystem.

## Features

- Text-to-Video (T2V) generation
- Image-to-Video (I2V) generation
- First-and-Last-Frame-to-Video (FLF2V) generation
- Video-Audio-Conditioned-Enhancement (VACE) generation
- Text-Image-to-Video (TI2V) generation
- Multiple hardware backends (CPU, CUDA, Metal, Vulkan)
- GGUF model format support
- Efficient memory management via ggml

## Requirements

- C++17 compatible compiler
- CMake 3.20+
- ggml (included as dependency)

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

See `examples` directory for sample programs demonstrating how to use the library for video generation.

## License

This project follows the same license as stable-diffusion.cpp.

## Acknowledgments

This library is extracted from [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp) by leejet.
