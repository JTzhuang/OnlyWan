#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "model_factory.hpp"

// Including model headers directly is SAFE -- all are header-only with
// implicitly inline member functions. See review_notes in plan context.
#include "t5.hpp"
#include "ggml_extend.hpp"
#include "model.h"
#include "ggml-backend.h"

// ---------------------------------------------------------------------------
// Factory helper
// ---------------------------------------------------------------------------

// Returns a ModelFactory with both T5Version values registered.
// T5Version enum (defined in test_helpers.hpp) maps to T5Runner bool is_umt5.
static ModelFactory<T5Runner, T5Version> register_t5_factory() {
    ModelFactory<T5Runner, T5Version> factory;

    // Explicit T5Version -> bool conversion: STANDARD_T5 -> is_umt5=false
    factory.register_version(T5Version::STANDARD_T5,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<T5Runner>(backend, offload, map, prefix, /*is_umt5=*/false);
        });
    // Explicit T5Version -> bool conversion: UMT5 -> is_umt5=true
    factory.register_version(T5Version::UMT5,
        [](ggml_backend_t backend, bool offload, const String2TensorStorage& map, const std::string& prefix) {
            return std::make_unique<T5Runner>(backend, offload, map, prefix, /*is_umt5=*/true);
        });

    return factory;
}

// ---------------------------------------------------------------------------
// Test: initialization for both T5 versions
// ---------------------------------------------------------------------------

// NOTE: T5Runner::get_desc() always returns "t5" regardless of the is_umt5 flag.
// This is verified from src/t5.hpp source. Both STANDARD_T5 and UMT5 return "t5".

void test_t5_init_all_versions(TestSuite& suite) {
    auto factory = register_t5_factory();
    String2TensorStorage empty_map{};

    suite.run("init_standard_t5", [&factory, &empty_map]() {
        // BackendRAII MUST be declared BEFORE runner (reverse destruction order).
        BackendRAII guard(ggml_backend_cpu_init());
        auto runner = factory.create(T5Version::STANDARD_T5, guard.backend, false, empty_map, "");
        runner->alloc_params_buffer();
        WAN_ASSERT_TRUE(runner != nullptr);
        // T5Runner::get_desc() returns "t5" for STANDARD_T5 (is_umt5=false)
        WAN_ASSERT_EQ(runner->get_desc(), std::string("t5"));
    });

    suite.run("init_umt5", [&factory, &empty_map]() {
        // BackendRAII MUST be declared BEFORE runner (reverse destruction order).
        BackendRAII guard(ggml_backend_cpu_init());
        auto runner = factory.create(T5Version::UMT5, guard.backend, false, empty_map, "");
        runner->alloc_params_buffer();
        WAN_ASSERT_TRUE(runner != nullptr);
        // T5Runner::get_desc() returns "t5" for UMT5 (is_umt5=true) -- same as STANDARD_T5.
        // Source code verified: t5.hpp always returns "t5" regardless of is_umt5 flag.
        WAN_ASSERT_EQ(runner->get_desc(), std::string("t5"));
    });
}

// ---------------------------------------------------------------------------
// Test: factory registration mechanics
// ---------------------------------------------------------------------------

void test_t5_factory_registration(TestSuite& suite) {
    suite.run("both_versions_registered", []() {
        auto factory = register_t5_factory();
        WAN_ASSERT_EQ(factory.registered_versions().size(), size_t(2));
    });

    suite.run("has_version_check", []() {
        auto factory = register_t5_factory();
        WAN_ASSERT_TRUE(factory.has_version(T5Version::STANDARD_T5));
        WAN_ASSERT_TRUE(factory.has_version(T5Version::UMT5));
    });
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    TestSuite suite{"T5 Model Tests"};
    test_t5_init_all_versions(suite);
    test_t5_factory_registration(suite);
    return suite.report();
}
