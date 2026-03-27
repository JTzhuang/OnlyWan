#pragma once

#include <functional>
#include <string>
#include <iostream>
#include <cmath>
#include <stdexcept>

// Lightweight single-header test framework for wan-cpp unit tests.
// Design: TestSuite wraps test cases with try/catch so failures are counted
// rather than aborting. Assertion macros throw std::runtime_error on failure
// so the suite can collect pass/fail statistics and report at the end.

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

    // Returns 0 if all tests passed, 1 if any failed.
    int report() const {
        std::cout << "\n=== " << name << " ===\n";
        std::cout << "Passed: " << passed << " / " << (passed + failed) << "\n";
        return (failed > 0) ? 1 : 0;
    }
};

// Assertion macros — throw std::runtime_error on failure so TestSuite::run
// can catch and count failures instead of aborting the process.

#define WAN_ASSERT_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            throw std::runtime_error( \
                std::string("Assert failed: " #a " == " #b \
                            " at line " + std::to_string(__LINE__))); \
        } \
    } while (0)

#define WAN_ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error( \
                std::string("Assert failed: " #cond \
                            " at line " + std::to_string(__LINE__))); \
        } \
    } while (0)

#define WAN_ASSERT_NEAR(a, b, eps) \
    do { \
        if (std::abs((a) - (b)) > (eps)) { \
            throw std::runtime_error( \
                std::string("Assert near failed at line " + std::to_string(__LINE__))); \
        } \
    } while (0)

#define WAN_ASSERT_THROWS(expr) \
    do { \
        bool _threw = false; \
        try { (expr); } catch (...) { _threw = true; } \
        if (!_threw) { \
            throw std::runtime_error( \
                std::string("Expected exception not thrown: " #expr \
                            " at line " + std::to_string(__LINE__))); \
        } \
    } while (0)
