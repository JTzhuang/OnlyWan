# Phase 17: 模型单元测试与工厂模式 - Research

**Researched:** 2026-03-27
**Domain:** C++17 单元测试框架设计 + 泛型工厂模式 + GGML Runner 测试架构
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01:** 使用自定义轻量级测试框架（基于assert），最小化依赖
- **D-02:** 框架应支持基本的测试用例组织、断言、fixture管理
- **D-03:** 采用通用模板工厂（Template Factory Pattern）
- **D-04:** 支持任意模型类型和版本的动态注册和创建
- **D-05:** 工厂应提供统一的注册接口：`register<ModelType, VersionEnum>(creator_func)`
- **D-06:** 工厂应提供统一的创建接口：`create<ModelType>(version, backend, params)`
- **D-07:** 初始化测试 — 模型初始化、权重加载、后端配置
- **D-08:** 推理测试 — 单个输入的推理、输出形状验证、数值范围检查
- **D-09:** 版本兼容性 — 多个版本在相同输入下的一致性验证
- **D-10:** 鲁棒性测试 — 内存泄漏、资源释放、边界条件
- **D-11:** 数值一致性对比 — 与标准输出进行对比验证
- **D-12:** 混合方案：快速单元测试用合成数据，集成测试用真实数据
- **D-13:** 单元测试使用小型随机张量（快速验证逻辑）
- **D-14:** 集成测试使用真实模型权重和标准输入/输出数据集

### Claude's Discretion

- 测试用例的具体数量和粒度
- 性能基准测试的阈值设定
- 测试数据集的具体来源和格式

### Deferred Ideas (OUT OF SCOPE)

- 性能基准测试套件（可作为Phase 18）
- 模型量化测试（FP8、INT8等）
- 分布式推理测试（多GPU场景）
</user_constraints>

---

## Summary

Phase 17 的目标是为四个核心模型（CLIP、T5/UMT5、VAE、Transformer/Flux）建立完整的 C++ 单元测试框架，并使用模板工厂模式管理多版本模型的注册与创建。

项目当前无 C++ 测试二进制（`tests/` 目录下全为 Python 测试）。C++ 测试需要新建 `tests/cpp/` 子目录，并在 CMakeLists.txt 中增加测试构建支持。所有四个模型均通过同一个 `GGMLRunner` 基类提供统一的初始化接口（`alloc_params_buffer` + `get_param_tensors`），这是工厂抽象的关键切入点。

由于决策 D-01 规定使用基于 `assert` 的自定义轻量框架（不引入 GTest/Catch2），测试框架本身必须在 Phase 内自行实现。框架应提供：测试套件注册、断言宏、测试报告输出（通过/失败/总数）。合成张量（随机浮点数填充）可以无需真实权重文件完成形状和逻辑验证，这是快速单元测试的核心策略。

**Primary recommendation:** 在 `tests/cpp/` 中实现共享的 `test_framework.hpp` 和 `model_factory.hpp`，然后为每个模型创建独立测试文件（`test_clip.cpp` 等），通过 CMake `enable_testing()` + `add_test()` 集成。

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| GGML | 已集成 (ggml/) | 张量分配、后端管理 | 所有 Runner 已依赖此库 |
| C++17 STL | N/A | 模板元编程、std::function、std::map | 项目已设定 C++17 标准 |
| ggml-backend | 已集成 | CPU/CUDA 后端初始化 | GGMLRunner 构造函数需要 ggml_backend_t |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `<cassert>` | C++标准库 | 基础断言（assert 宏） | 快速失败断言，D-01 要求 |
| `<random>` | C++标准库 | 合成测试数据生成 | 单元测试中生成随机张量（D-13）|
| `<iostream>` | C++标准库 | 测试报告输出 | 通过/失败统计输出 |
| CTest | CMake 内置 | 测试注册和运行 | `enable_testing()` + `add_test()` |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| 自定义 assert 框架 | Google Test | D-01 已锁定：最小化依赖，不引入 GTest |
| 自定义 assert 框架 | Catch2 | 同上，Catch2 也需额外依赖 |
| CTest | 独立测试可执行文件 | CTest 让 `cmake --build && ctest` 一体化，是 C++ 项目标准做法 |

