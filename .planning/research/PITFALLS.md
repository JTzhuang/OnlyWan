# Pitfalls Research

**Domain:** C++ monorepo code extraction (WAN video generation library)
**Researched:** 2026-03-12
**Confidence:** MEDIUM

## Critical Pitfalls

### Pitfall 1: Hardcoded Relative Include Paths

**What goes wrong:**
Extracted headers still reference other headers with relative paths that no longer exist in the new project structure. This causes compilation failures when the include path hierarchy changes between the monorepo and extracted library.

**Why it happens:**
Headers use `#include "common_block.hpp"` assuming they're in the same directory. When extracting, if these files move to different directories or the include search path changes, the resolution fails. Developers often copy headers without updating include statements.

**How to avoid:**
1. Create a unified include directory structure that mirrors the original layout
2. Configure CMake include directories with `${CMAKE_CURRENT_SOURCE_DIR}` or absolute paths
3. Use `target_include_directories(wan-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})` to include the source root
4. Avoid mixing relative and absolute include paths in the same project

**Warning signs:**
- Error: "common_block.hpp: No such file or directory"
- Successful includes only when compiled from specific directory
- Adding `..` to include paths to find headers

**Phase to address:**
Phase 1 (Code extraction) - Set up include paths before copying files

---

### Pitfall 2: Submodule Detached HEAD State

**What goes wrong:**
GGML submodule ends up in detached HEAD state, causing the library to build with an undefined or unexpected GGML version. Different developers get different builds, and CI builds may be inconsistent.

**Why it happens:**
When initializing submodules without specifying a branch or commit, git creates a detached HEAD. When the parent repository updates the submodule reference, local working directories may not track this change automatically.

**How to avoid:**
1. Use `git submodule update --init --recursive` in the clone/setup script
2. Pin GGML to a specific commit hash in `.gitmodules` (not just a branch)
3. Add CI verification that checks `git -C ggml rev-parse HEAD` against expected hash
4. Document the required GGML version in README

**Warning signs:**
- `git status` shows "detached HEAD" for ggml directory
- Different build results between fresh clone and existing checkout
- `git submodule status` shows `-` prefix indicating detached state

**Phase to address:**
Phase 1 (Code extraction) - Initial submodule setup must be verified

---

### Pitfall 3: Transitive Header Dependencies

**What goes wrong:**
wan.hpp compiles when included directly, but when downstream projects include only wan.hpp, they get "missing file" errors for dependencies like flux.hpp, rope.hpp, vae.hpp. These headers are not included by wan.hpp but used by template instantiations or inline functions.

**Why it happens:**
C++ headers with inline functions or templates may not directly include all their dependencies. The original monorepo included all headers together via unified compilation units. When extracted as a library, the transitive dependencies break.

**How to avoid:**
1. Run `gcc -MM -MD` or equivalent to discover all header dependencies
2. Add all directly included headers to the library target, even if they seem unused
3. Use `target_sources(wan-lib PRIVATE src/*.hpp)` to include all headers as library sources
4. Consider creating an umbrella header that includes all dependencies explicitly

**Warning signs:**
- Header compiles in isolation but fails when used in another project
- Template instantiation errors that work in monorepo
- Need to add `#include` statements in downstream code for "private" headers

**Phase to address:**
Phase 2 (Build system setup) - Verify header completeness during CMake configuration

---

### Pitfall 4: CMake Target Include Directory Scoping

**What goes wrong:**
Include directories marked PRIVATE in the parent library become unavailable to downstream projects. Users get "fatal error: ggml.h: No such file or directory" when trying to include the extracted library.

**Why it happens:**
CMake's `target_include_directories(wan-lib PRIVATE path)` only exposes paths to wan-lib's own sources. Downstream projects linking against wan-lib cannot access those directories. The parent monorepo built with a unified build where all targets shared the same scope.

**How to avoid:**
1. Mark include directories as PUBLIC for headers that downstream projects need: `target_include_directories(wan-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)`
2. Mark GGML includes as PUBLIC: `target_include_directories(wan-lib PUBLIC ${GGML_INCLUDE_DIRS})`
3. Keep implementation-only includes PRIVATE to avoid exposing internal structure
4. Document which headers are "public API" vs "internal implementation"

**Warning signs:**
- Downstream project cannot find library headers even after `find_package(wan)`
- Only works when sources are directly included in the same project
- Need to manually add include paths when using the library

**Phase to address:**
Phase 2 (Build system setup) - Test library consumption with external project

