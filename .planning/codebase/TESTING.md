# Testing Patterns

**Analysis Date:** 2026-03-17

## Test Framework

**Runner:**
- Not detected - No test framework configured
- CMake build system present but no test targets defined
- No test executables found in examples or build directories

**Assertion Library:**
- Not applicable - No testing framework detected

**Run Commands:**
- Not applicable - No test infrastructure present

## Test File Organization

**Location:**
- Not detected - No test files found in codebase
- Searched patterns: `*test*`, `*spec*`, `*_test.cpp`, `*_test.hpp`
- Result: No test files in `/home/jtzhuang/projects/stable-diffusion.cpp/wan/src` or subdirectories

**Naming:**
- Not applicable - No test files present

**Structure:**
- Not applicable - No test files present

## Test Structure

**Suite Organization:**
- Not applicable - No test framework detected

**Patterns:**
- Not applicable - No test files present

## Mocking

**Framework:**
- Not detected - No mocking library configured

**Patterns:**
- Not applicable - No test files present

**What to Mock:**
- Not applicable - No test files present

**What NOT to Mock:**
- Not applicable - No test files present

## Fixtures and Factories

**Test Data:**
- Not applicable - No test files present

**Location:**
- Not applicable - No test files present

## Coverage

**Requirements:**
- Not enforced - No coverage configuration detected

**View Coverage:**
- Not applicable - No test infrastructure present

## Test Types

**Unit Tests:**
- Not implemented - No unit tests found

**Integration Tests:**
- Not implemented - No integration tests found

**E2E Tests:**
- Not implemented - No end-to-end tests found

## Common Patterns

**Async Testing:**
- Not applicable - No test files present

**Error Testing:**
- Not applicable - No test files present

## Testing Approach in Codebase

**Current State:**
- This is a C++ library for video generation (WAN-2 model inference)
- Testing appears to be manual via example applications in `examples/` directory
- Examples include CLI tools and conversion utilities

**Example Applications (Manual Testing):**
- `examples/cli/` - Command-line interface for inference
- `examples/convert/` - Model conversion utilities
- These serve as integration tests for the library

**Validation Approach:**
- Error handling via return codes (`bool` success/failure)
- Context error messages: `ctx->last_error` for detailed error information
- Assertions for internal invariants: `GGML_ASSERT()` macro in `ggml_extend.hpp`

**Example Error Handling Pattern:**
```cpp
// From wan-api.cpp
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

**Assertion Pattern:**
```cpp
// From ggml_extend.hpp
GGML_ASSERT(dims == 2 || dims == 3);
static_assert(GGML_MAX_NAME >= 128, "GGML_MAX_NAME must be at least 128");
```

## Recommendations for Adding Tests

**If tests are to be added:**

1. **Framework Choice:**
   - Google Test (gtest) - Popular for C++ projects
   - Catch2 - Header-only, modern C++
   - doctest - Lightweight, fast compilation

2. **Test Organization:**
   - Create `tests/` directory at project root
   - Mirror source structure: `tests/unit/`, `tests/integration/`
   - Name test files: `test_*.cpp` or `*_test.cpp`

3. **Test Targets in CMake:**
   - Add `enable_testing()` in `CMakeLists.txt`
   - Create test executable targets
   - Register with `add_test()`

4. **What to Test:**
   - Model loading: `ModelLoader::init_from_file()` with various formats
   - Tensor operations: `ggml_extend.hpp` functions
   - Block forward passes: `GGMLBlock::forward()` implementations
   - Error handling: invalid inputs, missing files, corrupted data
   - API boundary: C API in `wan-api.cpp`

5. **Test Data:**
   - Create `tests/fixtures/` for small test models
   - Use mock tensors for unit tests
   - Use real models for integration tests

---

*Testing analysis: 2026-03-17*