**Installation:** 无需额外安装。全部使用项目已有的 GGML、CMake 和 C++ 标准库。

---

## Architecture Patterns

### Recommended Project Structure

```
tests/
├── cpp/                         # 新增 C++ 测试目录
│   ├── CMakeLists.txt           # 测试构建配置
│   ├── test_framework.hpp       # 自定义轻量测试框架（断言、套件、报告）
│   ├── model_factory.hpp        # 通用模板工厂实现
│   ├── test_helpers.hpp         # 合成张量生成、数值比较工具
│   ├── test_clip.cpp            # CLIP 模型测试（3个版本）
│   ├── test_t5.cpp              # T5/UMT5 模型测试（2个版本）
│   ├── test_vae.cpp             # VAE 模型测试（4+个版本）
│   ├── test_transformer.cpp     # Transformer/Flux 测试（5+个版本）
│   └── test_factory.cpp         # 工厂模式本身的测试
├── conftest.py                  # 已有（Python 测试配置）
├── fixtures/                    # 已有（Python 测试数据）
└── scripts/                     # 已有（Python 脚本测试）
```

### Pattern 1: 自定义轻量测试框架（test_framework.hpp）

**What:** 基于 `assert` 的单头文件测试框架，支持套件注册、断言宏、测试计数和报告。
**When to use:** 所有 C++ 测试文件中 `#include "test_framework.hpp"`。

```cpp
// Source: 自定义实现（基于 D-01 决策）
struct TestSuite {
    std::string name;
    int passed = 0;
    int failed = 0;

    void run(const std::string& test_name, std::function<void()> test_fn) {
        try {
            test_fn();
            std::cout << "  [PASS] " << test_name << "\n";
            passed++;
        } catch (const std::exception& e) {
            std::cout << "  [FAIL] " << test_name << ": " << e.what() << "\n";
            failed++;
        }
    }

    void report() const {
        std::cout << "\n=== " << name << " ===\n";
        std::cout << "Passed: " << passed << " / " << (passed + failed) << "\n";
        if (failed > 0) std::exit(1);
    }
};

// 断言宏——失败时抛异常而非直接 abort（便于收集多个失败）
#define WAN_ASSERT_EQ(a, b) \
    if (!((a) == (b))) throw std::runtime_error( \
        std::string("Assert failed: " #a " == " #b \
                    " at line " + std::to_string(__LINE__)))

#define WAN_ASSERT_TRUE(cond) \
    if (!(cond)) throw std::runtime_error( \
        std::string("Assert failed: " #cond \
                    " at line " + std::to_string(__LINE__)))

#define WAN_ASSERT_NEAR(a, b, eps) \
    if (std::abs((a) - (b)) > (eps)) throw std::runtime_error( \
        std::string("Assert near failed at line " + std::to_string(__LINE__)))
```

### Pattern 2: 通用模板工厂（model_factory.hpp）

**What:** C++17 模板工厂，用 `std::map` 存储版本到创建函数的映射，支持任意模型类型和版本枚举。
**When to use:** 需要统一注册和创建不同版本 Runner 实例时。

```cpp
// Source: C++17 模板工厂模式设计（D-03 ~ D-06）
template<typename ModelType, typename VersionEnum>
class ModelFactory {
public:
    using CreatorFn = std::function<std::unique_ptr<ModelType>(
        ggml_backend_t backend,
        bool offload_params_to_cpu,
        const String2TensorStorage& tensor_map,
        const std::string& prefix
    )>;

    // D-05: 统一注册接口
    void register_version(VersionEnum version, CreatorFn creator) {
        registry_[version] = std::move(creator);
    }

    // D-06: 统一创建接口
    std::unique_ptr<ModelType> create(
        VersionEnum version,
        ggml_backend_t backend,
        bool offload_params_to_cpu,
        const String2TensorStorage& tensor_map,
        const std::string& prefix = "") const {
        auto it = registry_.find(version);
        if (it == registry_.end()) {
            throw std::runtime_error("Unknown version");
        }
        return it->second(backend, offload_params_to_cpu, tensor_map, prefix);
    }

    bool has_version(VersionEnum version) const {
        return registry_.count(version) > 0;
    }

    std::vector<VersionEnum> registered_versions() const {
        std::vector<VersionEnum> versions;
        for (auto& [v, _] : registry_) versions.push_back(v);
        return versions;
    }

private:
    std::map<VersionEnum, CreatorFn> registry_;
};
```