---

### Pitfall 5: Header-Only Template Build Issues

**What goes wrong:**
Template code in header files fails to compile when the extracted library is built as shared/dynamic. Template instantiation happens in the library build, but the symbols are not exported properly.

**Why it happens:**
C++ templates require explicit instantiation or must be in headers. When the original code was all-in-one, this wasn't visible. Extracting as a library exposes template instantiation issues, especially with `extern template` patterns.

**How to avoid:**
1. Keep all template-heavy headers in the public include directory
2. Avoid mixing `.inl` files with complex template dependencies
3. If using `extern template`, ensure both declaration and instantiation are accessible
4. Test with both static and shared library builds

**Warning signs:**
- Undefined reference errors for template functions
- Linker errors that disappear when building as static library
- Build works in Debug but fails in Release due to optimization

**Phase to address:**
Phase 2 (Build system setup) - Enable both static and shared builds for testing

---

### Pitfall 6: Preprocessor Macro Name Collisions

**What goes wrong:**
When the WAN library is linked alongside the original stable-diffusion.cpp, preprocessor macros defined in both projects collide. This causes unexpected behavior or compilation errors due to macro redefinition.

**Why it happens:**
The original codebase uses common macro names like `CACHE_T`, `WAN_GRAPH_SIZE` without prefixing. When extracted and used alongside the parent project, these define the same symbols with potentially different values.

**How to avoid:**
1. Rename all macros in extracted code with a WAN-specific prefix: `WAN_CACHE_T`, `WAN_GRAPH_SIZE`
2. Add a configuration guard header that sets default values
3. Use `constexpr` or inline constants instead of macros where possible
4. Check for macro name collisions in namespace headers

**Warning signs:**
- Macro redefinition warnings during compilation
- Constants have unexpected values when using both libraries
- Behavior changes based on include order

**Phase to address:**
Phase 1 (Code extraction) - Audit all `#define` statements and rename

---

### Pitfall 7: GGML Backend Configuration Mismatch

**What goes wrong:**
The extracted WAN library fails to build with a specific GGML backend (CUDA, Metal, Vulkan) because the original CMakeLists.txt defines backend flags that aren't properly propagated to the new build system.

**Why it happens:**
GGML backend support requires both GGML compiled with the backend AND the wrapper code compiled with matching defines. The extraction may copy the flags but not ensure GGML submodule is built with the same configuration.

**How to avoid:**
1. Make GGML backend options explicit in WAN's CMakeLists.txt
2. Propagate backend flags to GGML via variables: `set(GGML_CUDA ${WAN_CUDA})`
3. Test build with each backend independently
4. Document which backends are actually tested/supported

**Warning signs:**
- Backend-specific code not compiled even when backend is enabled
- GGML headers don't include backend-specific definitions
- Backend functionality exists but fails at runtime

**Phase to address:**
Phase 2 (Build system setup) - Add backend-specific build tests

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Using `#include "../other_dir/header.hpp"` | Quick to fix missing includes | Fragile to directory moves, hard to maintain | Only during initial extraction, must be fixed before release |
| Copying GGML headers into include directory | Eliminates submodule complexity | Cannot upstream changes, version drift | Never - defeats purpose of using submodule |
| Using GLOB for sources in CMake | No need to list every file | Builds fail silently when files added/removed | Acceptable for internal dev, not for library |
| Adding `..` to CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES | Quick fix for missing paths | Breaks downstream projects, non-portable | Never |
| Skipping namespace migration | No code changes required | Symbol collisions when used alongside parent | Acceptable only if library is never used alongside monorepo |
| Hardcoding GGML include path | Simplest include configuration | Breaks if GGML installed in different location | Acceptable for submodules, but document requirement |

## Integration Gotchas

Common mistakes when connecting to external services.

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| GGML submodule | Treating it as vendored code, copying files | Keep as submodule, use `add_subdirectory(ggml)` |
| CMake find_package | Using `include_directories()` instead of `target_include_directories()` | Use modern CMake target-based configuration |
| Downstream projects | Including implementation headers directly | Provide well-defined public headers, document API |
| CI/CD | Assuming submodule initializes automatically | Add `git submodule update --init --recursive` to CI scripts |
| Multiple backends | Trying to build all backends in one binary | Build separate binaries per backend, use conditional compilation |

## Performance Traps

