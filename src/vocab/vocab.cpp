/**
 * @file vocab.cpp
 * @brief Vocabulary loading — mmap from external files, with optional embedded fallback.
 *
 * Default build (WAN_EMBED_VOCAB not defined): loads vocab from external files
 * in the directory set by wan_vocab_set_dir(). Returns empty string if files
 * are absent.
 *
 * WAN_EMBED_VOCAB=ON build: falls back to embedded hex arrays in .hpp files
 * when external files are not found.
 */

#include "vocab.h"

#include <string>
#include <cstdio>

#ifndef _WIN32
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <unistd.h>
#else
#  include <windows.h>
#endif

static std::string g_vocab_dir;

void wan_vocab_set_dir(const std::string& dir) {
    g_vocab_dir = dir;
}

bool wan_vocab_dir_is_set() {
    return !g_vocab_dir.empty();
}

const std::string& wan_vocab_get_dir() {
    return g_vocab_dir;
}

static std::string mmap_read(const std::string& path) {
#ifndef _WIN32
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) return {};
    struct stat st;
    if (::fstat(fd, &st) < 0) { ::close(fd); return {}; }
    size_t sz = (size_t)st.st_size;
    if (sz == 0) { ::close(fd); return {}; }
    void* addr = ::mmap(nullptr, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (addr == MAP_FAILED) return {};
    std::string result(static_cast<const char*>(addr), sz);
    ::munmap(addr, sz);
    return result;
#else
    // Windows fallback: read via FILE*
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return {};
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return {}; }
    std::string result(sz, '\0');
    fread(&result[0], 1, (size_t)sz, f);
    fclose(f);
    return result;
#endif
}

static std::string load_vocab_file(const std::string& filename) {
    if (!g_vocab_dir.empty()) {
        std::string s = mmap_read(g_vocab_dir + "/" + filename);
        if (!s.empty()) return s;
    }
    return {};
}

std::string load_umt5_tokenizer_json() {
    std::string s = load_vocab_file("umt5_tokenizer.json");
    if (!s.empty()) return s;
#ifdef WAN_EMBED_VOCAB
#  include "umt5.hpp"
    return std::string(reinterpret_cast<const char*>(umt5_tokenizer_json_str),
                       sizeof(umt5_tokenizer_json_str));
#else
    return {};
#endif
}

std::string load_t5_tokenizer_json() {
    std::string s = load_vocab_file("t5_tokenizer.json");
    if (!s.empty()) return s;
#ifdef WAN_EMBED_VOCAB
#  include "clip_t5.hpp"
    return std::string(reinterpret_cast<const char*>(t5_tokenizer_json_str),
                       sizeof(t5_tokenizer_json_str));
#else
    return {};
#endif
}

std::string load_clip_merges() {
    std::string s = load_vocab_file("clip_merges.txt");
    if (!s.empty()) return s;
#ifdef WAN_EMBED_VOCAB
#  ifndef CLIP_T5_HPP_INCLUDED
#  include "clip_t5.hpp"
#  define CLIP_T5_HPP_INCLUDED
#  endif
    return std::string(reinterpret_cast<const char*>(clip_merges_utf8_c_str),
                       sizeof(clip_merges_utf8_c_str));
#else
    return {};
#endif
}

std::string load_qwen2_merges() {
    std::string s = load_vocab_file("qwen2_merges.txt");
    if (!s.empty()) return s;
#ifdef WAN_EMBED_VOCAB
#  include "qwen.hpp"
    return std::string(reinterpret_cast<const char*>(qwen2_merges_utf8_c_str),
                       sizeof(qwen2_merges_utf8_c_str));
#else
    return {};
#endif
}

std::string load_mistral_merges() {
    std::string s = load_vocab_file("mistral_merges.txt");
    if (!s.empty()) return s;
#ifdef WAN_EMBED_VOCAB
#  include "mistral.hpp"
    return std::string(reinterpret_cast<const char*>(mistral_merges_utf8_c_str),
                       sizeof(mistral_merges_utf8_c_str));
#else
    return {};
#endif
}

std::string load_mistral_vocab_json() {
    std::string s = load_vocab_file("mistral_vocab.json");
    if (!s.empty()) return s;
#ifdef WAN_EMBED_VOCAB
#  ifndef MISTRAL_HPP_INCLUDED
#  include "mistral.hpp"
#  define MISTRAL_HPP_INCLUDED
#  endif
    return std::string(reinterpret_cast<const char*>(mistral_vocab_json_utf8_c_str),
                       sizeof(mistral_vocab_json_utf8_c_str));
#else
    return {};
#endif
}
