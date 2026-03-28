# Testing Patterns

**Analysis Date:** 2026-03-28

## Test Framework

**Runner:**
- Custom lightweight test framework in `tests/cpp/test_framework.hpp`
- No external test framework (gtest, Catch2, etc.)
- TestSuite class wraps test cases with try/catch for failure collection
- Assertion macros throw `std::runtime_error` on failure for test failure reporting

**Assertion Library:**
- Custom macros in `test_framework.hpp`:
  - `WAN_ASSERT_EQ(a, b)` - Equality assertion
  - `WAN_ASSERT_TRUE(cond)` - Boolean assertion
  - `WAN_ASSERT_NEAR(a, b, eps)` - Floating-point comparison with epsilon
  - `WAN_ASSERT_THROWS(expr)` - Exception assertion

**Run Commands:**
```bash
cd build && cmake .. && make test_clip
cd build && cmake .. && make test_t5
cd build && cmake .. && make test_vae
cd build && cmake .. && make test_transformer
```

## Test File Organization

**Location:**
- Test files in `tests/cpp/` directory
- Test files: `test_clip.cpp`, `test_t5.cpp`, `test_vae.cpp`, `test_transformer.cpp`
- Test utilities: `test_framework.hpp`, `test_helpers.hpp`, `test_io_utils.hpp`, `model_factory.hpp`

**Naming:**
- Pattern: `test_*.cpp` for test executables
- Pattern: `test_*.hpp` for test utilities and helpers
- Each model has dedicated test file: `test_<model>.cpp`

**Structure:**
```
tests/cpp/
├── test_framework.hpp       # TestSuite, assertion macros
├── test_helpers.hpp         # BackendRAII, T5Version enum
├── test_io_utils.hpp        # save_npy(), load_npy() for tensor I/O
├── model_factory.hpp        # Model factory declarations
├── test_clip.cpp            # CLIP model tests
├── test_t5.cpp              # T5 model tests
├── test_vae.cpp             # WAN VAE model tests
├── test_transformer.cpp     # WAN Transformer (DiT) tests
├── test_factory.cpp         # Model factory tests
└── test_io_npy.cpp          # NumPy I/O tests
```

## Test Structure

**Suite Organization:**
```cpp
int main() {
    TestSuite suite{"Model Name Tests"};
    test_model_init(suite);
    test_model_registry(suite);
    test_model_npy_io(suite);
    test_model_inference_with_random_data(suite);
    return suite.report();
}
```

**Patterns:**
- Each test file has 4 main test groups:
  1. Initialization tests (registry creation, model instantiation)
  2. Registry registration tests (verify versions are registered)
  3. NumPy I/O roundtrip tests (numerical consistency)
  4. Inference tests with random data (forward pass validation)

**Setup pattern:**
```cpp
void test_model_init(TestSuite& suite) {
    wan_force_model_registrations();  // Force linker to include model_factory.cpp
    String2TensorStorage empty_map{};

    suite.run("test_name", [&empty_map]() {
        BackendRAII guard(ggml_backend_cpu_init());  // RAII backend management
        auto runner = ModelRegistry::instance()->create<ModelType>(
            "version-string", guard.backend, false, empty_map, "");
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();
    });
}
```

## Mocking

**Framework:** No mocking library; uses random data generation for testing

**Patterns:**
- Random tensor generation: Fill tensors with pseudo-random values
- Dummy tensor storage: Empty `String2TensorStorage` maps for unit tests
- Dummy block tensors: Create minimal tensors to trigger model detection (e.g., `tensor_map["blocks.39.weight"]`)

**What to Mock:**
- Backend: Use `ggml_backend_cpu_init()` for CPU-only testing
- Tensor storage: Use empty maps for unit tests, real weights for integration tests
- Model parameters: Generate random weights for inference validation

**What NOT to Mock:**
- ggml context and tensor operations (use real ggml)
- Model forward passes (test actual computation)
- Registry mechanism (test real factory creation)

## Fixtures and Factories

