# Coding Conventions

**Analysis Date:** 2026-03-17

## Naming Patterns

**Files:**
- Header files: `.h` for C headers, `.hpp` for C++ headers
- Implementation files: `.cpp` for C++ source
- Vocabulary/data files: `.hpp` for large data structures (e.g., `clip_t5.hpp`, `umt5.hpp`)
- Example: `wan.hpp`, `model.h`, `util.cpp`

**Functions:**
- snake_case for C-style functions: `ends_with()`, `starts_with()`, `file_exists()`, `log_printf()`
- snake_case for member functions: `forward()`, `init_params()`, `get_tensor_storage_map()`
- Getter/setter pattern: `get_*()`, `set_*()` (e.g., `sd_get_system_info()`, `sd_set_log_callback()`)
- Callback functions: `*_cb_t` suffix for typedef (e.g., `sd_log_cb_t`, `sd_progress_cb_t`)

**Variables:**
- snake_case for local and member variables: `channels`, `out_channels`, `emb_channels`, `kernel_size`
- Underscore suffix for private members: `data_`, `size_`, `version_`, `file_paths_`
- UPPER_CASE for constants and macros: `EPS`, `SD_MAX_DIMS`, `WAN_GRAPH_SIZE`, `CACHE_T`
- Enum values: UPPER_CASE with prefix: `SD_LOG_DEBUG`, `SD_LOG_INFO`, `EULER_SAMPLE_METHOD`, `VERSION_SD1`

**Types:**
- PascalCase for C++ classes: `DownSampleBlock`, `ResBlock`, `CrossAttention`, `ModelLoader`
- snake_case_t suffix for C-style structs: `sd_image_t`, `sd_ctx_params_t`, `sd_sample_params_t`, `TensorStorage`
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
1. Standard library headers: `<cstdint>`, `<memory>`, `<string>`, `<vector>`, `<map>`
2. System headers: `<sys/sysctl.h>`, `<windows.h>`, `<unistd.h>`
3. Third-party headers: `"ggml.h"`, `"ggml-backend.h"`, `"json.hpp"`
4. Project headers: `"model.h"`, `"util.h"`, `"wan-types.h"`

**Path Aliases:**
- No path aliases detected; uses relative includes
- Includes from parent directories: `#include "../../examples/cli/avi_writer.h"`

**Example:**
```cpp
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ggml-backend.h"
#include "ggml.h"
#include "json.hpp"

#include "model.h"
#include "util.h"
#include "wan-types.h"
```

## Error Handling

**Patterns:**
- C-style error handling: functions return `bool` for success/failure
- Error messages stored in context: `ctx->last_error` (string)
- Error setting functions: `set_last_error()`, `set_last_error_fmt()` with printf-style formatting
- Assertions for internal invariants: `GGML_ASSERT()` macro
- No exceptions used (C++ but exception-free style)

**Example:**
```cpp
static void set_last_error(wan_context_t* ctx, const char* error_msg) {
    if (ctx) {
        ctx->last_error = error_msg ? error_msg : "Unknown error";
    }
}

static void set_last_error_fmt(wan_context_t* ctx, const char* fmt, ...) {
    if (!ctx) return;
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    ctx->last_error = buffer;
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
- Explain tensor shape transformations: `// [N, channels, h, w]`
- Mark non-obvious operations: `// undoing tucker decomposition for conv layers`
- Document parameter meanings in complex functions

## Function Design

**Size:** Functions range from 10-50 lines typically; larger functions (100+ lines) are rare and usually involve complex tensor operations

**Parameters:**
- Pass by const reference for large objects: `const std::string&`, `const String2TensorStorage&`
- Pass by pointer for optional parameters: `struct ggml_tensor* emb = nullptr`
- Pass by value for small types: `int`, `float`, `bool`
- Context objects passed as pointers: `GGMLRunnerContext* ctx`, `struct ggml_context* ctx`

**Return Values:**
- `bool` for success/failure operations
- Pointers for tensor operations: `struct ggml_tensor*`
- Shared pointers for managed objects: `std::shared_ptr<GGMLBlock>`
- Void for operations with side effects (logging, callbacks)

**Example:**
```cpp
virtual struct ggml_tensor* forward(GGMLRunnerContext* ctx,
                                    struct ggml_tensor* x,
                                    struct ggml_tensor* emb = nullptr) {
    // Implementation
    return x;
}
```

## Module Design

**Exports:**
- C API in `wan.h` (public interface)
- C++ implementation in `wan-api.cpp` (wraps C++ classes)
- Internal headers: `wan-internal.hpp` (not exposed)
- Block classes inherit from `GGMLBlock` or `UnaryBlock` base classes

**Inheritance Hierarchy:**
- `GGMLBlock` - Base class for multi-input/output blocks
- `UnaryBlock` - Base class for single-input/output blocks
- Specific blocks: `ResBlock`, `CrossAttention`, `SelfAttention`, etc.

**Barrel Files:**
- No barrel files detected; each module is self-contained
- Includes are explicit and specific

**Example module structure:**
```cpp
// wan.hpp - C++ implementation
namespace WAN {
    class CausalConv3d : public GGMLBlock { /* ... */ };
    class RMS_norm : public UnaryBlock { /* ... */ };
    struct Flux : public GGMLBlock { /* ... */ };
}

// wan-api.cpp - C API wrapper
WAN_API wan_context_t* wan_create_context(const wan_ctx_params_t* params) {
    // Implementation
}
```

## Platform-Specific Code

**Conditional Compilation:**
- Windows: `#ifdef _WIN32` for file operations, DLL exports
- POSIX: `#if !defined(_WIN32)` for Unix-like systems
- macOS: `#if defined(__APPLE__) && defined(__MACH__)`
- GPU backends: `#ifdef SD_USE_CUDA`, `#ifdef SD_USE_METAL`, etc.

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

*Convention analysis: 2026-03-17*