### Pattern 3: 合成张量测试助手（test_helpers.hpp）

**What:** 不依赖真实权重文件创建合成测试张量（D-13），用于验证形状和逻辑。
**When to use:** 所有单元测试中创建输入张量时。

```cpp
// Source: 自定义实现（D-13: 小型随机张量）
inline ggml_tensor* make_synthetic_tensor(
    ggml_context* ctx,
    ggml_type dtype,
    std::vector<int64_t> dims,   // [ne0, ne1, ne2, ne3]
    float min_val = -1.0f,
    float max_val = 1.0f) {

    ggml_tensor* t = nullptr;
    switch (dims.size()) {
        case 1: t = ggml_new_tensor_1d(ctx, dtype, dims[0]); break;
        case 2: t = ggml_new_tensor_2d(ctx, dtype, dims[0], dims[1]); break;
        case 3: t = ggml_new_tensor_3d(ctx, dtype, dims[0], dims[1], dims[2]); break;
        default: /* 4d */ t = ggml_new_tensor_4d(ctx, dtype, dims[0], dims[1], dims[2], dims[3]);
    }
    // fill with random F32 values
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(min_val, max_val);
    size_t n = ggml_nelements(t);
    std::vector<float> data(n);
    for (auto& v : data) v = dist(rng);
    ggml_backend_tensor_set(t, data.data(), 0, ggml_nbytes(t));
    return t;
}
```

### Pattern 4: Runner 无权重初始化模式

**What:** 利用空的 `String2TensorStorage{}` 创建 Runner 实例，测试构造函数和内存分配而不需要真实模型文件。
**When to use:** D-07 初始化测试（模型初始化、后端配置），不需要验证权重内容时。

```cpp
// Source: 分析 wan-api.cpp 中的初始化模式
// 关键洞察：Runner 构造函数接受空 tensor_storage_map，init 会创建零尺寸参数
ggml_backend_t cpu_backend = ggml_backend_cpu_init();
String2TensorStorage empty_map{};

// 测试 CLIPTextModelRunner 构造
CLIPTextModelRunner runner(
    cpu_backend,
    /*offload_params_to_cpu=*/ false,
    empty_map,
    /*prefix=*/ "",
    OPENAI_CLIP_VIT_L_14,
    /*with_final_ln=*/ true
);
runner.alloc_params_buffer();
// 验证 Runner 正确初始化（get_desc() 返回 "clip"）
WAN_ASSERT_EQ(runner.get_desc(), std::string("clip"));

ggml_backend_free(cpu_backend);
```

### Pattern 5: 工厂注册模式

**What:** 在测试文件中为每个版本注册创建函数，然后通过工厂统一创建。
**When to use:** D-04 多版本动态注册和创建的验证。

```cpp
// Source: 基于 D-05/D-06 决策设计
using CLIPFactory = ModelFactory<CLIPTextModelRunner, CLIPVersion>;
CLIPFactory factory;

// 注册各版本
factory.register_version(OPENAI_CLIP_VIT_L_14,
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<CLIPTextModelRunner>(backend, offload, map, prefix, OPENAI_CLIP_VIT_L_14);
    });

factory.register_version(OPEN_CLIP_VIT_H_14,
    [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
        return std::make_unique<CLIPTextModelRunner>(backend, offload, map, prefix, OPEN_CLIP_VIT_H_14);
    });

// 创建实例
auto clip_l14 = factory.create(OPENAI_CLIP_VIT_L_14, cpu_backend, false, empty_map, "");
WAN_ASSERT_TRUE(clip_l14 != nullptr);
```

