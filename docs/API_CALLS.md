# API Call Reference

This document provides comprehensive API call signatures and usage patterns for the two main APIs in the WAN C++ project:
1. **Model Registry API** — Create and manage model runners
2. **I/O Utilities API** — Load and save .npy tensor files

---

## Model Registry API

The `ModelRegistry` is a thread-safe singleton that manages model creation and version checking. All 9 registered models can be instantiated through this unified interface.

### Singleton Access

```cpp
ModelRegistry* registry = ModelRegistry::instance();
```

Returns a pointer to the global `ModelRegistry` singleton. Thread-safe.

### API Functions

#### `has_version<T>(version: string) -> bool`

Check if a specific model version is registered.

**Template Parameter:**
- `T` — Model runner type (e.g., `CLIPTextModelRunner`, `T5Runner`, `WAN::WanVAERunner`, `WAN::WanTransformerRunner`)

**Parameters:**
- `version` — Version string identifier (see registered models below)

**Returns:** `true` if the version exists, `false` otherwise

**Example:**
```cpp
if (ModelRegistry::instance()->has_version<CLIPTextModelRunner>("clip-vit-l-14")) {
    // Version is available
}
```

#### `create<T>(version, backend, offload_params_to_cpu, tensor_map, prefix) -> unique_ptr<T>`

Create a model runner instance.

**Template Parameter:**
- `T` — Model runner type

**Parameters:**
- `version` (string) — Version identifier
- `backend` (ggml_backend_t) — GGML backend (e.g., `ggml_backend_cpu_init()`)
- `offload_params_to_cpu` (bool) — Whether to offload parameters to CPU (for GPU backends)
- `tensor_map` (const String2TensorStorage&) — Pre-loaded model weights (empty map for tests)
- `prefix` (string, optional) — Prefix for tensor names in the map (default: "")

**Returns:** `unique_ptr<T>` — Unique pointer to the created runner

**Throws:** `std::runtime_error` if version is unknown

**Example:**
```cpp
BackendRAII guard(ggml_backend_cpu_init());
String2TensorStorage empty_map{};
auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>(
    "clip-vit-l-14",
    guard.backend,
    false,
    empty_map,
    ""
);
runner->alloc_params_buffer();
```

### Registered Models (9 Total)

#### CLIP Text Encoder (3 versions)
- `"clip-vit-l-14"` — ViT-L/14 model
- `"clip-vit-h-14"` — ViT-H/14 model
- `"clip-vit-bigg-14"` — ViT-bigG/14 model

**Runner Type:** `CLIPTextModelRunner`

#### T5 Text Encoder (2 versions)
- `"t5-standard"` — Standard T5 model
- `"t5-umt5"` — Multilingual T5 model

**Runner Type:** `T5Runner`

#### WAN VAE (2 versions)
- `"wan-vae-t2v-encode"` — Text-to-video VAE encoder
- `"wan-vae-t2v-decode"` — Text-to-video VAE decoder

**Runner Type:** `WAN::WanVAERunner`

#### WAN Transformer (1 version)
- `"wan-transformer-t2v"` — Text-to-video transformer

**Runner Type:** `WAN::WanTransformerRunner`

### Lifecycle Management: BackendRAII

Use `BackendRAII` to manage backend lifecycle:

```cpp
{
    BackendRAII guard(ggml_backend_cpu_init());
    // Use guard.backend for model creation
    auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>(
        "clip-vit-l-14",
        guard.backend,
        false,
        empty_map,
        ""
    );
    // Backend is automatically freed when guard goes out of scope
}
```

### Empty Tensor Map Pattern (for Tests)

When testing without actual model weights:

```cpp
String2TensorStorage empty_map{};
auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>(
    "clip-vit-l-14",
    backend,
    false,
    empty_map,  // Empty map — runner will allocate dummy tensors
    ""
);
```

---

## I/O Utilities API

The I/O utilities provide functions to load and save GGML tensors in NumPy `.npy` format. Supports float32, float16, int32, and int64 data types.

### API Functions

#### `load_npy(ctx, path) -> ggml_tensor*`

Load a `.npy` file and allocate a new tensor in the given context.

**Parameters:**
- `ctx` (ggml_context*) — GGML context for tensor allocation
- `path` (const string&) — Path to the `.npy` file

**Returns:** `ggml_tensor*` — Pointer to the newly allocated tensor (owned by context)

