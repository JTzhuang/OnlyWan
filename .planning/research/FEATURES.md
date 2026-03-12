# Feature Landscape

**Domain:** C++ AI Inference Library (Video Generation)
**Researched:** 2026-03-12
**Overall confidence:** MEDIUM

## Table Stakes

Features users expect. Missing = product feels incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Model loading from GGUF files | Standard format for GGML-based libraries, enables weight management | Medium | Must support WAN2.1, WAN2.2 variants |
| Basic inference API (T2V, I2V, FLF2V) | Core functionality, primary use case | High | Text-to-video, image-to-video, flow-latent-flow-to-video |
| Configuration parameters | Control generation quality, speed, output | Low | Seed, steps, guidance scale, resolution |
| Memory management | Prevent OOM on resource-constrained devices | High | Weight offloading to CPU, tensor allocation |
| Multi-backend support (CUDA, Metal, Vulkan, CPU) | Cross-platform compatibility, hardware choice | Medium | GGML backends are standard |
| CLI interface | Quick testing, validation, user acceptance | Low | Command-line argument parsing |
| Basic examples/demo code | Onboarding, proof of functionality | Low | Simple generation examples |
| CMake build system | Standard C++ project build, easy integration | Medium | Support multiple platforms |
| Video output (AVI export) | Save generated video, basic expectation | Low | Already has avi_writer.h |
| Version detection | Differentiate model variants, correct behavior | Low | WAN2.1 vs WAN2.2.2 (TI2V) |
| Tensor format support | Input preprocessing, output postprocessing | Medium | Float16, Float32 conversion |
| Error handling | Graceful failures, debugging support | Medium | Clear error messages, validation |

## Differentiators

Features that set product apart. Not expected, but valued.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Progress callbacks | Enable UI integration, user feedback | Medium | Callbacks during denoising steps |
| Preview generation during inference | Real-time visualization, better UX | High | TAE/VAE decoding at intervals |
| Batch generation support | Multiple videos in one run, efficiency | Medium | Process multiple prompts/images |
| Memory pooling | Reduce allocation overhead, better performance | Medium | Reuse tensors across generations |
| C-style API wrapper | Language bindings (Python, Rust, etc.) | Low | extern "C" wrapper around C++ API |
| Streaming output | Large video generation without memory explosion | High | Write frames as they're generated |
| Circular padding support | Better temporal consistency | Medium | For video continuity |
| Spectrum caching | Faster generation for similar prompts | Medium | Cache intermediate features |
| Custom tensor operations | Flexibility for advanced users | High | Allow custom GGML operations |
| RAII-based resource management | Automatic cleanup, memory safety | Low | Smart pointers for tensors/models |
| Verbose logging mode | Debugging, performance tuning | Low | Optional detailed output |

## Anti-Features

Features to explicitly NOT build.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Training capabilities | Library is inference-only, training is separate concern | Use PyTorch/official training repo |
| Weight conversion tools | Conversion is one-time, not core inference | Use existing convert script from parent repo |
| Other Stable Diffusion models (SD1, SD2, SDXL, Flux) | Keeps library focused, reduces bloat | Use parent stable-diffusion.cpp |
| Network server/API | Library consumers build their own servers | Example: use libtorch serving patterns |
| GUI application | Library is not an application, allows flexibility | Build consumer apps with Qt/Electron/etc |
| Image generation (static) | Library focused on video generation specifically | Use other stable-diffusion.cpp variants |
| Dynamic model switching during inference | Complex, error-prone, rarely needed | Load separate instances per model |
| Automatic model downloading | Security concerns, network dependency, complexity | Document manual download workflow |
| Plugin system | Over-engineering for this scope | Build into consumer application if needed |
| Serialization of internal state | GGUF weights already handle this | Use GGUF format for all persistence |

## Feature Dependencies

```
GGML backend initialization → Model loading → Inference execution
Model loading → Weight offloading → Memory management
Inference execution → Progress callbacks (optional) → Video output
Progress callbacks → Preview generation → Video output (optional)
Configuration parameters → Model loading
Configuration parameters → Inference execution
Version detection → Model loading (variant-specific behavior)
C-style API wrapper → C++ core API
Memory pooling → Multiple inference calls
```

## MVP Recommendation

**Phase 1 (Core Foundation):**
1. Model loading from GGUF files
2. Basic inference API (T2V, I2V)
3. Configuration parameters (seed, steps, resolution)
4. Memory management (basic weight loading)
5. CMake build system (single backend initially)
6. CLI interface (basic generation)

**Phase 2 (Production Ready):**
7. Multi-backend support (CUDA, Metal, Vulkan)
8. Version detection (WAN2.1, WAN2.2 variants)
9. Video output (AVI export)
10. Error handling and validation
11. Basic examples and documentation

**Phase 3 (Enhanced Experience):**
12. Progress callbacks
13. Preview generation
14. C-style API wrapper
15. Verbose logging

**Defer:**
- Streaming output: Can be added in later version, not blocking MVP
- Memory pooling: Optimization, not functional requirement
- Batch generation: Use multiple instances for now
- Spectrum caching: Complex feature, optional improvement

## Complexity Notes

**Low complexity:** Configuration, CLI, version detection, logging, RAII
- Typical implementation time: 1-2 days each
- Dependencies: Minimal

**Medium complexity:** Model loading, multi-backend, progress callbacks, C-style API
- Typical implementation time: 3-5 days each
- Dependencies: May require GGML backend knowledge

**High complexity:** Inference API, memory management, preview generation, streaming
- Typical implementation time: 1-2 weeks each
- Dependencies: Deep GGML knowledge, model architecture understanding

## Video Generation Specific Features

**WAN Model Modes:**
- T2V (Text-to-Video): Generate video from text prompt
- I2V (Image-to-Video): Generate video from input image
- FLF2V (Flow-Latent-Flow-to-Video): Advanced mode using flow matching
- VACE (Video Animation Enhancement): Enhance existing videos
- TI2V (Text+Image-to-Video): Generate video from both text and image

**Temporal Features:**
- Circular padding: Maintain temporal continuity across video frames
- Time-aware convolution: 3D convolutions with temporal dimension
- Feature caching: Cache intermediate temporal features for efficiency

## Sources

- [stable-diffusion.cpp GitHub repository](https://github.com/FasterAI/stable-diffusion.cpp) (LOW confidence - WebSearch only)
- [llama.cpp standalone library patterns](https://github.com/ggerganov/llama.cpp) (LOW confidence - WebSearch only)
- [ONNX Runtime C++ API documentation](https://onnxruntime.ai/docs/api/c-api/) (LOW confidence - WebSearch only)
- Local code inspection of stable-diffusion.cpp/wan.hpp (HIGH confidence - source code)
- Local code inspection of examples/cli/main.cpp (HIGH confidence - source code)
- Local code inspection of src/model.h (HIGH confidence - source code)

**Gap noted:** Official documentation for C++ AI inference libraries was not directly accessible via WebFetch due to network restrictions. Findings are based on WebSearch results and direct code inspection of the parent repository. Validation through Context7 was not available for these libraries.