### Anti-Patterns to Avoid

- **全局 backend 不释放:** 每个测试套件的 `ggml_backend_t` 必须在套件结束时调用 `ggml_backend_free()`，否则会泄漏系统资源。
- **在单元测试中加载真实权重:** 单元测试（D-13）必须使用合成数据。只有标记为 `[integration]` 的测试才加载真实文件。
- **测试共享可变状态:** 每个测试用例应创建独立的 ggml_context 和 backend，避免跨测试状态污染。
- **使用 `assert()` 直接 abort:** 生产代码中的 `GGML_ASSERT` 是 abort 语义；测试框架的断言宏应改用抛异常，让测试框架统计失败数而不是崩溃。

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| 后端初始化 | 自定义 backend 创建逻辑 | `ggml_backend_cpu_init()` | GGMLRunner 构造函数已有成熟接口 |
| 张量内存管理 | 手动 malloc/free | `ggml_init()` + `ggml_backend_alloc_ctx_tensors()` | GGML 的 arena 分配器已处理对齐和生命周期 |
| 版本枚举映射 | 自定义版本字符串系统 | 直接使用 `CLIPVersion`、`SDVersion` 枚举 | 已有精确的枚举值，与现有代码完全兼容 |
| 测试进程管理 | 自定义 test runner | CMake `enable_testing()` + `ctest` | CTest 提供并行运行、输出过滤、失败重跑等能力 |
| 工厂类型擦除 | 手动 `void*` 转换 | `std::function` + `std::unique_ptr` | C++17 类型安全，无运行时转换开销 |

**Key insight:** GGMLRunner 基类已经封装了所有内存分配和后端管理逻辑。测试不需要直接操作 `ggml_context`——只需调用 Runner 的公开接口（`alloc_params_buffer`、`get_param_tensors`、`compute`）。

---

## Runner Classes — Canonical API Surface

下表是工厂模式需要操作的核心 Runner 类接口摘要（从源码分析，置信度：HIGH）：

### CLIP

| Runner 类 | 构造参数（关键）| compute 签名 | 版本枚举 |
|-----------|---------------|-------------|---------|
| `CLIPTextModelRunner` | `backend, offload, tensor_map, prefix, CLIPVersion, with_final_ln, force_clip_f32` | `compute(n_threads, input_ids, ...)` | `CLIPVersion`: OPENAI_CLIP_VIT_L_14, OPEN_CLIP_VIT_H_14, OPEN_CLIP_VIT_BIGG_14 |
| `CLIPVisionModelProjectionRunner` | `backend, offload, tensor_map, prefix, CLIPVersion, transpose_proj_w, proj_in` | `compute(n_threads, pixel_values, return_pooled, clip_skip, output, output_ctx)` | 同上 |

### T5

| Runner 类 | 构造参数（关键）| 版本标识 |
|-----------|---------------|---------|
| `T5Runner` | `backend, offload, tensor_map, prefix, is_umt5=false` | `is_umt5=false`=标准T5, `is_umt5=true`=UMT5多语言 |
| `T5Embedder` | 同 T5Runner（包装类，包含 tokenizer） | 同上 |

注意：T5 没有独立的版本枚举——用 `bool is_umt5` 区分两个版本。工厂需要用自定义枚举包装：

```cpp
enum class T5Version { STANDARD_T5, UMT5 };
```

### VAE

| Runner 类 | 构造参数（关键）| 版本枚举 |
|-----------|---------------|---------|
| `AutoEncoderKL` | `backend, offload, tensor_map, prefix, decode_only, use_video_decoder, SDVersion` | `SDVersion`: VERSION_SD1, VERSION_SD2, VERSION_FLUX, VERSION_FLUX2 等 |
| `FakeVAE` | `backend, offload` | 无版本（测试用 stub） |
| `VAE` (纯虚基类) | — | 工厂可以返回 `std::unique_ptr<VAE>` |

`SDVersion` 定义在 `src/model.h`，包含：VERSION_SD1、VERSION_SD2、VERSION_FLUX、VERSION_FLUX2 等多个值。

