# Phase 19: .npy and .pt File I/O for Test Data Validation - Research

**Researched:** 2026-03-27
**Domain:** C++ binary file I/O for NumPy and PyTorch formats
**Confidence:** HIGH

## Summary

This research investigates C++ libraries for reading/writing .npy (NumPy) and .pt (PyTorch) files to enable test data validation by comparing C++ model outputs against Python reference implementations. The goal is to add lightweight file I/O utilities to the existing test framework without introducing heavy dependencies like LibTorch.

**Primary recommendation:** Use **libnpy** (header-only) for .npy files and implement a **minimal .pt reader** using pickle format parsing or fallback to .npy for test data exchange.

## Standard Stack

### Core Libraries

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| libnpy | Latest (header-only) | .npy file I/O | Zero dependencies, header-only, C++11, actively maintained |
| cnpy | 1.0.0+ | Alternative .npy/.npz I/O | Mature, widely used, requires zlib for .npz |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| LibTorch | 2.x | Full PyTorch C++ API | Only if full model interop needed (NOT recommended for tests) |
| npio | Latest | Minimal .npy reader | Ultra-lightweight alternative to libnpy |
| cnumpy | Latest | .npy + ndarray | If you need both file I/O and array container |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| libnpy | cnpy | cnpy requires zlib dependency, not header-only |
| LibTorch | Custom .pt parser | .pt format is pickle-based, complex to parse correctly |
| .pt files | .npy files | .npy is simpler format, better for test data exchange |

**Installation:**

For libnpy (header-only):
```bash
# Option 1: Direct download
wget https://raw.githubusercontent.com/llohse/libnpy/master/include/npy.hpp -P tests/cpp/
```

For cnpy (compiled):
```bash
# Add as git submodule or FetchContent in CMake
git submodule add https://github.com/rogersce/cnpy thirdparty/cnpy
```

**Version verification:** libnpy is header-only with no versioning system. cnpy latest commit as of 2024 is stable.

## Architecture Patterns

### Recommended Project Structure
```
tests/cpp/
├── test_framework.hpp        # Existing test framework
├── test_helpers.hpp           # Existing RAII helpers
├── test_io_utils.hpp          # NEW: File I/O utilities
├── test_wan_encoder.cpp       # Example test using I/O utils
└── test_data/                 # NEW: Test data directory
    ├── encoder_input.npy
    ├── encoder_output_ref.npy
    └── README.md
```

### Pattern 1: Test Data Validation Workflow

**What:** Load reference data from Python, run C++ model, compare outputs
**When to use:** Validating C++ implementation matches Python reference

**Example:**
```cpp
// tests/cpp/test_io_utils.hpp
#include "npy.hpp"  // libnpy header
#include "ggml_extend.hpp"

// Load .npy file into ggml_tensor
ggml_tensor* load_npy_to_tensor(ggml_context* ctx, const std::string& path) {
    std::vector<unsigned long> shape;
    std::vector<float> data;
    bool fortran_order;

    npy::LoadArrayFromNumpy(path, shape, fortran_order, data);

    // Create ggml tensor with matching shape
    ggml_tensor* tensor = ggml_new_tensor_4d(ctx, GGML_TYPE_F32,
        shape.size() > 0 ? shape[0] : 1,
        shape.size() > 1 ? shape[1] : 1,
        shape.size() > 2 ? shape[2] : 1,
        shape.size() > 3 ? shape[3] : 1
    );

    // Copy data (handle row-major to ggml layout conversion)
    memcpy(tensor->data, data.data(), data.size() * sizeof(float));

    return tensor;
}

// Save ggml_tensor to .npy file
void save_tensor_to_npy(const ggml_tensor* tensor, const std::string& path) {
    std::vector<unsigned long> shape;
    for (int i = 0; i < ggml_n_dims(tensor); i++) {
        shape.push_back(tensor->ne[i]);
    }

    // Extract data from tensor
    size_t nelements = ggml_nelements(tensor);
    std::vector<float> data(nelements);

    for (size_t i = 0; i < nelements; i++) {
        data[i] = ggml_get_f32_1d(tensor, i);
    }

    npy::SaveArrayAsNumpy(path, false, shape.size(), shape.data(), data);
}
```

