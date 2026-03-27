#pragma once
// libnpy - header-only C++ library for reading and writing .npy files
// .npy format: https://numpy.org/doc/stable/reference/generated/numpy.lib.format.html
//
// Supported dtypes:
//   float32  '<f4' / '>f4'
//   float16  '<f2' / '>f2'  (stored as raw uint16_t bytes)
//   int32    '<i4' / '>i4'
//   int64    '<i8' / '>i8'
//
// Usage example:
//   // Save
//   std::vector<float> v = {1.f, 2.f, 3.f};
//   npy::save("out.npy", v.data(), {3}, npy::DType::FLOAT32);
//
//   // Load
//   npy::NpyArray arr = npy::load("in.npy");
//   float* ptr = reinterpret_cast<float*>(arr.data.data());

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace npy {

// ---------------------------------------------------------------------------
// Data type descriptor
// ---------------------------------------------------------------------------

enum class DType {
    FLOAT32,
    FLOAT16,
    INT32,
    INT64,
    UNKNOWN
};

inline size_t dtype_itemsize(DType dt) {
    switch (dt) {
        case DType::FLOAT32: return 4;
        case DType::FLOAT16: return 2;
        case DType::INT32:   return 4;
        case DType::INT64:   return 8;
        default:             return 0;
    }
}

// Returns the NumPy descriptor string, e.g. "<f4"
inline std::string dtype_to_str(DType dt) {
    switch (dt) {
        case DType::FLOAT32: return "<f4";
        case DType::FLOAT16: return "<f2";
        case DType::INT32:   return "<i4";
        case DType::INT64:   return "<i8";
        default: throw std::runtime_error("npy: unknown dtype");
    }
}

// Parses "<f4", ">f4", "<f2", "<i4", "<i8" etc.
inline DType dtype_from_str(const std::string& raw) {
    // strip whitespace and quotes
    std::string s = raw;
    auto strip = [](std::string& str) {
        while (!str.empty() && (str.front() == ' ' || str.front() == '\'' || str.front() == '"'))
            str.erase(str.begin());
        while (!str.empty() && (str.back()  == ' ' || str.back()  == '\'' || str.back()  == '"'))
            str.pop_back();
    };
    strip(s);
    if (s.size() < 3) return DType::UNKNOWN;
    // s[0] = endian ('<' or '>' or '|')
    char kind = s[1];
    int  sz   = 0;
    try { sz = std::stoi(s.substr(2)); } catch (...) { return DType::UNKNOWN; }
    if (kind == 'f' && sz == 4) return DType::FLOAT32;
    if (kind == 'f' && sz == 2) return DType::FLOAT16;
    if (kind == 'i' && sz == 4) return DType::INT32;
    if (kind == 'i' && sz == 8) return DType::INT64;
    return DType::UNKNOWN;
}

// ---------------------------------------------------------------------------
// NpyArray: result of load()
// ---------------------------------------------------------------------------

struct NpyArray {
    std::vector<uint8_t> data;          // raw element bytes in C order
    std::vector<size_t>  shape;         // dimension sizes, e.g. {3, 4}
    DType                dtype;         // element type
    bool                 fortran_order; // true = column-major (Fortran)

    size_t num_elements() const {
        size_t n = 1;
        for (auto d : shape) n *= d;
        return n;
    }
    size_t num_bytes() const {
        return num_elements() * dtype_itemsize(dtype);
    }
};

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace detail {

// Trim whitespace from both ends
inline std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t a = s.find_first_not_of(ws);
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

// Find value string for a dict key like 'descr' in the .npy header
// Returns "" if not found
inline std::string find_field(const std::string& header, const std::string& key) {
    for (const std::string quote : {"'", "\""}) {
        std::string needle = quote + key + quote + ":";
        size_t pos = header.find(needle);
        if (pos == std::string::npos) continue;
        pos += needle.size();
        // skip whitespace
        while (pos < header.size() && header[pos] == ' ') pos++;
        return header.substr(pos);
    }
    return "";
}

// Parse the descr value (quoted string like 'descr': '<f4')
inline std::string parse_descr(const std::string& header) {
    std::string rest = find_field(header, "descr");
    if (rest.empty()) throw std::runtime_error("npy: 'descr' not found in header");
    // rest starts at the value, e.g. "'<f4', ..."
    // find opening quote
    size_t q1 = rest.find_first_of("'\"" );
    if (q1 == std::string::npos) throw std::runtime_error("npy: bad descr value");
    char qc = rest[q1];
    size_t q2 = rest.find(qc, q1 + 1);
    if (q2 == std::string::npos) throw std::runtime_error("npy: unterminated descr quote");
    return rest.substr(q1 + 1, q2 - q1 - 1);
}

// Parse fortran_order value -> bool
inline bool parse_fortran_order(const std::string& header) {
    std::string rest = find_field(header, "fortran_order");
    if (rest.empty()) return false;
    return (rest.substr(0, 4) == "True");
}

// Parse shape value: e.g. (3,) or (3, 4) or ()
// Returns vector of size_t
inline std::vector<size_t> parse_shape(const std::string& header) {
    std::string rest = find_field(header, "shape");
    if (rest.empty()) throw std::runtime_error("npy: 'shape' not found in header");
    size_t p1 = rest.find('(');
    size_t p2 = rest.find(')');
    if (p1 == std::string::npos || p2 == std::string::npos)
        throw std::runtime_error("npy: bad shape value");
    std::string inner = rest.substr(p1 + 1, p2 - p1 - 1);
    std::vector<size_t> shape;
    std::stringstream ss(inner);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = trim(token);
        if (!token.empty()) shape.push_back(static_cast<size_t>(std::stoull(token)));
    }
    return shape;
}

// Write a little-endian uint16_t
inline void write_u16le(std::ostream& os, uint16_t v) {
    uint8_t buf[2] = {static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>((v >> 8) & 0xFF)};
    os.write(reinterpret_cast<char*>(buf), 2);
}

// Read a little-endian uint16_t
inline uint16_t read_u16le(std::istream& is) {
    uint8_t buf[2];
    is.read(reinterpret_cast<char*>(buf), 2);
    return static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8);
}

} // namespace detail

