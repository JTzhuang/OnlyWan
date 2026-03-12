# Architecture Research

**Domain:** Standalone C++ Library Extraction
**Researched:** 2026-03-12
**Confidence:** HIGH

## Standard Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                        Public API Layer                       │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ Model   │  │ Config  │  │ Context │  │ Types   │        │
│  │ Loader  │  │ Builder │  │ Manager │  │ Enums   │        │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘        │
│       │            │            │            │              │
├───────┴────────────┴────────────┴────────────┴──────────────┤
│                        WAN Core Layer                         │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────┐    │
│  │         WAN Model Implementation (wan.hpp)          │    │
│  └─────────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│                      Supporting Modules                       │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │   VAE   │  │  Rope   │  │  Flux   │  │ Common  │        │
│  │ Encoder │  │  Pos    │  │  Flow   │  │ Blocks  │        │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘        │
│       │            │            │            │              │
├───────┴────────────┴────────────┴────────────┴──────────────┤
│                      Utility Layer                            │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ GGML    │  │ Preproc │  │   RNG   │  │ Tensor  │        │
│  │ Extend  │  │ essing  │  │  Utils  │  │ Utils   │        │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘        │
├─────────────────────────────────────────────────────────────┤
│                      External Dependencies                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │ ggml (submod)│  │ thirdparty   │  │ std::filesystem │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| Public API | C-compatible interface for library consumers | C API with opaque handles and callbacks |
| Model Loader | Loads WAN model weights from GGUF files | GGUF parsing, memory allocation |
| Config Builder | Build configuration for inference | Builder pattern with fluent API |
| Context Manager | Manages inference context and memory pools | RAII with cleanup handlers |
| WAN Model | Core video generation model implementation | Class hierarchy of DiT blocks |
| VAE Encoder | Variational Autoencoder for video encoding | Convolutional encoder-decoder |
| Rope Pos | Rotary position embedding for attention | Trigonometric position encoding |
| Flux Flow | Flow matching implementation | ODE solvers for diffusion |
| GGML Extend | GGML tensor operations extensions | Custom ops padded on ggml core |

## Recommended Project Structure

```
wan-cpp/
├── CMakeLists.txt              # Root CMake configuration
├── LICENSE                     # License file
├── README.md                   # Project documentation
├── .gitignore                  # Git ignore patterns
├── .gitmodules                 # Git submodules (ggml)
├── .clang-format               # Code formatting rules
│
├── include/                    # Public headers (installed location)
│   └── wan-cpp/
│       ├── wan.h               # Main public API (C-compatible)
│       ├── config.h            # Configuration types
│       ├── types.h             # Type definitions and enums
│       └── version.h           # Version information
│
├── src/                        # Private implementation
│   ├── wan.hpp                 # WAN model core implementation
│   ├── common_block.hpp        # Shared block implementations
│   ├── rope_pos.hpp            # Rotary position encoding
│   ├── vae_encoder.hpp         # VAE encoder/decoder
│   ├── flow_matching.hpp       # Flow matching (extracted from flux.hpp)
│   ├── ggml_extend.hpp         # GGML extensions
│   ├── preprocessing.hpp       # Image/video preprocessing
│   ├── model_loader.cpp        # Model loading logic
│   ├── context_manager.cpp     # Context and memory management
│   ├── config_builder.cpp      # Configuration builder
│   ├── rng.hpp                 # Random number generation
│   └── util.h/cpp              # Utility functions
│
├── thirdparty/                 # Third-party dependencies
│   ├── CMakeLists.txt          # Third-party build config
│   ├── json.hpp                # JSON parsing
│   ├── stb_image.h             # Image loading
│   ├── stb_image_write.h       # Image writing
│   └── zip.h/miniz.h           # ZIP compression
│
├── ggml/                       # Git submodule (GGML library)
│   ├── CMakeLists.txt
│   ├── include/
│   └── src/
│
├── examples/                   # Example programs
│   ├── CMakeLists.txt
│   ├── cli/
│   │   └── main.cpp            # Command-line example
│   └── common/
│       └── common.hpp          # Shared example utilities
│
├── tests/                      # Unit tests (optional)
│   ├── CMakeLists.txt
│   └── test_*.cpp
│
└── .planning/                  # Planning artifacts (not installed)
    └── research/               # Research documents
```

### Structure Rationale