**Test Data:**
- Random tensor generation in test code:
```cpp
ggml_tensor* x = ggml_new_tensor_4d(ctx, GGML_TYPE_F32, 8, 8, 2, 16);
float* x_data = (float*)x->data;
for (int i = 0; i < 16*2*8*8; ++i) x_data[i] = (float)(i % 100) * 0.01f - 0.5f;
```

- NumPy file I/O for comparison:
```cpp
fs::path tmp = fs::temp_directory_path() / "test_output.npy";
save_npy(tmp.string(), output);
// Compare with Python reference outputs
fs::remove(tmp);
```

**Location:**
- Test data generated inline in test functions
- Temporary files in system temp directory: `fs::temp_directory_path()`
- No persistent test fixtures directory

**Factory Pattern:**
- `ModelRegistry::instance()->create<ModelType>(version, backend, offload, tensor_map, prefix)`
- Factory functions registered in `src/model_factory.cpp` using `REGISTER_MODEL_FACTORY` macro
- Each model version has dedicated factory lambda

## Coverage

**Requirements:** Not enforced - No coverage configuration detected

**View Coverage:** Not applicable - No coverage tools configured

## Test Types

**Unit Tests:**
- Model initialization: Verify model creation via registry
- Registry mechanics: Verify version registration and lookup
- NumPy I/O: Verify tensor save/load roundtrip with numerical consistency
- Scope: Single model, no dependencies on other models

**Integration Tests:**
- Model inference: Forward pass with random inputs
- Scope: Full model pipeline from input to output
- Validates: Tensor shapes, computation success, output generation

**E2E Tests:**
- Not implemented - Tests focus on individual models
- Could be extended with multi-model pipelines (CLIP + T5 + VAE + Transformer)

## Common Patterns

**Async Testing:**
- Not applicable - All tests are synchronous CPU-based

**Error Testing:**
- Registry lookup failure: `ModelRegistry::instance()->create()` throws on unknown version
- Tensor shape validation: Assertions on output tensor dimensions
- Example:
```cpp
WAN_ASSERT_EQ(output->ne[0], 8);   // width
WAN_ASSERT_EQ(output->ne[1], 8);   // height
WAN_ASSERT_EQ(output->ne[2], 2);   // time
WAN_ASSERT_EQ(output->ne[3], 16);  // channels
```

**Memory Management:**
- RAII pattern for backend: `BackendRAII guard(ggml_backend_cpu_init())`
- Context cleanup: `ggml_free(ctx)` at test end
- Temporary file cleanup: `fs::remove(tmp)` after test

## Model-Specific Test Patterns

**CLIP Text Encoder (`test_clip.cpp`):**
- 3 versions: `clip-vit-l-14`, `clip-vit-h-14`, `clip-vit-bigg-14`
- Input: Token IDs `[1, 77]` (batch=1, seq_len=77)
- Input: Attention mask `[1, 77]` (float32)
- Output: Embeddings `[768, 77]` (hidden_size=768, seq_len=77)
- Context size: 100MB
- Forward signature: `runner->forward(&runner_ctx, input_ids, nullptr, mask, 0, false, -1)`

**T5 Text Encoder (`test_t5.cpp`):**
- 2 versions: `t5-standard`, `t5-umt5`
- Input: Token IDs `[1, 16]` (batch=1, seq_len=16)
- Input: Position bucket `[16, 16]` (int32)
- Output: Hidden states `[4096, 16]` (model_dim=4096, seq_len=16)
- Context size: 256MB
- Forward signature: `runner->forward(&runner_ctx, input_ids, pos_bucket, nullptr)`

**WAN VAE (`test_vae.cpp`):**
- 4 versions: `wan-vae-t2v`, `wan-vae-t2v-decode`, `wan-vae-i2v`, `wan-vae-ti2v`
- Decode-only testing: Use `wan-vae-t2v-decode` for inference tests
- Input: Latent `[16, 1, 8, 8]` (c=16, t=1, h=8, w=8) in ggml layout `[w, h, t, c]`
- Output: Decoded pixels (shape depends on model)
- Context size: 100MB
- Compute signature: `runner->compute(4, z, true, &decoded, ctx)` (4=n_threads, true=decode_only)

