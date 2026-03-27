#pragma once

#include "ggml-backend.h" // ggml_backend_t, ggml_backend_free

// Custom version enum for T5Runner, which uses bool is_umt5 instead of an enum.
// Factory creator lambdas convert: STANDARD_T5 -> is_umt5=false, UMT5 -> is_umt5=true.
// Defined here (in test_helpers.hpp) so both model_factory.hpp usage sites and
// individual test files can share the same type without redefinition.
enum class T5Version {
    STANDARD_T5,
    UMT5,
};

// RAII wrapper for ggml_backend_t.
//
// Lifetime management rationale:
//   GGMLRunner's destructor does NOT free its runtime_backend (the backend passed
//   to the GGMLRunner constructor). It only frees a separate params_backend if
//   offload_params_to_cpu created one. Therefore, test code MUST free the backend
//   explicitly. BackendRAII handles this via ggml_backend_free() in the destructor.
//
// Correct destruction order:
//   Declare BackendRAII BEFORE the Runner instance (or unique_ptr) in the same
//   scope. C++ destroys locals in reverse declaration order, so the Runner is
//   destroyed first (freeing its internal buffers), then BackendRAII frees the
//   backend. This avoids use-after-free.
//
// Example:
//   BackendRAII guard(ggml_backend_cpu_init());  // declare first
//   auto runner = factory.create(..., guard.backend, ...);  // Runner owns backend ptr but not lifetime
//   // At scope exit: runner destroyed, then guard destroyed (calls ggml_backend_free)
struct BackendRAII {
    ggml_backend_t backend;

    explicit BackendRAII(ggml_backend_t b) : backend(b) {}
    ~BackendRAII() {
        if (backend) {
            ggml_backend_free(backend);  // Safe for CPU backend
        }
    }

    // Non-copyable, non-assignable -- each test owns its backend exclusively.
    BackendRAII(const BackendRAII&) = delete;
    BackendRAII& operator=(const BackendRAII&) = delete;
};
