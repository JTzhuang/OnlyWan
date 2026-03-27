#pragma once

// test_io_utils.hpp - ggml_tensor <-> .npy file I/O utilities for C++ tests.
//
// Provides load_npy() and save_npy() to read/write NumPy .npy files backed by
// ggml tensors. Supports float32, float16, int32, and int64 dtypes.
//
// Memory layout note:
//   NumPy stores arrays in row-major (C) order. ggml also uses row-major order
//   for its tensor data, so no transposition is needed for contiguous tensors.
//   ggml dimension ordering is [ne[0], ne[1], ne[2], ne[3]] where ne[0] is the
//   innermost (fastest-varying) dimension -- identical to NumPy's last axis.
//   For a 2D matrix this means ggml ne = {cols, rows}, shape = (rows, cols).
//
// Usage:
//   struct ggml_context* ctx = ...;  // preallocated ggml context
//
//   // Load:
//   ggml_tensor* t = load_npy(ctx, "data.npy");
//
//   // Save:
//   save_npy("out.npy", t);

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "ggml.h"
#include "libnpy/npy.hpp"

// ---------------------------------------------------------------------------
// load_npy
// ---------------------------------------------------------------------------
// Load a .npy file and allocate a new ggml_tensor inside `ctx`.
// Supported dtypes: float32, float16, int32, int64.
// Throws std::runtime_error on any I/O or format error.
//
// Dimension mapping:
//   .npy shape (d0, d1, ..., dn)  ->  ggml ne[0]=dn, ne[1]=d(n-1), ...
//   (ggml ne[0] is the innermost/fastest-varying dimension, matching NumPy's
//    last axis when both use row-major layout.)
static struct ggml_tensor* load_npy(struct ggml_context* ctx,
                                     const std::string& path) {
    npy::NpyArray arr = npy::load(path);

    // Map DType enum to ggml type
    enum ggml_type gtype;
    switch (arr.dtype) {
        case npy::DType::FLOAT32: gtype = GGML_TYPE_F32;  break;
        case npy::DType::FLOAT16: gtype = GGML_TYPE_F16;  break;
        case npy::DType::INT32:   gtype = GGML_TYPE_I32;  break;
        case npy::DType::INT64:   gtype = GGML_TYPE_I64;  break;
        default:
            throw std::runtime_error(
                "load_npy: unsupported dtype in " + path);
    }

    const std::vector<size_t>& shape = arr.shape;
    size_t ndim = shape.size();

    if (ndim == 0 || ndim > 4) {
        throw std::runtime_error(
            "load_npy: shape must be 1-4 dimensional, got " +
            std::to_string(ndim) + "D in " + path);
    }

    // Build ggml tensor. ggml ne[0] = innermost dim = numpy's last axis.
    // numpy shape = (d0, d1, ..., d_{n-1}), so:
    //   ne[0] = shape[n-1], ne[1] = shape[n-2], ...
    struct ggml_tensor* tensor = nullptr;
    switch (ndim) {
        case 1:
            tensor = ggml_new_tensor_1d(ctx, gtype,
                                        (int64_t)shape[0]);
            break;
        case 2:
            tensor = ggml_new_tensor_2d(ctx, gtype,
                                        (int64_t)shape[1],
                                        (int64_t)shape[0]);
            break;
        case 3:
            tensor = ggml_new_tensor_3d(ctx, gtype,
                                        (int64_t)shape[2],
                                        (int64_t)shape[1],
                                        (int64_t)shape[0]);
            break;
        case 4:
            tensor = ggml_new_tensor_4d(ctx, gtype,
                                        (int64_t)shape[3],
                                        (int64_t)shape[2],
                                        (int64_t)shape[1],
                                        (int64_t)shape[0]);
            break;
    }

    if (!tensor) {
        throw std::runtime_error(
            "load_npy: ggml_new_tensor returned null for " + path);
    }

    // Validate byte count matches
    size_t expected_bytes = arr.num_bytes();
    size_t ggml_bytes     = ggml_nbytes(tensor);
    if (expected_bytes != ggml_bytes) {
        throw std::runtime_error(
            "load_npy: byte count mismatch: npy=" +
            std::to_string(expected_bytes) +
            " ggml=" + std::to_string(ggml_bytes));
    }

    // Copy raw bytes into tensor data buffer
    std::memcpy(tensor->data, arr.data.data(), expected_bytes);
    return tensor;
}

// ---------------------------------------------------------------------------
// save_npy
// ---------------------------------------------------------------------------
// Write a ggml_tensor to a .npy file.
// Supported types: GGML_TYPE_F32, GGML_TYPE_F16, GGML_TYPE_I32, GGML_TYPE_I64.
// Throws std::runtime_error on unsupported type or I/O error.
//
// Dimension mapping (reverse of load_npy):
//   ggml ne[0]=innermost -> numpy last axis
//   For a tensor with ndims=2: numpy shape = (ne[1], ne[0])
static void save_npy(const std::string& path, struct ggml_tensor* tensor) {
    if (!tensor || !tensor->data) {
        throw std::runtime_error("save_npy: null tensor or tensor data");
    }

    // Map ggml type to npy DType enum
    npy::DType dtype;
    switch (tensor->type) {
        case GGML_TYPE_F32: dtype = npy::DType::FLOAT32; break;
        case GGML_TYPE_F16: dtype = npy::DType::FLOAT16; break;
        case GGML_TYPE_I32: dtype = npy::DType::INT32;   break;
        case GGML_TYPE_I64: dtype = npy::DType::INT64;   break;
        default:
            throw std::runtime_error(
                "save_npy: unsupported ggml type " +
                std::to_string((int)tensor->type));
    }

    // Determine number of active dimensions and build shape
    // ggml ne[0] = innermost -> maps to last numpy axis
    int ndims = ggml_n_dims(tensor);

    // Build numpy shape: reversed ggml ne (ne[ndims-1], ..., ne[0])
    std::vector<size_t> shape;
    shape.reserve(ndims);
    for (int i = ndims - 1; i >= 0; --i) {
        shape.push_back((size_t)tensor->ne[i]);
    }

    // Save raw bytes using npy::save(path, data_ptr, shape, dtype)
    npy::save(path,
              reinterpret_cast<const uint8_t*>(tensor->data),
              shape,
              dtype);
}