### Pattern 2: Test Case with Reference Data

**What:** Compare C++ output against Python-generated reference
**When to use:** Regression testing, numerical accuracy validation

**Example:**
```cpp
// tests/cpp/test_wan_encoder.cpp
#include "test_framework.hpp"
#include "test_io_utils.hpp"

void test_encoder_output_matches_python() {
    // Setup
    ggml_init_params params = {/*.mem_size=*/10*1024*1024, /*.mem_buffer=*/nullptr};
    ggml_context* ctx = ggml_init(params);

    // Load input from Python
    ggml_tensor* input = load_npy_to_tensor(ctx, "tests/cpp/test_data/encoder_input.npy");

    // Run C++ model
    auto encoder = create_encoder(...);
    ggml_tensor* output = encoder->forward(input);

    // Load Python reference output
    ggml_tensor* expected = load_npy_to_tensor(ctx, "tests/cpp/test_data/encoder_output_ref.npy");

    // Compare with tolerance
    float max_diff = 0.0f;
    for (size_t i = 0; i < ggml_nelements(output); i++) {
        float diff = std::abs(ggml_get_f32_1d(output, i) - ggml_get_f32_1d(expected, i));
        max_diff = std::max(max_diff, diff);
    }

    WAN_ASSERT_TRUE(max_diff < 1e-4);  // Tolerance for float32

    ggml_free(ctx);
}
```

### Anti-Patterns to Avoid

- **Linking full LibTorch for tests:** Adds 500MB+ dependency, slow compile times, overkill for simple I/O
- **Parsing .pt files directly:** PyTorch .pt format is pickle-based and complex; use .npy for test data instead
- **Ignoring memory layout:** NumPy uses row-major (C order), ggml tensors may need transposition
- **Not handling dtype mismatches:** Always verify .npy dtype matches ggml tensor type

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| .npy file parsing | Custom binary parser | libnpy or cnpy | .npy format has endianness, dtype, shape metadata - easy to get wrong |
| .pt file loading | Pickle parser in C++ | Save as .npy from Python | .pt is Python pickle format, extremely complex to parse correctly |
| Tensor comparison | Manual element loops | Existing `ggml_ext_tensor_diff` | Already exists in ggml_extend.hpp with gap tolerance |
| Test data generation | C++ random data | Python scripts | Python ecosystem has better tools for generating reference data |

**Key insight:** The .npy format is well-documented and simple (header + raw data), but .pt format is Python-specific pickle serialization with complex object graphs. For test validation, convert .pt to .npy in Python preprocessing.

## Common Pitfalls

### Pitfall 1: Memory Layout Mismatch (Row-Major vs Column-Major)

**What goes wrong:** NumPy arrays are row-major (C order) by default, but ggml tensors use a different indexing scheme. Direct memcpy can produce incorrect results.

**Why it happens:** Different libraries use different memory layouts for multi-dimensional arrays.

**How to avoid:**
- Always verify array order with `fortran_order` flag from libnpy
- For ggml tensors, understand the `ne[]` (dimensions) and `nb[]` (strides) layout
- Use `ggml_ext_tensor_get_f32()` and `ggml_ext_tensor_set_f32()` for safe element access
- Test with known data (e.g., sequential integers) to verify layout correctness

**Warning signs:**
- Test passes for 1D arrays but fails for 2D+
- Transposed-looking output
- First/last elements correct but middle scrambled

### Pitfall 2: Data Type Mismatches

**What goes wrong:** Loading float64 .npy into float32 ggml tensor, or vice versa, causes precision loss or crashes.

**Why it happens:** NumPy supports many dtypes (float16, float32, float64, int32, etc.), ggml has its own type system.

