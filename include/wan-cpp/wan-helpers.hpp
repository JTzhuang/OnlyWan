/**
 * @file wan-helpers.hpp
 * @brief Helper functions for Wan API implementation
 *
 * This file contains helper functions used across the C API implementation.
 */

#ifndef WAN_HELPERS_HPP
#define WAN_HELPERS_HPP

#include "wan.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Error Handling Helpers
 * ============================================================================ */

/**
 * @brief Set last error message for a context
 *
 * This function is used internally by the API implementation
 * to set error messages that can be retrieved via wan_get_error_message.
 */
void wan_set_last_error(wan_context_t* ctx, const char* error_msg);

/**
 * @brief Set formatted last error message
 */
void wan_set_last_error_fmt(wan_context_t* ctx, const char* fmt, ...);

/* ============================================================================
 * Global State Accessors
 * ============================================================================ */

/**
 * @brief Get the global log callback
 */
wan_log_cb_t wan_get_log_callback(void);

/**
 * @brief Get the global log callback user data
 */
void* wan_get_log_callback_user_data(void);

/* ============================================================================
 * Global State Setters (for internal use)
 * ============================================================================ */

/**
 * @brief Set the global log callback
 */
void wan_set_log_callback_internal(wan_log_cb_t callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* WAN_HELPERS_HPP */
