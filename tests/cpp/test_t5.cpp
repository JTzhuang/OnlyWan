#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_registry.hpp"
#include "t5.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

// Force-link model_factory.cpp to ensure static registrations are not DCE'd by linker.
extern "C" void wan_force_model_registrations();

// ---------------------------------------------------------------------------
// Test: initialization for both T5 versions
// ---------------------------------------------------------------------------

void test_t5_init_all_versions(TestSuite& suite) {
    wan_force_model_registrations();
    String2TensorStorage empty_map{};

    suite.run("init_standard_t5_via_registry", [&empty_map]() {
        BackendRAII guard(ggml_backend_cpu_init());
        auto runner = ModelRegistry::instance()->create<T5Runner>("t5-standard", guard.backend, false, empty_map, "");
        runner->alloc_params_buffer();
        WAN_ASSERT_TRUE(runner != nullptr);
        WAN_ASSERT_EQ(runner->get_desc(), std::string("t5"));
    });

    suite.run("init_umt5_via_registry", [&empty_map]() {
        BackendRAII guard(ggml_backend_cpu_init());
        auto runner = ModelRegistry::instance()->create<T5Runner>("t5-umt5", guard.backend, false, empty_map, "");
        runner->alloc_params_buffer();
        WAN_ASSERT_TRUE(runner != nullptr);
        WAN_ASSERT_EQ(runner->get_desc(), std::string("t5"));
    });
}

// ---------------------------------------------------------------------------
// Test: registry registration mechanics
// ---------------------------------------------------------------------------

void test_t5_registry_registration(TestSuite& suite) {
    suite.run("both_versions_registered", []() {
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<T5Runner>("t5-standard"));
        WAN_ASSERT_TRUE(ModelRegistry::instance()->has_version<T5Runner>("t5-umt5"));
    });
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"T5 Model Tests"};
    test_t5_init_all_versions(suite);
    test_t5_registry_registration(suite);
    return suite.report();
}