### Transformer (Flux)

| Runner 类 | 构造参数（关键）| 版本枚举 |
|-----------|---------------|---------|
| `Flux::FluxRunner` | `backend, offload, tensor_map, prefix, SDVersion, use_mask` | `SDVersion`: VERSION_FLUX, VERSION_FLUX_FILL, VERSION_FLEX_2, VERSION_CHROMA_RADIANCE, VERSION_OVIS_IMAGE 等 |

注意：`FluxRunner` 在 `Flux` 命名空间内（`namespace Flux { struct FluxRunner : public GGMLRunner {...}; }`）。

---

## CMake Integration Pattern

现有的 CMakeLists.txt 没有测试构建，需要在根 CMakeLists.txt 添加：

```cmake
# 在根 CMakeLists.txt 中（tests/ 子目录入口）
option(WAN_BUILD_TESTS "wan-cpp: build C++ unit tests" ${WAN_STANDALONE})

if(WAN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/cpp)
endif()
```

`tests/cpp/CMakeLists.txt` 模式：

```cmake
# tests/cpp/CMakeLists.txt
function(wan_add_test target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE wan-cpp)
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}  # 访问 test_framework.hpp
    )
    target_compile_features(${target_name} PRIVATE cxx_std_17)
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

wan_add_test(test_factory     test_factory.cpp)
wan_add_test(test_clip        test_clip.cpp)
wan_add_test(test_t5          test_t5.cpp)
wan_add_test(test_vae         test_vae.cpp)
wan_add_test(test_transformer test_transformer.cpp)
```

运行测试：
```bash
cmake -B build -DWAN_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Common Pitfalls

### Pitfall 1: 空 TensorStorage 导致 init 分配零参数但 alloc 仍需调用

**What goes wrong:** 用空 `String2TensorStorage{}` 构造 Runner 后，`params_ctx` 中没有任何张量，但 `alloc_params_buffer()` 仍然需要调用——否则 `compute()` 调用时 `compute_allocr` 为空会触发 `GGML_ASSERT`。
**Why it happens:** GGMLRunner 的 `compute()` 内部会调用 `alloc_compute_buffer()`，而后者需要 `params_buffer` 已分配。
**How to avoid:** 始终在构造后调用 `alloc_params_buffer()`，即使 params_ctx 中没有张量。
**Warning signs:** 在 `alloc_compute_buffer` 或 `ggml_gallocr_alloc_graph` 处触发 assert。

### Pitfall 2: FluxRunner 在 Flux 命名空间内

**What goes wrong:** `FluxRunner` 不能直接用 `FluxRunner`，必须用 `Flux::FluxRunner`。
**Why it happens:** `flux.hpp` 中所有 Flux 相关类都放在 `namespace Flux {}` 中。
**How to avoid:** 在 `test_transformer.cpp` 中使用 `using namespace Flux;` 或完整路径 `Flux::FluxRunner`。
**Warning signs:** 编译错误 "FluxRunner was not declared in this scope"。

### Pitfall 3: T5 没有版本枚举——需要测试端定义

**What goes wrong:** 工厂模板需要 VersionEnum 类型参数，但 T5Runner 只用 `bool is_umt5` 区分版本，没有对应的枚举类型。
**Why it happens:** T5Runner 是 bool 参数设计，不是枚举版本体系。
**How to avoid:** 在 `model_factory.hpp` 或 `test_t5.cpp` 中定义 `enum class T5Version { STANDARD_T5, UMT5 };` 作为工厂的 VersionEnum 参数，creator function 中将枚举转换为 bool。
**Warning signs:** 模板实例化时类型不匹配。

### Pitfall 4: 跨测试 backend 泄漏

**What goes wrong:** 若 `ggml_backend_t` 在测试失败时未释放，后续测试可能受影响（CUDA 情况下），且会引发内存检测工具报告泄漏。
**Why it happens:** 测试框架用异常处理失败，异常路径绕过了 `ggml_backend_free()` 调用。
**How to avoid:** 使用 RAII 包装器或在测试套件的 teardown 中调用 `ggml_backend_free()`。
**Warning signs:** CUDA 后端报 "context is already in use" 或 valgrind 报内存泄漏。

### Pitfall 5: AutoEncoderKL 构造需要 tensor_storage_map 中存在特定张量才能推断 use_linear_projection

**What goes wrong:** `AutoEncoderKL` 构造函数扫描 `tensor_storage_map` 查找 `attn_1.proj_out.weight` 来判断 `use_linear_projection`。空 map 默认为 false，这对 VERSION_SD1 是正确的，但对于某些版本可能产生不同的网络结构。
**Why it happens:** 网络结构推断依赖 tensor 名称存在性。
**How to avoid:** 在单元测试中接受默认值（`use_linear_projection=false`），仅在集成测试中使用真实权重。

### Pitfall 6: CLIPTextModelRunner 的 proj_in 检测

**What goes wrong:** `CLIPTextModelRunner` 构造函数扫描 tensor_storage_map 寻找 `self_attn.in_proj` 键来决定是否使用 `proj_in=true`。空 map 会导致 `proj_in=false`（OPENAI_CLIP_VIT_L_14 的正确默认值）。
**Why it happens:** 网络结构推断依赖 tensor 命名约定。
**How to avoid:** 同 Pitfall 5，单元测试接受默认值，集成测试使用真实权重文件。

---

## Code Examples

### 完整的测试文件结构示例 (test_clip.cpp)

```cpp
// Source: 基于 src/clip.hpp、wan-api.cpp 分析
#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_factory.hpp"