- **include/wan-cpp/**: Public headers organized with namespace prefix to avoid conflicts. This is the installed location for users.
- **src/**: All private implementation remains internal. Using .hpp for template-heavy code, .cpp for compiled code.
- **thirdparty/**: Header-only dependencies isolated for easier maintenance and upgrades.
- **ggml/**: Git submodule for staying synchronized with upstream GGML. Independent release cycle.
- **examples/**: Demonstrates usage and serves as integration tests.
- **tests/**: Optional test suite using a common testing framework.

## Architectural Patterns

### Pattern 1: C API with Opaque Handles

**What:** Expose a C-compatible API that hides C++ implementation details using opaque pointers/handles.

**When to use:** When creating a library that may be consumed by different languages or when you need ABI stability.

**Trade-offs:**
- Pros: ABI stability, language-agnostic, no name mangling issues
- Cons: Verbose error handling, no exceptions, manual memory management

**Example:**
```c
// Public API (include/wan-cpp/wan.h)
typedef struct wan_context wan_context_t;

WAN_API wan_context_t* wan_create_context(const wan_config_t* config);
WAN_API void wan_free_context(wan_context_t* ctx);
WAN_API int wan_generate_video(wan_context_t* ctx,
                               const wan_input_t* input,
                               wan_output_t* output);
```

```cpp
// Private implementation (src/wan.cpp)
struct wan_context {
    // All private members here
    std::unique_ptr<WAN::WANModel> model;
    std::unique_ptr<GGMLRunnerContext> ggml_ctx;
};

wan_context_t* wan_create_context(const wan_config_t* config) {
    try {
        auto ctx = new wan_context();
        ctx->model = std::make_unique<WAN::WANModel>(config);
        return ctx;
    } catch (const std::exception& e) {
        return nullptr;  // Error via null return
    }
}
```

### Pattern 2: Builder Pattern for Configuration

**What:** Provide a fluent builder interface for complex configuration objects.

**When to use:** When configuration has many optional parameters with default values.

**Trade-offs:**
- Pros: Readable code, easy to add parameters, type-safe defaults
- Cons: More boilerplate, additional classes

**Example:**
```cpp
class ConfigBuilder {
    WanConfig config;
public:
    ConfigBuilder() {
        // Set defaults
        config.model_path = "";
        config.num_steps = 50;
        config.guidance_scale = 7.5f;
        config.width = 640;
        config.height = 480;
        config.num_frames = 16;
    }

    ConfigBuilder& model_path(const std::string& path) {
        config.model_path = path;
        return *this;
    }

    ConfigBuilder& steps(int n) {
        config.num_steps = n;
        return *this;
    }

    WanConfig build() const {
        if (config.model_path.empty()) {
            throw std::runtime_error("Model path required");
        }
        return config;
    }
};

// Usage
auto config = ConfigBuilder()
    .model_path("wan2.1.gguf")
    .steps(30)
    .width(512)
    .height(512)
    .build();
```

### Pattern 3: RAII Resource Management

**What:** Use Resource Acquisition Is Initialization for all resources (memory, file handles, GPU buffers).

**When to use:** Always in C++ - automatic cleanup is essential for error safety.

**Trade-offs:**
- Pros: No memory leaks, exception-safe, predictable cleanup
- Cons: Requires careful ownership semantics

**Example:**
```cpp
class GGMLRunnerContext {
    ggml_context* ctx;
    ggml_cgraph* graph;
    void* buffer;

public:
    GGMLRunnerContext(size_t buffer_size) {
        ggml_init_params params = {
            .mem_size = buffer_size,
            .mem_buffer = nullptr,
            .no_alloc = false
        };
        ctx = ggml_init(params);
        graph = ggml_new_graph(ctx);
        buffer = malloc(buffer_size);
    }

    ~GGMLRunnerContext() {
        ggml_free(ctx);
        free(buffer);
    }

    // Non-copyable
    GGMLRunnerContext(const GGMLRunnerContext&) = delete;
    GGMLRunnerContext& operator=(const GGMLRunnerContext&) = delete;
};
```

### Pattern 4: Submodule Dependency Management

**What:** Use git submodules for external dependencies that are actively developed.

**When to use:** When you need to track specific versions of a dependency and want to update it independently.

**Trade-offs:**
- Pros: Version control, independent releases, easy updates
- Cons: Clone complexity, requires subcommand awareness

**Example:**
```cmake
# CMakeLists.txt
option(WAN_USE_SYSTEM_GGML "Use system-installed GGML" OFF)

if (WAN_USE_SYSTEM_GGML)
    find_package(ggml REQUIRED)
    add_library(ggml ALIAS ggml::ggml)
else()
    # Add ggml as a subdirectory
    add_subdirectory(ggml)
endif()

# Link the library
target_link_libraries(wan PRIVATE ggml)
```

## Data Flow

### Request Flow

```
[User Application]
    ↓ (wan_generate_video)
[wan_context_t Handle]
    ↓ (validate input)
[Configuration Builder]
    ↓ (build config)
[WANModel::forward()]
    ↓ (preprocess input)
[Preprocessing Module]
    ↓ (load model weights)
[Model Loader]
    ↓ (compute tensors)
[GGML Compute Graph]
    ↓ (execute on chosen backend)
[Backend: CUDA/Metal/Vulkan/CPU]
    ↓ (postprocess output)
[Output Tensor]
    ↓ (return to user)
[Generated Video Frames]
```

### Build Flow

```
[cmake configure]
    ↓
[Detect GGML submodule]
    ↓
[Configure GGML options (CUDA, Metal, etc.)]
    ↓
[Add wan library target]
    ↓
[Set include directories (include/ src/ thirdparty/)]
    ↓
[Link dependencies: ggml, zip]
    ↓
[Add examples (if BUILD_EXAMPLES)]
    ↓
[Generate build files]
```

### Key Data Flows

1. **Model Loading Flow**: GGUF file parsing -> Tensor memory allocation -> GGML context creation -> Model instantiation
2. **Video Generation Flow**: Input validation -> Preprocessing -> Denoising loop -> VAE decoding -> Frame extraction
3. **Backend Selection Flow**: CMake option -> Preprocessor defines -> Conditional includes -> Backend-specific GGML calls

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|--------------------------|
| Single video generation | Single-threaded CPU backend is sufficient |
| Batch generation | Use GPU backend (CUDA/Metal/Vulkan) for parallelism |
| Multiple concurrent requests | Create multiple wan_context_t instances with thread-local GGML backends |
| Production service | Add thread pool, request queue, and connection pooling for network API |

### Scaling Priorities

1. **First bottleneck:** Memory allocation for large video tensors
   - Fix: Implement tensor pooling, reuse GGML contexts, use quantization
2. **Second bottleneck:** CPU-only backend for generation
   - Fix: Add GPU backend support via GGML's CUDA/Metal/Vulkan backends
3. **Third bottleneck:** Model loading time
   - Fix: Implement lazy loading, cache parsed models, use memory mapping

## Anti-Patterns

### Anti-Pattern 1: Leaking Implementation Details in Public Headers

**What people do:** Putting template implementations, internal classes, or third-party dependencies in the public include directory.

**Why it's wrong:** Causes compilation dependency hell for consumers, breaks ABI stability, forces exposure of internal headers.

**Do this instead:**
```cpp
// ✅ CORRECT: include/wan-cpp/wan.h
typedef struct wan_context wan_context_t;  // Opaque forward declaration

WAN_API wan_context_t* wan_create_context();

// ❌ WRONG: Don't do this
#include "src/wan.hpp"  // Exposes entire implementation
```

### Anti-Pattern 2: Mixing C and C++ Error Handling

**What people do:** Throwing exceptions from C API functions or mixing errno with exceptions.

**Why it's wrong:** C consumers can't catch exceptions, errno handling is inconsistent, crash risk.

**Do this instead:**
```cpp
// ✅ CORRECT: Return error codes, optional error message
WAN_API int wan_generate_video(wan_context_t* ctx,
                               const wan_input_t* input,
                               wan_output_t* output,
                               char* error_buffer,
                               size_t error_size);

// ❌ WRONG
WAN_API void wan_generate_video(...) {
    throw std::runtime_error("error");  // C can't catch this
}
```

### Anti-Pattern 3: Hardcoding Paths and Constants

**What people do:** Hardcoding model paths, buffer sizes, or GPU settings in the library code.

**Why it's wrong:** Not portable, inflexible, requires recompilation for different environments.

**Do this instead:**
```cpp
// ✅ CORRECT: Use configuration and discovery
struct WanConfig {
    std::string model_path;           // User-provided
    size_t buffer_size = 512 * 1024 * 1024;  // Configurable default
    bool use_gpu = true;             // Auto-detect with option
};

// ❌ WRONG
constexpr size_t BUFFER_SIZE = 512 * 1024 * 1024;
std::string MODEL_PATH = "/models/wan2.1.gguf";
```

### Anti-Pattern 4: Coupling to Stable-Diffusion.cpp

**What people do:** Keeping references to SD-specific code, types, or patterns from the original monolithic project.

**Why it's wrong:** Defeats the purpose of extraction, creates hidden dependencies, confusing for users.

**Do this instead:**
- Create WAN-specific type aliases and enums
- Remove SD-specific constants and defines
- Rename variables and functions to be WAN-focused
- Create a clean separation layer

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| GGML | Submodule + CMake add_subdirectory | Supports multiple backends via CMake options |
| thirdparty libraries | Header-only includes | No build step, just include path |
| Filesystem | std::filesystem | C++17 standard, cross-platform |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Public API <-> Internal | C function calls with opaque handles | Maintains ABI stability |
| WAN Module <-> GGML | Direct GGML API calls | Minimal abstraction layer for performance |
| Examples <-> Library | Public API only | No internal dependencies |
| Tests <-> Library | Public API + test-only internal header | White-box testing via friend classes or test-only includes |

## Build System Architecture

### CMake Configuration Levels

```
Level 1: Root CMakeLists.txt
    ├─ Detect build environment
    ├─ Define options (WAN_USE_CUDA, WAN_BUILD_EXAMPLES, etc.)
    ├─ Configure GGML submodule
    ├─ Add thirdparty subdirectory
    ├─ Add library target (wan)
    └─ Conditionally add examples and tests

Level 2: src/CMakeLists.txt (if separate)
    ├─ Define library target
    ├─ Add source files
    ├─ Set include directories
    ├─ Link dependencies
    └─ Install targets

Level 3: examples/CMakeLists.txt
    ├─ Define example executables
    ├─ Link to library
    └─ Install (optional)

Level 4: thirdparty/CMakeLists.txt
    ├─ Define thirdparty targets (zip)
    └─ Set include directories
```

### Recommended Build Order

For the extraction process:

1. **Phase 1: Skeleton Creation**
   - Create directory structure
   - Set up CMakeLists.txt skeleton
   - Add .gitmodules for ggml
   - Create placeholder public headers

2. **Phase 2: Dependencies Setup**
   - Clone/initialize ggml submodule
   - Copy thirdparty directory
   - Verify ggml builds independently
   - Configure CMake options

3. **Phase 3: Core Extraction**
   - Extract src/wan.hpp and direct dependencies
   - Extract src/common_block.hpp
   - Extract src/rope.hpp
   - Extract src/vae.hpp
   - Extract src/flux.hpp (partial - only needed parts)
   - Extract src/ggml_extend.hpp

4. **Phase 4: Public API Creation**
   - Create include/wan-cpp/wan.h (C API)
   - Create include/wan-cpp/config.h
   - Create include/wan-cpp/types.h
   - Implement wrapper functions in src/api.cpp

5. **Phase 5: Build System Integration**
   - Complete CMakeLists.txt
   - Set up include directories correctly
   - Link ggml and thirdparty targets
   - Configure install rules

6. **Phase 6: Examples and Testing**
   - Extract and adapt example code
   - Create minimal working example
   - Build and test on multiple backends (CPU, GPU)

7. **Phase 7: Cleanup and Polish**
   - Remove SD-specific code
   - Add proper documentation
   - Set up versioning
   - Add CI/CD configuration

## Public API Design Considerations

### API Naming Convention

```c
// All public functions use wan_ prefix
WAN_API wan_context_t* wan_create_context(...);
WAN_API void wan_free_context(wan_context_t* ctx);
WAN_API int wan_generate_video(...);
WAN_API const char* wan_get_version_string(void);

// Types use _t suffix
typedef struct wan_context wan_context_t;
typedef struct wan_config wan_config_t;
typedef struct wan_input wan_input_t;
typedef struct wan_output wan_output_t;

// Enums use wan_ prefix and _t suffix
typedef enum {
    WAN_MODEL_TYPE_2_1,
    WAN_MODEL_TYPE_2_2,
    WAN_MODEL_TYPE_COUNT
} wan_model_type_t;
```

### Memory Management Strategy

1. **Context Allocation**: User creates and destroys via API
2. **Temporary Tensors**: Managed internally by RAII
3. **Output Buffers**: User provides buffer, library fills it
4. **Strings**: Copy to user-provided buffer, return length

### Error Handling Strategy

```c
// Option 1: Return error code with optional message
WAN_API int wan_generate_video(..., char* error_buf, size_t error_size);

// Option 2: Callback for progress and errors
typedef void (*wan_progress_callback)(float progress, const char* message, void* user_data);

WAN_API int wan_generate_video(..., wan_progress_callback callback, void* user_data);
```

## Sources

- [CMake Packaging Guide](https://cmake.org/cmake/help/latest/guide/creating-packages/A-Quick-Start.html) (MEDIUM confidence - standard practice)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/) (MEDIUM confidence - established best practices)
- [GGML Project Structure](https://github.com/ggml-org/ggml) (HIGH confidence - analyzed actual code)
- [stable-diffusion.cpp Analysis](/home/jtzhuang/projects/stable-diffusion.cpp) (HIGH confidence - analyzed actual codebase)

---
*Architecture research for: Standalone C++ Library Extraction*
*Researched: 2026-03-12*
