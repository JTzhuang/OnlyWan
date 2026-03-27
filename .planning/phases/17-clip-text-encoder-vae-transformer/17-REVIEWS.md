---
phase: 17
reviewers: [claude]
reviewed_at: 2026-03-27T00:00:00Z
plans_reviewed: [17-01-PLAN.md, 17-02-PLAN.md]
---

# Cross-AI Plan Review — Phase 17

## Claude Review (独立会话)

# Cross-AI Plan Review: Phase 17 Unit Test Framework

## Summary

Both plans are well-structured and address the core phase goals (test infrastructure, template factory, initialization tests). The factory pattern design is sound and the test organization is logical. However, there are **critical ODR (One Definition Rule) risks** that could reintroduce the bugs the project already fixed, plus several implementation details that need clarification before execution. The plans assume model constructors work with empty weight storage and synthetic data, which needs verification against actual constructor signatures.

---

## PLAN 17-01: Test Infrastructure + Template Factory

### Strengths

- **Lightweight framework design**: Assert-based macros with no external dependencies aligns with project philosophy
- **Generic factory pattern**: `ModelFactory<ModelType, VersionEnum>` is appropriately generic and extensible
- **RAII wrapper**: BackendRAII for ggml_backend_t is a good safety pattern
- **T5Version bridge**: Creating an enum to abstract the `bool is_umt5` parameter is a clean solution
- **Comprehensive factory tests**: Tests all 4 model types with realistic scenarios (registration, creation, error handling)
- **Clear verification criteria**: must_haves.truths are specific and testable

### Concerns

| Issue | Severity | Details |
|-------|----------|---------|
| **ODR Violation Risk** | **HIGH** | Plan states tests will "include model headers directly from src/". Project already fixed ODR issues by making wan-api.cpp the *only* TU including model headers. If tests also include them, you'll have multiple definitions of CLIPTextModelRunner, T5Runner, etc. across test_clip.cpp, test_t5.cpp, test_vae.cpp, test_transformer.cpp, AND wan-api.cpp. This will cause linker errors or undefined behavior. |
| **T5Version Bridge Unclear** | **MEDIUM** | Plan creates T5Version enum but doesn't specify how factory converts `T5Version::STANDARD_T5` → `bool is_umt5=false` when calling T5Runner constructor. The factory's `create()` method needs explicit logic for this conversion. |
| **Empty String2TensorStorage Assumption** | **MEDIUM** | Plan assumes `String2TensorStorage{}` (empty) works for all model constructors. But what if a constructor tries to access weights during initialization? Need to verify all 4 model constructors tolerate empty storage. |
| **BackendRAII Implementation Missing** | **MEDIUM** | Plan mentions BackendRAII but doesn't specify: Does it call `ggml_backend_free()`? Is it safe to free CPU backend? What about reference counting? |
| **CMake wan_add_test() Underspecified** | **MEDIUM** | Function signature and linking strategy not detailed. Does it link against wan-cpp library? Does it handle include paths for src/ headers? |
| **alloc_params_buffer() Precondition** | **LOW** | Plan calls `alloc_params_buffer()` before `get_desc()`, but doesn't explain why this order matters or what happens if skipped. |

### Suggestions

1. **Solve ODR by forward-declaring in tests**: Instead of including model headers directly, create thin wrapper headers in tests/cpp/ that forward-declare the model classes and provide factory registration. This keeps model definitions in wan-api.cpp only.
   - Example: `tests/cpp/model_wrappers.hpp` declares `class CLIPTextModelRunner;` and provides factory registration without including src/models/clip.hpp

2. **Explicit T5Version → bool conversion**: Add to model_factory.hpp:
   ```cpp
   template<> struct VersionTraits<T5Version> {
     static bool to_is_umt5(T5Version v) { return v == T5Version::UMT5; }
   };
   ```
   Then factory's `create()` uses this trait.

3. **Verify empty storage assumption**: Add a pre-execution check task:
   - Create minimal test that constructs each model with empty storage
   - Confirm no crashes or exceptions during construction
   - Document which models require non-empty storage (if any)

4. **BackendRAII specification**: Clarify in test_helpers.hpp:
   - Does it own the backend handle?
   - Does it call ggml_backend_free() in destructor?
   - Is CPU backend safe to free?
   - Add comments explaining lifetime management

5. **CMake function detail**: Specify wan_add_test() implementation:
   ```cmake
   function(wan_add_test name source)
     add_executable(${name} ${source})
     target_link_libraries(${name} PRIVATE wan-cpp)
     target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
     add_test(NAME ${name} COMMAND ${name})
   endfunction()
   ```

---

## PLAN 17-02: Four Model Version Initialization Unit Tests

### Strengths

- **Comprehensive version coverage**: Tests all documented versions (3 CLIP, 2 T5, 4 VAE, 5 Flux)
- **Correct namespace handling**: Properly qualifies `Flux::FluxRunner`
- **Logical file organization**: Separate test files per model type aids maintainability
- **RAII safety**: Uses BackendRAII consistently across all tests
- **Clear test names**: test_clip_factory, test_t5_factory, etc. are self-documenting

