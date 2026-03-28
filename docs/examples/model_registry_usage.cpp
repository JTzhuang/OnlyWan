// docs/examples/model_registry_usage.cpp
//
// Runnable examples demonstrating how to create and use all 4 model types
// through the ModelRegistry API.
//
// Compile with: g++ -std=c++17 -I/path/to/src -I/path/to/ggml/include \
//               model_registry_usage.cpp -o model_registry_usage
//
// Note: This file is for documentation purposes. In actual use, link against
// the compiled model_factory.cpp to ensure all 9 model registrations are active.

#include <iostream>
#include <memory>
#include <stdexcept>

// Model registry and model headers
#include "model_registry.hpp"
#include "clip.hpp"
#include "t5.hpp"
#include "wan.hpp"
#include "ggml-backend.h"
#include "model.h"

// Force-link model_factory.cpp to ensure static registrations are active
extern "C" void wan_force_model_registrations();

// RAII wrapper for ggml_backend lifecycle
struct BackendRAII {
    ggml_backend_t backend;

    explicit BackendRAII(ggml_backend_t b) : backend(b) {}

    ~BackendRAII() {
        if (backend) {
            ggml_backend_free(backend);
        }
    }

    // Non-copyable
    BackendRAII(const BackendRAII&) = delete;
    BackendRAII& operator=(const BackendRAII&) = delete;
};

// ---------------------------------------------------------------------------
// Example 1: CLIP Text Model
// ---------------------------------------------------------------------------

void example_clip_text_model() {
    std::cout << "\n=== Example 1: CLIP Text Model ===" << std::endl;

    try {
        // Initialize backend with RAII guard
        BackendRAII guard(ggml_backend_cpu_init());

        // Empty tensor map for tests (no pre-loaded weights)
        String2TensorStorage empty_map{};

        // Check if version exists before creation
        if (!ModelRegistry::instance()->has_version<CLIPTextModelRunner>("clip-vit-l-14")) {
            std::cerr << "clip-vit-l-14 not registered" << std::endl;
            return;
        }

        // Create CLIP runner
        auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>(
            "clip-vit-l-14",
            guard.backend,
            false,  // offload_params_to_cpu
            empty_map,
            ""      // prefix
        );

        std::cout << "Created CLIP runner: " << runner->get_desc() << std::endl;

        // Allocate parameter buffer
        runner->alloc_params_buffer();
        std::cout << "Parameter buffer allocated" << std::endl;

        // Runner is automatically cleaned up when it goes out of scope

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------------
// Example 2: T5 Model (both variants)
// ---------------------------------------------------------------------------

void example_t5_models() {
    std::cout << "\n=== Example 2: T5 Models ===" << std::endl;

    try {
        BackendRAII guard_std(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};

        // Create T5 Standard variant
        std::cout << "Creating T5 Standard..." << std::endl;
        if (ModelRegistry::instance()->has_version<T5Runner>("t5-standard")) {
            auto t5_std = ModelRegistry::instance()->create<T5Runner>(
                "t5-standard",
                guard_std.backend,
                false,
                empty_map,
                ""
            );
            std::cout << "T5 Standard created: " << t5_std->get_desc() << std::endl;
            t5_std->alloc_params_buffer();
            // t5_std cleaned up here
        }

        // Create T5 Multilingual variant
        std::cout << "Creating T5 Multilingual..." << std::endl;
        BackendRAII guard_umt5(ggml_backend_cpu_init());
        if (ModelRegistry::instance()->has_version<T5Runner>("t5-umt5")) {
            auto t5_umt5 = ModelRegistry::instance()->create<T5Runner>(
                "t5-umt5",
                guard_umt5.backend,
                false,
                empty_map,
                ""
            );
            std::cout << "T5 Multilingual created: " << t5_umt5->get_desc() << std::endl;
            t5_umt5->alloc_params_buffer();
            // t5_umt5 cleaned up here
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------------
// Example 3: WAN VAE (encode and decode)
// ---------------------------------------------------------------------------

void example_wan_vae() {
    std::cout << "\n=== Example 3: WAN VAE ===" << std::endl;

    try {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};

        // Create VAE Encoder
        std::cout << "Creating WAN VAE Encoder..." << std::endl;
        if (ModelRegistry::instance()->has_version<WAN::WanVAERunner>("wan-vae-t2v-encode")) {
            auto vae_encode = ModelRegistry::instance()->create<WAN::WanVAERunner>(
                "wan-vae-t2v-encode",
                guard.backend,
                false,
                empty_map,
                ""
            );
            std::cout << "VAE Encoder created: " << vae_encode->get_desc() << std::endl;
            vae_encode->alloc_params_buffer();
        }

        // Create VAE Decoder
        std::cout << "Creating WAN VAE Decoder..." << std::endl;
        if (ModelRegistry::instance()->has_version<WAN::WanVAERunner>("wan-vae-t2v-decode")) {
            auto vae_decode = ModelRegistry::instance()->create<WAN::WanVAERunner>(
                "wan-vae-t2v-decode",
                guard.backend,
                false,
                empty_map,
                ""
            );
            std::cout << "VAE Decoder created: " << vae_decode->get_desc() << std::endl;
            vae_decode->alloc_params_buffer();
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------------
// Example 4: WAN Transformer
// ---------------------------------------------------------------------------

void example_wan_transformer() {
    std::cout << "\n=== Example 4: WAN Transformer ===" << std::endl;

    try {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};

        // Create WAN Transformer
        std::cout << "Creating WAN Transformer..." << std::endl;
        if (ModelRegistry::instance()->has_version<WAN::WanTransformerRunner>("wan-transformer-t2v")) {
            auto transformer = ModelRegistry::instance()->create<WAN::WanTransformerRunner>(
                "wan-transformer-t2v",
                guard.backend,
                false,
                empty_map,
                ""
            );
            std::cout << "Transformer created: " << transformer->get_desc() << std::endl;
            transformer->alloc_params_buffer();
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------------
// Example 5: Error Handling - Unknown Version
// ---------------------------------------------------------------------------

void example_error_handling() {
    std::cout << "\n=== Example 5: Error Handling ===" << std::endl;

    BackendRAII guard(ggml_backend_cpu_init());
    String2TensorStorage empty_map{};

    try {
        // Try to create a non-existent version
        std::cout << "Attempting to create unknown CLIP version..." << std::endl;
        auto runner = ModelRegistry::instance()->create<CLIPTextModelRunner>(
            "clip-nonexistent",
            guard.backend,
            false,
            empty_map,
            ""
        );
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::cout << "WAN Model Registry Usage Examples" << std::endl;
    std::cout << "==================================" << std::endl;

    // Ensure all 9 model registrations are active
    wan_force_model_registrations();

    // Run all examples
    example_clip_text_model();
    example_t5_models();
    example_wan_vae();
    example_wan_transformer();
    example_error_handling();

    std::cout << "\n=== All examples completed ===" << std::endl;
    return 0;
}