#include "clip.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

// 初始化测试：测试3个版本的 CLIPTextModelRunner 构造
void test_clip_initialization(TestSuite& suite) {
    suite.run("OPENAI_CLIP_VIT_L_14 init", []() {
        ggml_backend_t backend = ggml_backend_cpu_init();
        String2TensorStorage empty_map{};
        CLIPTextModelRunner runner(backend, false, empty_map, "", OPENAI_CLIP_VIT_L_14);
        runner.alloc_params_buffer();
        WAN_ASSERT_EQ(runner.get_desc(), std::string("clip"));
        ggml_backend_free(backend);
    });

    suite.run("OPEN_CLIP_VIT_H_14 init", []() {
        ggml_backend_t backend = ggml_backend_cpu_init();
        String2TensorStorage empty_map{};
        CLIPTextModelRunner runner(backend, false, empty_map, "", OPEN_CLIP_VIT_H_14);
        runner.alloc_params_buffer();
        WAN_ASSERT_EQ(runner.get_desc(), std::string("clip"));
        ggml_backend_free(backend);
    });

    suite.run("OPEN_CLIP_VIT_BIGG_14 init", []() {
        ggml_backend_t backend = ggml_backend_cpu_init();
        String2TensorStorage empty_map{};
        CLIPTextModelRunner runner(backend, false, empty_map, "", OPEN_CLIP_VIT_BIGG_14);
        runner.alloc_params_buffer();
        WAN_ASSERT_EQ(runner.get_desc(), std::string("clip"));
        ggml_backend_free(backend);
    });
}

// 工厂测试：3个版本都能通过工厂注册和创建
void test_clip_factory(TestSuite& suite) {
    suite.run("factory_registers_all_3_versions", []() {
        using CLIPTextFactory = ModelFactory<CLIPTextModelRunner, CLIPVersion>;
        CLIPTextFactory factory;
        factory.register_version(OPENAI_CLIP_VIT_L_14, [...]);
        factory.register_version(OPEN_CLIP_VIT_H_14, [...]);
        factory.register_version(OPEN_CLIP_VIT_BIGG_14, [...]);
        WAN_ASSERT_TRUE(factory.has_version(OPENAI_CLIP_VIT_L_14));
        WAN_ASSERT_TRUE(factory.has_version(OPEN_CLIP_VIT_H_14));
        WAN_ASSERT_TRUE(factory.has_version(OPEN_CLIP_VIT_BIGG_14));
        WAN_ASSERT_EQ(factory.registered_versions().size(), size_t(3));
    });
}

