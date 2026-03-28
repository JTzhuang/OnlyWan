# Coding Conventions

**Analysis Date:** 2026-03-28

## Naming Patterns

**Files:**
- Header files: `.h` for C headers, `.hpp` for C++ headers
- Implementation files: `.cpp` for C++ source
- Vocabulary/data files: `.hpp` for large data structures (e.g., `clip.hpp`, `t5.hpp`, `wan.hpp`)
- Test files: `test_*.cpp` in `tests/cpp/` directory
- Example: `wan.hpp`, `model.h`, `util.cpp`, `test_transformer.cpp`

**Functions:**
- snake_case for C-style functions: `ends_with()`, `starts_with()`, `file_exists()`, `log_printf()`
- snake_case for member functions: `forward()`, `compute()`, `init_params()`, `get_desc()`, `alloc_params_buffer()`
- Getter/setter pattern: `get_*()`, `set_*()` (e.g., `get_desc()`, `get_tensor_storage_map()`)
- Callback functions: `*_cb_t` suffix for typedef (e.g., `sd_log_cb_t`, `sd_progress_cb_t`)
- Factory functions: `create()` template method in `ModelRegistry` for model instantiation

**Variables:**
- snake_case for local and member variables: `channels`, `out_channels`, `emb_channels`, `kernel_size`, `n_threads`
- Underscore suffix for private members: `data_`, `size_`, `version_`, `file_paths_`, `backend_`
- UPPER_CASE for constants and macros: `EPS`, `SD_MAX_DIMS`, `WAN_GRAPH_SIZE`, `CACHE_T`
- Enum values: UPPER_CASE with prefix: `SD_LOG_DEBUG`, `SD_LOG_INFO`, `VERSION_WAN2`, `VERSION_WAN2_2_I2V`

**Types:**
- PascalCase for C++ classes: `DownSampleBlock`, `ResBlock`, `CrossAttention`, `ModelLoader`, `WanRunner`, `T5Runner`, `CLIPTextModelRunner`
- snake_case_t suffix for C-style structs: `sd_image_t`, `sd_ctx_params_t`, `TensorStorage`, `GGMLRunnerContext`
- Enum names: PascalCase or snake_case_t: `SDVersion`, `PMVersion`, `sd_type_t`, `sd_log_level_t`

## Code Style

**Formatting:**
- Tool: clang-format (configured in `.clang-format`)
- Indentation: 4 spaces (no tabs)
- Column limit: 0 (no hard limit)
- Brace style: Chromium style (opening brace on same line)
- Access modifier offset: -4 spaces (aligns with class keyword)

**Linting:**
- Tool: clang-tidy (configured in `.clang-tidy`)
- Enabled checks: modernize-* rules (make_shared, use_nullptr, use_override, pass_by_value, return_braced_init_list, deprecated_headers)
- Header filter: disabled (empty regex)
- Warnings as errors: disabled

**Example formatting:**
```cpp
class ResBlock : public GGMLBlock {
protected:
    int64_t channels;
    int64_t emb_channels;
    int64_t out_channels;

public:
    ResBlock(int64_t channels,
             int64_t emb_channels,
             int64_t out_channels)
        : channels(channels),
          emb_channels(emb_channels),
          out_channels(out_channels) {}
};
```

## Import Organization

**Order:**
1. Standard library headers: `<cstdint>`, `<memory>`, `<string>`, `<vector>`, `<map>`, `<filesystem>`
2. System headers: `<sys/sysctl.h>`, `<windows.h>`, `<unistd.h>`
3. Third-party headers: `"ggml.h"`, `"ggml-backend.h"`, `"json.hpp"`
4. Project headers: `"model.h"`, `"util.h"`, `"wan.hpp"`, `"clip.hpp"`, `"t5.hpp"`
5. Test headers (in test files): `"test_framework.hpp"`, `"test_helpers.hpp"`, `"test_io_utils.hpp"`

**Path Aliases:**
- No path aliases detected; uses relative includes
- Includes from parent directories: `#include "../../examples/cli/avi_writer.h"`

**Example (test file):**
```cpp
#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "test_io_utils.hpp"
#include "model_registry.hpp"
#include "wan.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

#include <filesystem>
namespace fs = std::filesystem;
```

## Error Handling

**Patterns:**
- C-style error handling: functions return `bool` for success/failure
- Error messages stored in context: `ctx->last_error` (string)
- Error setting functions: `set_last_error()`, `set_last_error_fmt()` with printf-style formatting
- Assertions for internal invariants: `GGML_ASSERT()` macro
- No exceptions used in production code (C++ but exception-free style)
- Test code uses exceptions: `std::runtime_error` thrown by assertion macros for test failure reporting

**Example:**
```cpp
static void set_last_error(wan_context_t* ctx, const char* error_msg) {
    if (ctx) {
        ctx->last_error = error_msg ? error_msg : "Unknown error";
    }
}

// Usage
if (!loader.init_from_file(model_path)) {
    set_last_error(ctx.get(), "Failed to load model");
    return nullptr;
}
```

## Logging

**Framework:** Custom logging via `log_printf()` in `util.h`

**Macros:**
- `LOG_DEBUG(format, ...)` - Debug level
- `LOG_INFO(format, ...)` - Info level
- `LOG_WARN(format, ...)` - Warning level
- `LOG_ERROR(format, ...)` - Error level

**Implementation:** Macros call `log_printf(level, __FILE__, __LINE__, format, ...)`

**Callback system:**
- `sd_set_log_callback(sd_log_cb_t cb, void* data)` - Set custom log callback
- Callback signature: `void (*sd_log_cb_t)(enum sd_log_level_t level, const char* text, void* data)`
- Default callback: `ggml_log_callback_default()` in `ggml_extend.hpp` routes to LOG_* macros