**Throws:** `std::runtime_error` if:
- File does not exist or cannot be read
- Unsupported dtype (only F32, F16, I32, I64 supported)
- Shape is not 1-4 dimensional

**Dimension Mapping:**
- NumPy shape `(d0, d1, ..., dn)` → ggml `ne[0]=dn, ne[1]=d(n-1), ...`
- ggml `ne[0]` is the innermost (fastest-varying) dimension, matching NumPy's last axis

**Example:**
```cpp
GGMLCtxRAII ctx;
struct ggml_tensor* t = load_npy(ctx.ctx, "data.npy");
// Use tensor...
// Automatically freed when ctx goes out of scope
```

#### `save_npy(path, tensor) -> void`

Write a tensor to a `.npy` file.

**Parameters:**
- `path` (const string&) — Output file path
- `tensor` (ggml_tensor*) — Tensor to save

**Throws:** `std::runtime_error` if:
- Tensor is null or has no data
- Unsupported ggml type (only F32, F16, I32, I64 supported)
- File cannot be written

**Dimension Mapping (reverse of load_npy):**
- ggml `ne[0]=innermost` → NumPy last axis
- For 2D tensor: NumPy shape = `(ne[1], ne[0])`

**Example:**
```cpp
struct ggml_tensor* t = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 4, 3);
// Fill tensor with data...
save_npy("output.npy", t);
```

### Supported Data Types

| GGML Type | NumPy dtype | Size | Use Case |
|-----------|------------|------|----------|
| `GGML_TYPE_F32` | `float32` | 4 bytes | Full precision floats |
| `GGML_TYPE_F16` | `float16` | 2 bytes | Half precision floats |
| `GGML_TYPE_I32` | `int32` | 4 bytes | 32-bit integers |
| `GGML_TYPE_I64` | `int64` | 8 bytes | 64-bit integers |

### Lifecycle Management: GGMLCtxRAII

Use `GGMLCtxRAII` to manage GGML context lifecycle:

```cpp
{
    GGMLCtxRAII ctx(64 * 1024 * 1024);  // 64MB context
    struct ggml_tensor* t = load_npy(ctx.ctx, "data.npy");
    // Use tensor...
    // Context is automatically freed when ctx goes out of scope
}
```

**Constructor Parameters:**
- `mem_bytes` (size_t, optional) — Context memory size in bytes (default: 64MB)

### Temporary File Pattern (for Tests)

Use `TempFile` for automatic cleanup:

```cpp
{
    TempFile tmp("test_data.npy");
    save_npy(tmp.path, tensor);
    // Use tmp.path...
    // File is automatically deleted when tmp goes out of scope
}
```

### Dimension Mapping Details

**1D Tensor:**
- NumPy shape: `(n,)` → ggml `ne[0]=n`

**2D Tensor (Matrix):**
- NumPy shape: `(rows, cols)` → ggml `ne[0]=cols, ne[1]=rows`
- Example: 3×4 matrix in NumPy → ggml `ne[0]=4, ne[1]=3`

**3D Tensor:**
- NumPy shape: `(d0, d1, d2)` → ggml `ne[0]=d2, ne[1]=d1, ne[2]=d0`

**4D Tensor:**
- NumPy shape: `(d0, d1, d2, d3)` → ggml `ne[0]=d3, ne[1]=d2, ne[2]=d1, ne[3]=d0`

---

## Common Patterns

### Error Handling

```cpp
try {
    auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>(
        "clip-unknown",
        backend,
        false,
        empty_map,
        ""
    );
} catch (const std::runtime_error& e) {
    std::cerr << "Failed to create model: " << e.what() << std::endl;
}
```

### Version Checking Before Creation

```cpp
if (ModelRegistry::instance()->has_version<T5Runner>("t5-standard")) {
    auto runner = ModelRegistry::instance()->create<T5Runner>(
        "t5-standard",
        backend,
        false,
        empty_map,
        ""
    );
} else {
    std::cerr << "t5-standard not available" << std::endl;
}
```

### Round-trip Save/Load

```cpp
GGMLCtxRAII ctx1;
struct ggml_tensor* t1 = ggml_new_tensor_2d(ctx1.ctx, GGML_TYPE_F32, 4, 3);
// Fill t1 with data...
save_npy("data.npy", t1);

GGMLCtxRAII ctx2;
struct ggml_tensor* t2 = load_npy(ctx2.ctx, "data.npy");
// t2 has same shape and data as t1
```