int main() {
    TestSuite suite{"CLIP Model Tests"};
    test_clip_initialization(suite);
    test_clip_factory(suite);
    suite.report();
    return 0;
}
```

### VAE 版本枚举映射

```cpp
// Source: 分析 src/model.h 的 SDVersion 枚举
// VAE 相关版本（VERSION_SD* 开头的版本 AutoEncoderKL 会使用）：
const std::vector<SDVersion> VAE_VERSIONS = {
    VERSION_SD1,    // Standard SD 1.x VAE
    VERSION_SD2,    // SD 2.x VAE
    VERSION_FLUX,   // Flux VAE
    VERSION_FLUX2,  // Flux2 VAE
};
```

### Transformer 版本映射

```cpp
// Source: 分析 src/flux.hpp 的 FluxRunner 构造函数中的 version 条件分支
const std::vector<SDVersion> FLUX_VERSIONS = {
    VERSION_FLUX,           // 标准 Flux
    VERSION_FLUX_FILL,      // Flux Fill (in_channels=384)
    VERSION_FLEX_2,         // Flex 2 (in_channels=196)
    VERSION_CHROMA_RADIANCE,// Chroma Radiance (in_channels=3, patch_size=16)
    VERSION_OVIS_IMAGE,     // Ovis Image (use_yak_mlp=true)
};
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Python-only 测试 (pytest) | C++ 单元测试 + Python 测试并存 | Phase 17 | C++ 测试直接验证内部 Runner 接口 |
| 无工厂模式（每次手动构造） | 模板工厂统一管理版本注册 | Phase 17 | 测试代码可扩展性大幅提升 |
| 测试只有集成测试 | 单元测试（合成数据） + 集成测试（真实权重） | Phase 17 | 单元测试无需 GPU 或大模型文件 |

**Deprecated/outdated:**
- 直接在测试代码中手动创建 Runner（不经过工厂）：仍然有效，但工厂模式是 Phase 17 的目标。

---

## Open Questions

1. **T5 版本枚举的归宿**
   - What we know: T5Runner 用 `bool is_umt5` 而非枚举区分版本
   - What's unclear: 是在 `model_factory.hpp` 中定义共享的 `T5Version` 枚举，还是在 `test_t5.cpp` 中本地定义
   - Recommendation: 在 `test_helpers.hpp` 中定义，供工厂和测试文件共用

2. **WAN_BUILD_TESTS CMake 选项的默认值**
   - What we know: `WAN_BUILD_EXAMPLES` 使用 `${WAN_STANDALONE}` 作为默认值
   - What's unclear: 测试是否默认开启（开启会增加构建时间）
   - Recommendation: 与 `WAN_BUILD_EXAMPLES` 对齐，使用 `${WAN_STANDALONE}` 默认值

3. **Flux::WanRunner 与 Flux::FluxRunner 的区别**
   - What we know: wan-api.cpp 使用 `WAN::WanRunner`（在 wan.hpp 中）而非 `Flux::FluxRunner`
   - What's unclear: Transformer 测试应该测试哪个 Runner 类
   - Recommendation: 测试 `Flux::FluxRunner`（在 flux.hpp 中定义），这是更底层的实现

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| C++ compiler (g++) | C++ 测试编译 | 需验证 | — | — |
| CMake 3.20+ | 测试构建系统 | 已有（主项目要求） | 3.20+ | — |
| GGML (CPU backend) | 所有 Runner 测试 | 已集成 (ggml/) | 已有 | — |
| Python + pytest | Python 测试（已有）| 已有 | — | — |

**Step 2.6: 注意** — 单元测试（D-13）使用合成数据，不需要 GPU 或真实模型权重文件。集成测试（D-14）需要真实权重文件，但这些测试可以条件性跳过（`-DWAN_INTEGRATION_TEST_DATA_DIR=...`）。

---

## Validation Architecture

> `nyquist_validation: true` — 本节包含。

### Test Framework

