/**
 * @file wan-helpers.cpp
 * @brief Helper functions implementation for Wan API
 */

#include "wan-helpers.hpp"

#include <cstdarg>
#include <cstring>
#include <memory>
#include <string>

// Define the internal context structure (same as in wan-api.cpp)
struct wan_context {
    std::string last_error;
    std::string model_path;
    WanModelPtr model;
    WanBackendPtr backend;
    WanVAEPtr vae;
    WanParams params;
    int n_threads;
    std::string backend_type;
};

// Global state for callbacks
static wan_log_cb_t g_log_callback = nullptr;
static void* g_log_callback_user_data = nullptr;

/* ============================================================================
 * Error Handling Helpers
 * ============================================================================ */

void wan_set_last_error(wan_context_t* ctx, const char* error_msg) {
    if (ctx) {
        ctx->last_error = error_msg ? error_msg : "Unknown error";
    }
}

void wan_set_last_error_fmt(wan_context_t* ctx, const char* fmt, ...) {
    if (!ctx) return;

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    ctx->last_error = std::string(buffer);
}

/* ============================================================================
 * Global State Accessors
 * ============================================================================ */

wan_log_cb_t wan_get_log_callback(void) {
    return g_log_callback;
}

void* wan_get_log_callback_user_data(void) {
    return g_log_callback_user_data;
}

/* ============================================================================
 * Global State Setters
 * ============================================================================ */

void wan_set_log_callback_internal(wan_log_cb_t callback, void* user_data) {
    g_log_callback = callback;
    g_log_callback_user_data = user_data;
}