**Example:**
```cpp
#define LOG_DEBUG(format, ...) log_printf(SD_LOG_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) log_printf(SD_LOG_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) log_printf(SD_LOG_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) log_printf(SD_LOG_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
```

## Comments

**When to Comment:**
- Complex algorithms: explain the mathematical or algorithmic approach
- Non-obvious tensor operations: document shape transformations
- Workarounds: explain why a non-standard approach is used
- TODO/FIXME: mark incomplete or known issues
- Model-specific details: document version detection logic, layer counts, dimensions

**JSDoc/Doxygen:**
- File-level documentation: `@file`, `@brief` tags (e.g., in `wan-api.cpp`)
- Function documentation: minimal; relies on clear naming
- Example from `wan-api.cpp`:
```cpp
/**
 * @file wan-api.cpp
 * @brief C-style public API implementation for wan-cpp
 *
 * This file implements of C API declared in wan.h.
 */
```

**Inline comments:**
- Explain tensor shape transformations: `// [N, channels, h, w]` or `// [c=16, t=2, h=8, w=8]`
- Mark non-obvious operations: `// undoing tucker decomposition for conv layers`
- Document parameter meanings in complex functions
- Explain ggml dimension ordering: `// ggml dim order: ne[0]=ch, ne[1]=w, ne[2]=h, ne[3]=frames`

## Function Design

**Size:** Functions range from 10-50 lines typically; larger functions (100+ lines) are rare and usually involve complex tensor operations

**Parameters:**
- Pass by const reference for large objects: `const std::string&`, `const String2TensorStorage&`
- Pass by pointer for optional parameters: `struct ggml_tensor* emb = nullptr`
- Pass by value for small types: `int`, `float`, `bool`
- Context objects passed as pointers: `GGMLRunnerContext* ctx`, `struct ggml_context* ctx`, `ggml_backend_t backend`

**Return Values:**
- `bool` for success/failure operations
- Pointers for tensor operations: `struct ggml_tensor*`
- Shared pointers for managed objects: `std::shared_ptr<GGMLBlock>`
- Void for operations with side effects (logging, callbacks)
- `std::unique_ptr<ModelType>` for factory-created models

**Example:**
```cpp
virtual struct ggml_tensor* forward(GGMLRunnerContext* ctx,
                                    struct ggml_tensor* x,
                                    struct ggml_tensor* emb = nullptr) {
    // Implementation
    return x;
}

bool compute(const int n_threads,
             struct ggml_tensor* x,
             struct ggml_tensor* timesteps,
             struct ggml_tensor* context,
             struct ggml_tensor* output) {
    // Implementation
    return true;
}
```

## Module Design

**Exports:**
- C API in `wan.h` (public interface)
- C++ implementation in `wan-api.cpp` (wraps C++ classes)
- Internal headers: `wan-internal.hpp` (not exposed)
- Block classes inherit from `GGMLBlock` or `UnaryBlock` base classes
- Model runners: `WanRunner`, `T5Runner`, `CLIPTextModelRunner`, `WanVAERunner` inherit from `GGMLRunner`

**Inheritance Hierarchy:**
- `GGMLBlock` - Base class for multi-input/output blocks
- `UnaryBlock` - Base class for single-input/output blocks
- `GGMLRunner` - Base class for model runners
- Specific blocks: `ResBlock`, `CrossAttention`, `SelfAttention`, `CausalConv3d`, `RMS_norm`
- Specific runners: `WanRunner` (DiT), `T5Runner` (text encoder), `CLIPTextModelRunner` (text encoder), `WanVAERunner` (VAE)

**Barrel Files:**
- No barrel files detected; each module is self-contained
- Includes are explicit and specific

**Example module structure:**
```cpp
// wan.hpp - C++ implementation
namespace WAN {
    class CausalConv3d : public GGMLBlock { /* ... */ };
    class RMS_norm : public UnaryBlock { /* ... */ };
    class WanRunner : public GGMLRunner { /* ... */ };
    class WanVAERunner : public GGMLRunner { /* ... */ };
}

// wan-api.cpp - C API wrapper
WAN_API wan_context_t* wan_create_context(const wan_ctx_params_t* params) {
    // Implementation
}
```

## Model Registry Pattern

**Registration Mechanism:**
- Centralized in `src/model_factory.cpp` using `REGISTER_MODEL_FACTORY` macro
- Registry defined in `src/model_registry.hpp` as singleton `ModelRegistry`
- Factory functions use lambda expressions to capture model parameters

**Registration Example:**
```cpp
REGISTER_MODEL_FACTORY(CLIPTextModelRunner, "clip-vit-l-14",
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<CLIPTextModelRunner>(
            backend, offload, map, prefix, OPENAI_CLIP_VIT_L_14, /*with_final_ln=*/true);
    })
```

**Model Creation Pattern:**
```cpp
auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>(
    "clip-vit-l-14", guard.backend, false, empty_map, "");
runner->alloc_params_buffer();
```

## Platform-Specific Code

**Conditional Compilation:**
- Windows: `#ifdef _WIN32` for file operations, DLL exports
- POSIX: `#if !defined(_WIN32)` for Unix-like systems
- macOS: `#if defined(__APPLE__) && defined(__MACH__)`
- GPU backends: `#ifdef SD_USE_CUDA`, `#ifdef SD_USE_METAL`, etc.
- Multi-GPU: `#ifdef WAN_USE_MULTI_GPU` for scheduler-based compute

**Example:**
```cpp
#ifdef _WIN32
bool file_exists(const std::string& filename) {
    DWORD attributes = GetFileAttributesA(filename.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}
#else
// POSIX implementation
#endif
```

---

*Convention analysis: 2026-03-28*