Patterns that work at small scale but fail as usage grows.

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Inline-heavy template code | Slow compilation times, large binary | Move non-critical code to .cpp files | At 100+ template instantiations |
| Header-only GGML wrapper | Every file that includes wan.hpp recompiles GGML types | Use PCH or move heavy includes to .cpp | At 10+ translation units |
| Include bloat | All GGML types visible to downstream users | Use forward declarations, hide implementation headers | When library API grows large |
| Static linking duplication | Multiple copies of GGML code in process | Use shared GGML library | When multiple WAN instances in same process |

## Security Mistakes

Domain-specific security issues beyond general web security.

| Mistake | Risk | Prevention |
|---------|------|------------|
| Loading GGUF files without validation | Malicious model files can cause crashes | Validate file structure before loading |
| Not checking GGML buffer sizes | Buffer overflows in custom ops | Assert all tensor dimensions are reasonable |
| Trusting GGUF metadata strings | Arbitrary code execution via deserialization | Don't execute code from model metadata |
| Missing bounds checks on user inputs | Memory corruption from malformed parameters | Validate all user-provided dimensions and parameters |

## UX Pitfalls

Common user experience mistakes in this domain.

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| No clear installation instructions | Users cannot get started | Provide step-by-step build guide for each platform |
| Backend selection unclear | Users waste time building wrong variant | Document backends clearly, detect available hardware |
| Undocumented GGML version requirement | Builds fail with cryptic errors | Specify exact GGML commit, provide version check |
| No example programs | Users don't know how to use API | Include working examples for common use cases |
| Include paths not exposed | Integration is painful | Export all necessary includes via CMake targets |

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces.

- [ ] **Header completeness:** Headers compile in isolation - verify downstream project can use library without internal headers
- [ ] **Submodule stability:** GGML builds correctly - verify fresh clone builds without manual intervention
- [ ] **Cross-platform builds:** Linux builds work - verify Windows and macOS builds with all backends
- [ ] **Backend functionality:** Code compiles with CUDA - verify CUDA backend actually runs and produces correct output
- [ ] **API completeness:** Library compiles - verify all documented functions are actually exported (nm/dumpbin check)
- [ ] **Dependency isolation:** Works in standalone build - verify no hidden dependencies on monorepo structure
- [ ] **Version consistency:** Current build works - verify pinned GGML commit doesn't change unexpectedly
- [ ] **Namespace isolation:** No obvious conflicts - verify macros and namespaces don't collide with parent project

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Hardcoded relative includes | LOW | Create `src/include` symlink or reorganize directory structure |
| Submodule detached HEAD | LOW | Run `git submodule update --init --recursive --force` |
| Missing transitive headers | MEDIUM | Use dependency scanner to find all headers, add to library sources |
| Include directory scoping | MEDIUM | Change PRIVATE to PUBLIC for public headers, rebuild |
| Template instantiation issues | HIGH | Move templates to headers, add explicit instantiations, test extensively |
| Macro name collisions | MEDIUM | Add `#undef` before `#define` or rename with prefixes, test with both libraries |
| Backend configuration mismatch | HIGH | Rebuild GGML with correct flags, verify CMake propagation, test each backend |

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls.

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Hardcoded relative includes | Phase 1 (Code extraction) | Build succeeds from any working directory |
| Submodule detached HEAD | Phase 1 (Code extraction) | CI verifies HEAD is attached to tracked commit |
| Transitive header dependencies | Phase 2 (Build system setup) | Downstream test project builds successfully |
| CMake include directory scoping | Phase 2 (Build system setup) | External find_package() works correctly |
| Header-only template build issues | Phase 2 (Build system setup) | Both static and shared builds succeed |
| Preprocessor macro name collisions | Phase 1 (Code extraction) | No macro redefinition warnings with parent project |
| GGML backend configuration mismatch | Phase 2 (Build system setup) | Each backend builds and runs correctly |
| Include bloat | Phase 3 (Optimization) | Check binary size, measure compile time |
| Namespace isolation | Phase 1 (Code extraction) | Link alongside stable-diffusion.cpp without conflicts |

## Sources

- Web searches on C++ code extraction pitfalls (MEDIUM confidence)
- Analysis of stable-diffusion.cpp source code structure (HIGH confidence)
- Observations from wan.hpp and related header dependencies (HIGH confidence)
- CMake include directory best practices (MEDIUM confidence)
- Git submodule management common issues (MEDIUM confidence)

---
*Pitfalls research for: C++ monorepo code extraction (WAN video generation library)*
*Researched: 2026-03-12*
