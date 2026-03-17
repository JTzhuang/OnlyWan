# Phase 5: Encoders - Implementation Plan

**Objective:** Integrate T5 and CLIP text/image encoders from original stable-diffusion.cpp to enable complete video generation

---

## Context

Phase 5 integrates the T5 text encoder and CLIP image encoder from the original `stable-diffusion.cpp` project. These encoders are required for complete T2V (text-to-video) and I2V (image-to-video) generation functionality.

### Current State

- **Phase 4 (Examples)**: Complete - CLI with full argument parsing
- **Phase 3 (Public API)**: Complete - API framework with stub implementations
- **T2V/I2V Generation**: Currently returns `WAN_ERROR_UNSUPPORTED_OPERATION`

### Source Analysis

From `/home/jtzhuang/projects/stable-diffusion.cpp/src/`:

| Component | Main File | Vocab File | Size |
|-----------|-----------|-------------|------|
| T5 Encoder | t5.hpp (~1037 lines) | vocab/umt5.hpp (~29 MB) | ~30 MB |
| CLIP Encoder | clip.hpp (~1023 lines) | vocab/clip_t5.hpp (~29 MB) | ~30 MB |

### Dependencies

**T5 Encoder requires:**
- `darts.h` - DoubleArray Trie structure
- `json.hpp` - JSON parsing (nlohmann/json)
- `vocab/vocab.h`, `vocab/vocab.cpp` - Vocabulary management
- `vocab/umt5.hpp` - Unigram tokenizer (~56 MB)
- `model.h`, `ggml_extend.hpp` - GGML integration

**CLIP Encoder requires:**
- `tokenize_util.h` - Tokenization utilities
- `vocab/vocab.h`, `vocab/vocab.cpp` - Vocabulary management
- `vocab/clip_t5.hpp` - CLIP-T5 vocabulary (~29 MB)
- `model.h`, `ggml_extend.hpp` - GGML integration

---

## Implementation Plan

### Task 1: Copy Vocabulary Infrastructure

**Description:** Copy required vocabulary and supporting infrastructure files

**Files to copy:**
1. `vocab/umt5.hpp` - T5 unigram tokenizer
2. `vocab/clip_t5.hpp` - CLIP-T5 vocabulary
3. `vocab/vocab.h` - Vocabulary base header
4. `vocab/vocab.cpp` - Vocabulary implementation
5. `json.hpp` - JSON parsing (nlohmann/json)

**Action:**
- Create `src/vocab/` directory
- Copy files from original project
- Update include paths for wan-cpp structure

**Acceptance:** All vocabulary files compile without errors

---

### Task 2: Copy Supporting Infrastructure

**Description:** Copy darts.h and tokenize utilities

**Files to copy:**
1. `darts.h` - DoubleArray Trie data structure
2. `tokenize_util.h` - Tokenization utilities

**Action:**
- Copy to `src/` directory
- Verify include paths work correctly

**Acceptance:** Supporting files compile without errors

---

### Task 3: Copy T5 Encoder

**Description:** Copy complete T5 text encoder implementation

**Files to copy:**
1. `t5.hpp` - Main T5 encoder header

**Action:**
- Copy to `src/` directory
- Verify include paths resolve correctly

**Acceptance:** T5 encoder compiles

---

### Task 4: Copy CLIP Encoder

**Description:** Copy complete CLIP image encoder implementation

**Files to copy:**
1. `clip.hpp` - Main CLIP encoder header

**Action:**
- Copy to `src/` directory
- Verify include paths resolve correctly

**Acceptance:** CLIP encoder compiles

---

### Task 5: Update CMakeLists.txt

**Description:** Add new source files to build system

**Files to modify:**
1. `CMakeLists.txt` - Add vocab/, darts.h, tokenize_util.h, t5.hpp, clip.hpp

**Action:**
- Add vocab/ subdirectory
- Add new headers and sources
- Ensure proper include paths
- Test compilation

**Acceptance:** `cmake -B build .` succeeds

---

### Task 6: Update wan-internal.hpp

**Description:** Add encoder types to internal API structure

**Files to modify:**
1. `include/wan-cpp/wan-internal.hpp` - Add WanT5, WanCLIP types

**Action:**
- Add forward declarations for encoder classes
- Add smart pointer types (WanT5Ptr, WanCLIPPtr)
- Add encoder structures to WanContext

**Acceptance:** API header compiles

---

### Task 7: Integrate T5 into T2V Generation

**Description:** Connect T5 encoder to T2V generation pipeline

**Files to modify:**
1. `src/api/wan_t2v.cpp` - Add T5 encoder usage
2. `src/api/wan_loader.cpp` - Load T5 encoder from model

**Action:**
- Update T2V generation to use T5 for text encoding
- Add T5 encoder loading to model loader
- Test with minimal T2V generation

**Acceptance:** T2V generation can encode text prompts

---

### Task 8: Integrate CLIP into I2V Generation

**Description:** Connect CLIP encoder to I2V generation pipeline

**Files to modify:**
1. `src/api/wan_i2v.cpp` - Add CLIP encoder usage
2. `src/api/wan_loader.cpp` - Load CLIP encoder from model

**Action:**
- Update I2V generation to use CLIP for image encoding
- Add CLIP encoder loading to model loader
- Test with minimal I2V generation

**Acceptance:** I2V generation can encode input images

---

## Verification Checklist

After completing all tasks:

- [ ] All vocabulary files copy and compile
- [ ] T5 encoder compiles
- [ ] CLIP encoder compiles
- [ ] CMake configuration succeeds
- [ ] Library builds without errors
- [ ] T2V generation can use T5 encoder
- [ ] I2V generation can use CLIP encoder
- [ ] CLI example can run with functional encoders

---

## Notes

### Implementation Complexity

This is a **large integration task** involving:
- ~60+ MB of vocabulary data files
- ~2000+ lines of encoder code
- Complex dependency relationships
- Build system updates

### Memory Considerations

The vocabulary files are large (29 MB each). Consider:
- Using memory-mapped file I/O
- Optional quantization for reduced memory footprint
- Lazy loading of vocabulary data

### Testing Strategy

1. **Unit Tests:** Test encoder functionality independently
2. **Integration Tests:** Test T2V/I2V with encoders
3. **CLI Tests:** Use wan-cli to verify end-to-end functionality

---

*Plan created: 2026-03-15*