### Concerns

| Issue | Severity | Details |
|-------|----------|---------|
| **Version Enum Correctness** | **MEDIUM** | Plan uses `SDVersion::VERSION_SD1, SD2, FLUX, FLUX2` for VAE and `VERSION_FLUX, FLUX_FILL, FLEX_2, CHROMA_RADIANCE, OVIS_IMAGE` for Flux. Are these actual enum values in the codebase? Need to verify against model.h. |
| **Model Constructor Signatures** | **MEDIUM** | Plan assumes all models can be constructed with `(ggml_backend_t, bool, String2TensorStorage&, std::string)`. But do they all accept these exact parameters? Research notes say "common pattern" but doesn't guarantee all 4 match. |
| **get_desc() Preconditions** | **MEDIUM** | Plan calls `get_desc()` after `alloc_params_buffer()`. But what if get_desc() requires actual model metadata that's missing from empty storage? Could return garbage or crash. |
| **Flux version name typo?** | **LOW** | Plan lists `FLEX_2` — is this a typo for `FLUX_2`? Need to verify against actual enum. |
| **No negative tests** | **LOW** | Plan only tests successful initialization. No tests for invalid versions, null backends, or corrupted storage. Scope is initialization only, but worth noting. |

### Suggestions

1. **Verify version enums before execution**: Grep codebase for `enum.*SDVersion` and `enum.*CLIPVersion` definitions and confirm all version values listed in plan actually exist.

2. **Verify constructor signatures**: Before writing test code, read CLIPTextModelRunner, T5Runner, AutoEncoderKL, Flux::FluxRunner constructors and confirm all accept `(ggml_backend_t, bool, String2TensorStorage&, std::string)`.

3. **Test get_desc() safety**: Add a verification step — call `get_desc()` on a freshly constructed model with empty storage and confirm it returns expected string.

4. **Add fixture for common setup**: Create a test helper:
   ```cpp
   struct ModelTestFixture {
     BackendRAII backend{ggml_backend_cpu_init()};
     String2TensorStorage empty_storage{};
   };
   ```

5. **Document version mapping**: In test files, add comments mapping enum values to model versions.

---

## Risk Assessment

### Overall Risk Level: **MEDIUM-HIGH**

**Justification:**

1. **ODR Risk (HIGH Impact, HIGH Probability)**: The project already had ODR bugs. Including model headers in multiple test files will almost certainly reintroduce them. This is the **blocking risk** — if not addressed, the build will fail or produce undefined behavior.

2. **Implementation Assumption Gaps (MEDIUM Impact, MEDIUM Probability)**: Several assumptions about constructor signatures, enum values, and empty storage behavior are not verified. If any assumption is wrong, tests will fail to compile or crash at runtime.

3. **CMake Integration (MEDIUM Impact, LOW Probability)**: The build system integration is underspecified but straightforward. Low risk if done carefully.

**Recommendation:**

✅ **Approve PLAN 17-01** if:
- ODR solution is implemented (forward declarations + factory registration, not direct includes)
- BackendRAII and T5Version bridge implementations are detailed
- CMake wan_add_test() function is specified

✅ **Approve PLAN 17-02** if:
- Version enum values are verified against codebase
- Constructor signatures are confirmed for all 4 models
- get_desc() behavior with empty storage is tested in a pre-execution check

---

## Consensus Summary

*（单一评审员 — 无多方对比）*

### Key Concerns (High Priority)

1. **ODR 违反风险** — 测试文件直接 include model headers 将重新引入项目已修复的 ODR 问题。测试文件应通过工厂注册 (creator lambdas) 访问模型，而不是 include 头文件。这是**执行前必须解决**的阻塞性问题。

2. **构造函数空 TensorStorage 假设** — 需要验证四个模型的构造函数对 `String2TensorStorage{}` 的容错性。

3. **版本枚举值正确性** — FLEX_2 是否是 FLUX_2 的笔误？需在执行前核实 model.h 中的实际枚举值。

### Agreed Strengths

- 模板工厂设计思路正确，可扩展性好
- BackendRAII RAII 模式符合项目 C++17 规范
- T5Version 枚举桥接方案是解决 `bool is_umt5` 的优雅方法
- RAII 资源管理确保后端正确释放

### Divergent Views

N/A — 单一评审员

### Action Items for --reviews Replan

1. **解决 ODR**: 在计划中明确说明测试通过 `wan-cpp` 库访问模型，而非直接 include 源文件。executor 应该能通过链接 wan-cpp 获得 Runner 类符号。
2. **澄清 BackendRAII**: 明确其析构函数调用 `ggml_backend_free()`。
3. **验证枚举值**: 将 FLEX_2 验证步骤加入 Task 1 的 acceptance criteria。
4. **CMake 函数规范**: 将完整的 wan_add_test() 函数实现写入计划。
