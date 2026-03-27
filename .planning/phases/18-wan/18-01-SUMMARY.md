# Phase 18 Plan 01: Model Registration Refactor Summary

## Summary
- Migrated model registration from test-only enum-based factories to a production-ready infrastructure in `src/` using compile-time macros and string-based versioning.
- Established `ModelRegistry` singleton with type-safe registration and creation of model runners (CLIP, T5, WAN VAE).
- Registered 9 WAN-specific model variants: CLIP (3), T5 (2), and WAN VAE (4).
- Handled the deletion of `src/flux.hpp` by rewriting `test_transformer.cpp` to test `WAN::WanRunner` (WAN DiT) instead of `FluxRunner`.
- Verified the new registry system with 5/5 passing `ctest` binaries.

## Tech Stack
- C++17
- GGML (Tensor library)
- Singleton Pattern
- Static Initialization (Macro-based registration)

## Key Files
- `src/model_registry.hpp`: Generic `ModelRegistry` singleton with `REGISTER_MODEL_FACTORY` macro.
- `src/model_registry.cpp`: Registry implementation.
- `src/model_factory.hpp`: Single include point for model headers and `wan_force_model_registrations()` declaration.
- `src/model_factory.cpp`: 9 model registrations and `wan_force_model_registrations()` definition to prevent linker DCE.
- `tests/cpp/test_factory.cpp`: Registry unit tests.
- `tests/cpp/test_clip.cpp`: Updated CLIP tests using string-based registry.
- `tests/cpp/test_t5.cpp`: Updated T5 tests using string-based registry.
- `tests/cpp/test_vae.cpp`: Updated VAE tests using `WanVAERunner` and string-based registry.
- `tests/cpp/test_transformer.cpp`: Rewritten to test `WAN::WanRunner` (T2V/I2V/TI2V) instead of Flux.

## Decisions Made
- **Type-safe Registry**: Used a template-based registry that stores type-erased factory functions, allowing retrieval of `unique_ptr<ModelType>` without a common base class for all runners.
- **String-based Versioning**: Replaced enums with descriptive strings (e.g., `"clip-vit-l-14"`, `"wan-vae-t2v"`) for better extensibility and future API exposure.
- **Linker DCE Guard**: Implemented `extern "C" void wan_force_model_registrations()` to ensure the `model_factory.cpp` translation unit (which contains only static registrations) is not discarded by the linker during test binary creation.
- **Macro Robustness**: Used `__LINE__` with double-expansion helper macros (`CONCAT`) in `REGISTER_MODEL_FACTORY` to generate unique names even for types with namespaces (e.g., `WAN::WanVAERunner`) and version strings with hyphens/quotes.

## Deviations from Plan
- **WanRunner Constructor Requirements**: Discovered that `WanRunner` constructor requires `num_layers` to be 30 or 40 (or it aborts). Updated `test_transformer.cpp` to include dummy tensors in the `tensor_storage_map` to trigger layer detection.
- **WanRunner Description**: Source-verified that `WanRunner::get_desc()` returns specific strings like `"Wan2.x-T2V-14B"` or `"Wan2.2-TI2V-5B"` based on detected layers and version, instead of a static `"wan"`.
- **Macro Fix**: The initial `REGISTER_MODEL_FACTORY` implementation failed because `ModelType` and `VersionString` could not be directly concatenated if they contained symbols (`::`, `-`, `"`). Switched to `__LINE__`-based unique identifiers with proper expansion helpers.

## Test Results
- **test_factory**: PASS (CLIP, T5, WAN VAE registry tests)
- **test_clip**: PASS (3 CLIP versions)
- **test_t5**: PASS (2 T5 versions)
- **test_vae**: PASS (4 WAN VAE versions)
- **test_transformer**: PASS (WAN DiT T2V/I2V/TI2V initialization)

## Self-Check: PASSED
- [x] All 5 test targets compile and pass.
- [x] 9 model variants registered in `src/`.
- [x] No `flux.hpp` references in updated code.
- [x] Registry is string-based and macro-driven.
