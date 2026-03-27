#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_registry.hpp"

// WAN Transformer (DiT) header
// NOTE: flux.hpp has been REMOVED from src/. This file no longer tests FluxRunner.
// WAN::WanRunner is the WAN DiT Transformer (replaces Flux for video generation).
#include "wan.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

// Force-link model_factory.cpp to ensure static registrations are not DCE'd by linker.
extern "C" void wan_force_model_registrations();

// ---------------------------------------------------------------------------
// WAN Transformer (WanRunner) registry test
// ---------------------------------------------------------------------------
// WAN::WanRunner supports T2V, I2V, and TI2V variants via SDVersion.

void test_wan_transformer_init(TestSuite& suite) {
    wan_force_model_registrations();

    suite.run("init_wan_runner_t2v", []() {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};
        // WAN::WanRunner constructor: (backend, offload, tensor_map, prefix, SDVersion)
        auto runner = std::make_unique<WAN::WanRunner>(
            guard.backend, false, empty_map, "", VERSION_WAN2);
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();
        // WAN::WanRunner::get_desc() returns "wan" by default if no layers found in empty map
        WAN_ASSERT_EQ(runner->get_desc(), std::string("wan"));
    });

    suite.run("init_wan_runner_i2v", []() {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};
        auto runner = std::make_unique<WAN::WanRunner>(
            guard.backend, false, empty_map, "", VERSION_WAN2_2_I2V);
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();
        WAN_ASSERT_EQ(runner->get_desc(), std::string("wan"));
    });

    suite.run("init_wan_runner_ti2v", []() {
        BackendRAII guard(ggml_backend_cpu_init());
        String2TensorStorage empty_map{};
        auto runner = std::make_unique<WAN::WanRunner>(
            guard.backend, false, empty_map, "", VERSION_WAN2_2_TI2V);
        WAN_ASSERT_TRUE(runner != nullptr);
        runner->alloc_params_buffer();
        WAN_ASSERT_EQ(runner->get_desc(), std::string("wan"));
    });
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"WAN Transformer Tests"};
    test_wan_transformer_init(suite);
    return suite.report();
}
