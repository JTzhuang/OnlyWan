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
// Supported dtypes: float32 ('<f4'), float16 ('<f2'), int32 ('<i4'), int64 ('<i8').
// Throws std::runtime_error on any I/O or format error.
//
// Dimension mapping:
//   .npy shape (d0, d1, ..., dn)  ->  ggml ne[0]=dn, ne[1]=d(n-1), ...
//   (ggml ne[0] is the innermost/fastest-varying dimension, matching NumPy's
//    last axis when both use row-major layout.)
static struct ggml_tensor* load_npy(struct ggml_context* ctx,
                                     const std::string& path) {
    npy::NpyArray arr = npy::load(path);

    // Map dtype string to ggml type
    enum ggml_type gtype;
    const std::string& dtype = arr.dtype;
    if (dtype == "<f4" || dtype == "=f4" || dtype == ">f4" || dtype == "f4") {
        gtype = GGML_TYPE_F32;
    } else if (dtype == "<f2" || dtype == "=f2" || dtype == ">f2" || dtype == "f2") {
        gtype = GGML_TYPE_F16;
    } else if (dtype == "<i4" || dtype == "=i4" || dtype == ">i4" || dtype == "i4") {
        gtype = GGML_TYPE_I32;
    } else if (dtype == "<i8" || dtype == "=i8" || dtype == ">i8" || dtype == "i8") {
        gtype = GGML_TYPE_I64;
    } else {
        throw std::runtime_error("load_npy: unsupported dtype '" + dtype +
                                 "' in file " + path);
    }

    const std::vector<unsigned long>& shape = arr.shape;
    if (shape.empty() || shape.size() > 4) {
        throw std::runtime_error("load_npy: shape must be 1-4 dimensional, got " +
                                 std::to_string(shape.size()) + "D in " + path);
    }

    // ggml ne[0] = innermost (last numpy axis), ne[1] = second-to-last, etc.
    struct ggml_tensor* tensor = nullptr;
    switch (shape.size()) {
        case 1:
            tensor = ggml_new_tensor_1d(ctx, gtype, (int64_t)shape[0]);
            break;
        case 2:
            // numpy shape (rows, cols) -> ggml ne[0]=cols, ne[1]=rows
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
        throw std::runtime_error("load_npy: ggml_new_tensor failed for " + path);
    }

    // Copy raw bytes from npy buffer into tensor data
    size_t nbytes = ggml_nbytes(tensor);
    if (arr.data.size() != nbytes) {
        throw std::runtime_error(
            "load_npy: size mismatch: npy has " + std::to_string(arr.data.size()) +
            " bytes but tensor expects " + std::to_string(nbytes) +
            " bytes (file: " + path + ")");
    }
    std::memcpy(tensor->data, arr.data.data(), nbytes);

    return tensor;
}

// ---------------------------------------------------------------------------
// save_npy
// ---------------------------------------------------------------------------
// Save a contiguous ggml_tensor to a .npy file.
// Supported types: GGML_TYPE_F32, GGML_TYPE_F16, GGML_TYPE_I32, GGML_TYPE_I64.
// Throws std::runtime_error on any error.
//
// Dimension mapping (inverse of load_npy):
//   ggml ne[0]=cols, ne[1]=rows  ->  .npy shape (rows, cols)
static void save_npy(const std::string& path, const struct ggml_tensor* tensor) {
    if (!tensor) {
        throw std::runtime_error("save_npy: null tensor");
    }
    if (!tensor->data) {
        throw std::runtime_error("save_npy: tensor has no data");
    }

    // Map ggml type to npy dtype string
    std::string dtype;
    switch (tensor->type) {
        case GGML_TYPE_F32: dtype = "<f4"; break;
        case GGML_TYPE_F16: dtype = "<f2"; break;
        case GGML_TYPE_I32: dtype = "<i4"; break;
        case GGML_TYPE_I64: dtype = "<i8"; break;
        default:
            throw std::runtime_error(
                "save_npy: unsupported ggml type " +
                std::to_string((int)tensor->type));
    }

    // Determine number of active dimensions and build shape
    // ggml ne[0] = innermost -> maps to last numpy axis
    int ndims = ggml_n_dims(tensor);

    // Build numpy shape: reversed ggml ne (ne[ndims-1], ..., ne[0])
    std::vector<unsigned long> shape;
    shape.reserve(ndims);
    for (int i = ndims - 1; i >= 0; --i) {
        shape.push_back((unsigned long)tensor->ne[i]);
    }

    // Copy tensor data
    size_t nbytes = ggml_nbytes(tensor);
    std::vector<char> data(nbytes);
    std::memcpy(data.data(), tensor->data, nbytes);

    npy::save(path, dtype, shape, data);
}