**How to avoid:**
- Check .npy dtype before loading: libnpy provides dtype info
- Convert in Python if needed: `arr.astype(np.float32)` before saving
- Add dtype validation in load function
- Document expected dtypes in test data README

**Warning signs:**
- Segfaults when loading
- Wildly incorrect numerical values
- Size mismatches between file and tensor

### Pitfall 3: LibTorch Dependency Bloat

**What goes wrong:** Adding LibTorch to read .pt files balloons binary size by 500MB+ and compile time by minutes.

**Why it happens:** LibTorch is a full ML framework with CUDA support, autograd, etc.

**How to avoid:**
- Use .npy format for test data exchange instead of .pt
- If .pt files exist, convert them in Python: `torch.save(tensor.numpy(), 'file.npy')`
- Only use LibTorch if you need full PyTorch model interop (not just test data)

**Warning signs:**
- CMake configure takes >5 minutes
- Binary size jumps from ~10MB to >500MB
- Link errors about CUDA symbols when not using GPU

### Pitfall 4: Forgetting .npz Compression

**What goes wrong:** Using cnpy for .npz files but forgetting to link zlib, causing link errors.

**Why it happens:** .npz is zip-compressed .npy, requires zlib dependency.

**How to avoid:**
- Use .npy (uncompressed) for test data - simpler and faster for small files
- If using .npz, ensure zlib is linked in CMakeLists.txt
- Or use libnpy which doesn't support .npz (forces you to use .npy)

**Warning signs:**
- Undefined reference to `inflate`, `deflate` during linking
- cnpy compiles but crashes at runtime when loading .npz

## Code Examples

Verified patterns from research:

### Loading .npy File with libnpy

```cpp
// Source: https://github.com/llohse/libnpy
#include "npy.hpp"

std::vector<float> load_npy_data(const std::string& path) {
    std::vector<unsigned long> shape;
    std::vector<float> data;
    bool fortran_order;

    npy::LoadArrayFromNumpy(path, shape, fortran_order, data);

    // Verify expected shape
    if (shape.size() != 2 || shape[0] != 10 || shape[1] != 20) {
        throw std::runtime_error("Unexpected shape");
    }

    return data;
}
```

### Saving .npy File with libnpy

```cpp
// Source: https://github.com/llohse/libnpy
#include "npy.hpp"

void save_npy_data(const std::string& path, const std::vector<float>& data,
                   const std::vector<unsigned long>& shape) {
    npy::SaveArrayAsNumpy(path, false, shape.size(), shape.data(), data);
}
```

### Alternative: Using cnpy

```cpp
// Source: https://github.com/rogersce/cnpy
#include "cnpy.h"

void load_and_save_with_cnpy() {
    // Load
    cnpy::NpyArray arr = cnpy::npy_load("input.npy");
    float* data = arr.data<float>();

    // Process...

    // Save
    std::vector<size_t> shape = {10, 20};
    cnpy::npy_save("output.npy", data, shape, "w");
}
```

### Converting ggml_tensor to/from NumPy Layout