// ---------------------------------------------------------------------------
// load() - read a .npy file from disk
// ---------------------------------------------------------------------------

inline NpyArray load(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
        throw std::runtime_error("npy::load: cannot open file: " + path);

    // Magic: \x93NUMPY
    uint8_t magic[6];
    ifs.read(reinterpret_cast<char*>(magic), 6);
    if (magic[0] != 0x93 || magic[1] != 'N' || magic[2] != 'U' ||
        magic[3] != 'M'  || magic[4] != 'P'  || magic[5] != 'Y')
        throw std::runtime_error("npy::load: not a .npy file: " + path);

    // Version
    uint8_t major_ver, minor_ver;
    ifs.read(reinterpret_cast<char*>(&major_ver), 1);
    ifs.read(reinterpret_cast<char*>(&minor_ver), 1);
    (void)minor_ver;

    // Header length (2 or 4 bytes depending on version)
    uint32_t header_len = 0;
    if (major_ver == 1) {
        header_len = detail::read_u16le(ifs);
    } else if (major_ver == 2 || major_ver == 3) {
        uint8_t buf[4];
        ifs.read(reinterpret_cast<char*>(buf), 4);
        header_len = static_cast<uint32_t>(buf[0])
                   | (static_cast<uint32_t>(buf[1]) << 8)
                   | (static_cast<uint32_t>(buf[2]) << 16)
                   | (static_cast<uint32_t>(buf[3]) << 24);
    } else {
        throw std::runtime_error("npy::load: unsupported .npy version " + std::to_string(major_ver));
    }

    // Read header string
    std::string header(header_len, '\0');
    ifs.read(&header[0], header_len);

    // Parse header fields
    std::string descr_str  = detail::parse_descr(header);
    bool        fortran    = detail::parse_fortran_order(header);
    std::vector<size_t> shape = detail::parse_shape(header);

    DType dtype = dtype_from_str(descr_str);
    if (dtype == DType::UNKNOWN)
        throw std::runtime_error("npy::load: unsupported dtype '" + descr_str + "'");

    // Compute total bytes
    size_t n_elems = 1;
    for (auto d : shape) n_elems *= d;
    size_t n_bytes = n_elems * dtype_itemsize(dtype);

    // Read data
    std::vector<uint8_t> data(n_bytes);
    ifs.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(n_bytes));
    if (!ifs && n_bytes > 0)
        throw std::runtime_error("npy::load: unexpected end of file reading data: " + path);

    NpyArray arr;
    arr.data         = std::move(data);
    arr.shape        = std::move(shape);
    arr.dtype        = dtype;
    arr.fortran_order = fortran;
    return arr;
}

// ---------------------------------------------------------------------------
// save() - write a .npy file to disk
// ---------------------------------------------------------------------------

// Save raw bytes with given shape and dtype
inline void save(const std::string&          path,
                 const void*                  data,
                 const std::vector<size_t>&   shape,
                 DType                        dtype,
                 bool                         fortran_order = false)
{
    // Build header dict string
    std::string descr = dtype_to_str(dtype);
    std::string fo_str = fortran_order ? "True" : "False";

    std::string shape_str = "(";
    for (size_t i = 0; i < shape.size(); i++) {
        shape_str += std::to_string(shape[i]);
        shape_str += ",";
        if (i + 1 < shape.size()) shape_str += " ";
    }
    shape_str += ")";

    std::string dict = "{\"descr\": \"" + descr + "\", \"fortran_order\": " + fo_str +
                       ", \"shape\": " + shape_str + ", }";

    // The header must be padded to a multiple of 64 bytes (version 1.0).
    // Total prefix = magic(6) + version(2) + header_len_field(2) = 10 bytes.
    // Header string + newline must fill the rest.
    const size_t PREFIX = 10;
    size_t       header_str_len = dict.size() + 1; // +1 for '\n'
    size_t       total          = PREFIX + header_str_len;
    size_t       pad            = (64 - (total % 64)) % 64;
    // pad with spaces before the terminating newline
    dict.append(pad, ' ');
    dict += '\n';

    uint16_t header_len = static_cast<uint16_t>(dict.size());

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open())
        throw std::runtime_error("npy::save: cannot open file for writing: " + path);

    // Magic
    const uint8_t magic[6] = {0x93, 'N', 'U', 'M', 'P', 'Y'};
    ofs.write(reinterpret_cast<const char*>(magic), 6);

    // Version 1.0
    ofs.put(1);
    ofs.put(0);

    // Header length (little-endian uint16)
    detail::write_u16le(ofs, header_len);

    // Header dict
    ofs.write(dict.c_str(), static_cast<std::streamsize>(dict.size()));

    // Data
    size_t n_elems = 1;
    for (auto d : shape) n_elems *= d;
    size_t n_bytes = n_elems * dtype_itemsize(dtype);
    ofs.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(n_bytes));

    if (!ofs)
        throw std::runtime_error("npy::save: write error: " + path);
}

// Convenience overload: save a std::vector<T>
template<typename T>
void save(const std::string&         path,
          const std::vector<T>&       data,
          const std::vector<size_t>&  shape,
          DType                       dtype)
{
    save(path, data.data(), shape, dtype);
}

} // namespace npy