| Property | Value |
|----------|-------|
| Framework | 自定义 assert 框架（基于 C++ 异常 + `test_framework.hpp`）|
| Config file | `tests/cpp/CMakeLists.txt`（Wave 0 创建）|
| Quick run command | `ctest --test-dir build -R test_factory --output-on-failure` |
| Full suite command | `ctest --test-dir build --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| D-01/02 | 自定义框架支持套件、断言、报告 | unit | `ctest -R test_factory` | Wave 0 |
| D-03/04 | 模板工厂动态注册和创建 | unit | `ctest -R test_factory` | Wave 0 |
| D-05 | `register<ModelType, VersionEnum>(creator)` 接口 | unit | `ctest -R test_factory` | Wave 0 |
| D-06 | `create<ModelType>(version, ...)` 接口 | unit | `ctest -R test_factory` | Wave 0 |
| D-07 | CLIP 3版本初始化测试 | unit | `ctest -R test_clip` | Wave 0 |
| D-07 | T5/UMT5 2版本初始化测试 | unit | `ctest -R test_t5` | Wave 0 |
| D-07 | VAE 4+版本初始化测试 | unit | `ctest -R test_vae` | Wave 0 |
| D-07 | Transformer 5+版本初始化测试 | unit | `ctest -R test_transformer` | Wave 0 |
| D-10 | 析构函数正确释放 backend | unit | `ctest --output-on-failure` (valgrind 可选) | Wave 0 |

### Sampling Rate

- **Per task commit:** `cmake --build build && ctest --test-dir build --output-on-failure -R test_factory`
- **Per wave merge:** `cmake --build build && ctest --test-dir build --output-on-failure`
- **Phase gate:** 全部测试通过后才能运行 `/gsd:verify-work`

### Wave 0 Gaps

- [ ] `tests/cpp/CMakeLists.txt` — 测试构建配置
- [ ] `tests/cpp/test_framework.hpp` — 自定义轻量测试框架（D-01、D-02）
- [ ] `tests/cpp/model_factory.hpp` — 通用模板工厂（D-03 ~ D-06）
- [ ] `tests/cpp/test_helpers.hpp` — 合成张量生成工具（D-13）
- [ ] 根 `CMakeLists.txt` 需增加 `WAN_BUILD_TESTS` 选项和 `add_subdirectory(tests/cpp)`

---

## Sources

### Primary (HIGH confidence)

- `src/clip.hpp` — CLIPTextModelRunner、CLIPVisionModelProjectionRunner 构造函数和接口
- `src/t5.hpp` — T5Runner、T5Embedder 构造函数和接口
- `src/vae.hpp` — VAE 基类、AutoEncoderKL、FakeVAE 接口
- `src/flux.hpp` — Flux::FluxRunner 构造函数、版本分支逻辑
- `src/ggml_extend.hpp` — GGMLRunner 基类、GGMLRunnerContext、alloc_params_buffer
- `src/model.h` — SDVersion 枚举（VERSION_SD1 ~ VERSION_OVIS_IMAGE）、CLIPVersion 枚举
- `src/api/wan-api.cpp` — 真实初始化模式参考（make_shared + alloc_params_buffer 调用顺序）
- `CMakeLists.txt` — 现有构建系统结构，examples 子目录模式参考

### Secondary (MEDIUM confidence)

- `ggml/tests/test-backend-ops.cpp` — GGML 内置测试参考结构（C++ 测试风格）
- `.planning/phases/17-clip-text-encoder-vae-transformer/17-CONTEXT.md` — 用户决策（D-01 ~ D-14）

### Tertiary (LOW confidence)

- 无。所有关键发现均来自项目源码直接分析，置信度 HIGH。

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — 全部来自项目现有源码分析
- Architecture patterns: HIGH — 基于 wan-api.cpp 真实初始化流程
- Pitfalls: HIGH — 从 CLIPTextModelRunner/AutoEncoderKL 源码构造逻辑直接识别
- CMake integration: HIGH — 与现有 examples/ 子目录模式一致

**Research date:** 2026-03-27
**Valid until:** 2026-05-01（GGML 接口稳定；若 Runner 类接口有重大改动需重新验证）