```cpp
// Helper to handle potential layout differences
void copy_ggml_to_numpy_layout(const ggml_tensor* tensor, std::vector<float>& out) {
    size_t nelements = ggml_nelements(tensor);
    out.resize(nelements);

    // For simple contiguous tensors
    if (ggml_is_contiguous(tensor)) {
        memcpy(out.data(), tensor->data, nelements * sizeof(float));
    } else {
        // Element-by-element copy for non-contiguous
        for (size_t i = 0; i < nelements; i++) {
            out[i] = ggml_get_f32_1d(tensor, i);
        }
    }
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual binary parsing | libnpy/cnpy libraries | ~2018 | Simplified .npy I/O significantly |
| LibTorch for all .pt files | Convert .pt to .npy in Python | ~2020 | Reduced C++ test dependencies |
| Custom test data formats | NumPy .npy standard | ~2019 | Better Python/C++ interop |

**Deprecated/outdated:**
- **numpy_data library**: Older header-only library, less maintained than libnpy
- **Direct pickle parsing in C++**: Too complex, use .npy instead
- **Embedding test data in code**: Use external .npy files for maintainability

## Open Questions

1. **Should we support .npz (compressed) files?**
   - What we know: .npz requires zlib, adds complexity
   - What's unclear: Whether test data size justifies compression
   - Recommendation: Start with .npy only, add .npz if test data grows >100MB

2. **How to handle .pt files from existing PyTorch models?**
   - What we know: .pt is pickle format, hard to parse in C++
   - What's unclear: Whether we need direct .pt loading or can preprocess
   - Recommendation: Add Python script to convert .pt → .npy for test data

3. **Memory layout conversion strategy?**
   - What we know: NumPy is row-major, ggml has custom layout
   - What's unclear: Whether we need automatic transposition or manual handling
   - Recommendation: Document expected layout, provide helper functions, validate with tests

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| zlib | cnpy (.npz support) | ✓ | system | Use .npy only (no compression) |
| C++11 compiler | libnpy | ✓ | GCC 7+ | — |
| Python + NumPy | Test data generation | ✓ | Python 3.8+, NumPy 1.20+ | — |
| LibTorch | .pt file loading | ✗ | — | Convert .pt to .npy in Python |

**Missing dependencies with no fallback:**
- None - all critical dependencies available

**Missing dependencies with fallback:**
- LibTorch: Not installed, but not needed - use .npy format instead

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Custom (test_framework.hpp) |
| Config file | None - header-only framework |
| Quick run command | `./build/bin/test_wan_encoder` |
| Full suite command | `ctest --test-dir build` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| IO-01 | Load .npy to ggml_tensor | unit | `./build/bin/test_io_utils` | ❌ Wave 0 |
| IO-02 | Save ggml_tensor to .npy | unit | `./build/bin/test_io_utils` | ❌ Wave 0 |
| IO-03 | Compare tensors with tolerance | unit | `./build/bin/test_io_utils` | ❌ Wave 0 |
| IO-04 | Validate encoder output vs Python | integration | `./build/bin/test_wan_encoder` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `./build/bin/test_io_utils`
- **Per wave merge:** `ctest --test-dir build`
- **Phase gate:** All tests green + manual validation with Python reference

### Wave 0 Gaps
- [ ] `tests/cpp/test_io_utils.hpp` - I/O utility functions
- [ ] `tests/cpp/test_io_utils.cpp` - Unit tests for I/O utilities
- [ ] `tests/cpp/test_wan_encoder.cpp` - Integration test with reference data
- [ ] `tests/cpp/test_data/` - Directory for .npy test files
- [ ] Python script to generate reference test data

## Sources

### Primary (HIGH confidence)
- [libnpy GitHub](https://github.com/llohse/libnpy) - Header-only C++ .npy library
- [cnpy GitHub](https://github.com/rogersce/cnpy) - C/C++ .npy/.npz library
- [npio GitHub](https://github.com/onai/npio) - Lightweight alternative
- [PyTorch Serialization Docs](https://pytorch.org/docs/stable/notes/serialization.html) - .pt format details

### Secondary (MEDIUM confidence)
- [NumPy .npy format spec](https://numpy.org/devdocs/reference/generated/numpy.lib.format.html) - Official format documentation
- [ggml tensor layout](https://huggingface.co/blog/introduction-to-ggml) - Understanding ggml memory layout
- [PyTorch C++ discussions](https://discuss.pytorch.org/t/load-tensors-saved-in-python-from-c-and-vice-versa/39435) - Community patterns

### Tertiary (LOW confidence)
- Various Stack Overflow discussions on .npy parsing - marked for validation

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - libnpy and cnpy are well-established, widely used
- Architecture: HIGH - Patterns verified from existing ggml codebase
- Pitfalls: HIGH - Based on common issues in ML C++/Python interop

**Research date:** 2026-03-27
**Valid until:** 2026-06-27 (90 days - stable domain)