**WAN Transformer/DiT (`test_transformer.cpp`):**
- 3 versions: `wan-runner-t2v`, `wan-runner-i2v`, `wan-runner-ti2v`
- Input: Latent `[16, 2, 8, 8]` (c=16, t=2, h=8, w=8) in ggml layout `[w, h, t, c]`
- Input: Timesteps `[1]` (int32, value=500)
- Input: Context `[1, 77, 4096]` (text embeddings, batch=1, seq_len=77, dim=4096)
- Output: Latent `[16, 2, 8, 8]` (same shape as input)
- Context size: 100MB
- Compute signature: `runner->compute(4, x, timesteps, context, nullptr, nullptr, nullptr, nullptr, 1.0f, &output, ctx)`

## GGMLRunnerContext Pattern

**Structure:**
```cpp
struct GGMLRunnerContext {
    ggml_backend_t backend                        = nullptr;
    ggml_context* ggml_ctx                        = nullptr;
    bool flash_attn_enabled                       = false;
    bool conv2d_direct_enabled                    = false;
    bool circular_x_enabled                       = false;
    bool circular_y_enabled                       = false;
};
```

**Usage in tests:**
```cpp
GGMLRunnerContext runner_ctx;
runner_ctx.ggml_ctx = ctx;
runner_ctx.backend = guard.backend;
// Pass to model forward/compute methods
struct ggml_tensor* output = runner->forward(&runner_ctx, input_ids, nullptr, mask, 0, false, -1);
```

## NumPy I/O Utilities

**Location:** `tests/cpp/test_io_utils.hpp`

**Functions:**
- `load_npy(ctx, path)` - Load .npy file into ggml tensor
- `save_npy(path, tensor)` - Save ggml tensor to .npy file

**Supported types:** FLOAT32, FLOAT16, INT32, INT64

**Dimension mapping:**
- NumPy shape (d0, d1, ..., dn) → ggml ne[0]=dn, ne[1]=d(n-1), ...
- ggml ne[0] is innermost (fastest-varying) dimension
- For 2D: numpy shape (rows, cols) → ggml ne = {cols, rows}

**Usage:**
```cpp
// Save tensor to .npy
fs::path tmp = fs::temp_directory_path() / "test_output.npy";
save_npy(tmp.string(), output);

// Load tensor from .npy
ggml_tensor* loaded = load_npy(ctx, tmp.string());
fs::remove(tmp);
```

## Model Detection Logic

**Layer count detection:**
- Scan tensor_map for `blocks.N.weight` to find max layer index
- WAN 14B models: 40 layers (blocks.0 to blocks.39)
- WAN 5B models: 30 layers (blocks.0 to blocks.29)
- WAN 1.3B models: 30 layers (blocks.0 to blocks.29)

**Model type detection:**
- T2V: Default, no special markers
- I2V: Presence of `img_emb` tensor in tensor_map
- TI2V: Presence of `img_emb` tensor + 30 layers (5B model)

**Version detection:**
- `VERSION_WAN2` (14B T2V): 40 layers, no img_emb
- `VERSION_WAN2_2_I2V` (14B I2V): 40 layers, has img_emb
- `VERSION_WAN2_2_TI2V` (5B TI2V): 30 layers, has img_emb

## Test Execution Flow

**Typical test execution:**
1. Call `wan_force_model_registrations()` to prevent linker DCE
2. Create `BackendRAII` guard for CPU backend
3. Create empty or dummy `String2TensorStorage` map
4. Call `ModelRegistry::instance()->create<ModelType>(version, backend, offload, map, prefix)`
5. Call `runner->alloc_params_buffer()` to allocate parameter buffers
6. Create ggml context with appropriate size (100MB-256MB)
7. Create input tensors with random data
8. Create `GGMLRunnerContext` with backend and ggml_ctx
9. Call `runner->forward()` or `runner->compute()` with inputs
10. Validate output tensor shapes and values
11. Optionally save output to .npy for comparison
12. Clean up: `ggml_free(ctx)`, `fs::remove(tmp)`

---

*Testing analysis: 2026-03-28*
