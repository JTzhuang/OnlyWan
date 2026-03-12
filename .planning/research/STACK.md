# Technology Stack

**Project:** wan-cpp
**Researched:** 2025-03-12
**Overall confidence:** HIGH

## Recommended Stack

### Core Framework
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| C++ | 17 | Language standard | Required by ggml and stable-diffusion.cpp; provides sufficient features for the project |
| CMake | 3.20+ | Build system | Modern target-based approach, better support for C++17/20 features, widely available on all platforms |
| ggml | 0.9.7+ | Tensor computation backend | Required dependency from parent project, provides cross-platform ML operations |

### Build System
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| Ninja | Latest | Build generator | Faster than Makefiles, better parallel builds, recommended for CMake projects |
| ccache | 3.7+ | Build caching | Significantly speeds up rebuilds during development, especially useful for large C++ projects |

### Development Tools
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| clang-format | 10+ | Code formatting | Industry standard for C++ code formatting, ensures consistency |
| clang-tidy | 10+ | Static analysis | Catches common errors, enforces modern C++ best practices |
| git | 2.30+ | Version control | Required for submodule management |

### Supporting Libraries
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| nloh (json.hpp) | 3.11+ | JSON parsing | Required for config/model files; single-header, zero-build dependency |
| stb_image | Latest | Image I/O | Required for image preprocessing in video generation |
| miniz | Latest | ZIP compression | Used for weight loading in parent project |
| httplib | Latest | HTTP server | Optional for server features |

### Testing Framework
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Catch2 | 3.4+ | Unit testing | Recommended for modern C++ testing, header-only option available, excellent integration with CMake |

## Installation

### Core Dependencies
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build ccache git

# macOS
brew install cmake ninja ccache git

# Windows (via vcpkg)
vcpkg install cmake ninja ccache
```

### Building with CMake
```bash
# Standard build
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel

# With backend (example: CUDA)
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DWAN_CUDA=ON ..
cmake --build . --parallel

# With all warnings enabled
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DWAN_ALL_WARNINGS=ON ..
cmake --build . --parallel
```

## CMake Best Practices for Submodules

### Modern Target-Based Approach
```cmake
# Create library with proper visibility
add_library(wan STATIC
    src/wan.cpp
    src/preprocessing.cpp
    src/util.cpp
)

# Use target-specific properties instead of global variables
target_include_directories(wan
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies with proper visibility
target_link_libraries(wan
    PUBLIC
        ggml
    PRIVATE
        zip
)

# Set C++ standard on target
target_compile_features(wan PUBLIC cxx_std_17)

# Export for downstream consumption
install(TARGETS wan
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
```

### Git Submodule Integration
```cmake
# Check for system ggml first, fallback to submodule
find_package(ggml QUIET)

if (NOT ggml_FOUND)
    # Use ggml as submodule
    add_subdirectory(ggml EXCLUDE_FROM_ALL)
else()
    # Use system-installed ggml
    add_library(ggml ALIAS ggml::ggml)
endif()

# Always link with proper target name
target_link_libraries(wan PUBLIC ggml)
```

## Cross-Platform Considerations

### Platform-Specific Options
```cmake
# Windows
if (MSVC)
    target_compile_definitions(wan PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(wan PRIVATE /utf-8)
endif()

# macOS
if (APPLE)
    # Metal is default on Apple Silicon
    option(WAN_METAL "Enable Metal backend" ON)
    if (WAN_METAL)
        target_compile_definitions(wan PRIVATE WAN_USE_METAL)
        set(GGML_METAL ON)
    endif()
endif()

# Linux
if (UNIX AND NOT APPLE)
    option(WAN_VULKAN "Enable Vulkan backend" OFF)
endif()
```

### Compiler Feature Detection
```cmake
# Use CMake's built-in feature detection instead of version checks
target_compile_features(wan
    PUBLIC
        cxx_std_17
        c_std_11
)

# Compiler-specific optimizations
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(wan PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -fPIC
    )
elseif (MSVC)
    target_compile_options(wan PRIVATE
        /W4
        /utf-8
    )
endif()
```

## Project Structure

```
wan-cpp/
├── CMakeLists.txt           # Main CMake file
├── include/                 # Public headers
│   └── wan.h              # Public API
├── src/                    # Source files
│   ├── wan.cpp
│   ├── preprocessing.cpp
│   └── util.cpp
├── tests/                  # Unit tests
│   ├── CMakeLists.txt
│   └── test_wan.cpp
├── examples/               # Example programs
│   ├── CMakeLists.txt
│   └── cli_example.cpp
├── thirdparty/            # Single-header deps
│   ├── json.hpp
│   ├── stb_image.h
│   └── miniz.h
├── ggml/                   # Git submodule
├── .clang-format          # Code formatting config
├── .clang-tidy            # Static analysis config
├── format-code.sh         # Formatting script
└── README.md
```

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| Build System | CMake | Meson | CMake is more widely supported, better tooling integration, ggml uses CMake |
| Build Generator | Ninja | Make | Ninja is significantly faster for large projects, better parallelization |
| C++ Standard | C++17 | C++20 | C++20 adoption is not universal, C++17 has all needed features |
| Testing | Catch2 | Google Test | Catch2 is header-only (optional), simpler syntax, better modern C++ support |
| Package Manager | git submodule | vcpkg/conan | Submodules are simpler for this use case, matches parent project approach |
| JSON Library | nlohmann | rapidjson | nlohmann has modern C++ API, single-header, intuitive interface |

## Anti-Patterns to Avoid

### Don't Use Global CMake Variables
**Bad:**
```cmake
include_directories(${PROJECT_SOURCE_DIR}/include)
add_definitions(-DDEBUG)
```

**Good:**
```cmake
target_include_directories(wan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(wan PRIVATE DEBUG)
```

### Don't Mix Global and Target Commands
**Bad:**
```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
target_link_libraries(wan PUBLIC pthread)
```

**Good:**
```cmake
target_compile_options(wan PRIVATE -Wall)
target_link_libraries(wan PUBLIC Threads::Threads)
```

### Don't Hardcode Library Paths
**Bad:**
```cmake
target_include_directories(wan PRIVATE /usr/local/include/ggml)
```

**Good:**
```cmake
find_package(ggml REQUIRED)
target_link_libraries(wan PUBLIC ggml::ggml)
```

### Don't Use Legacy CMake Features
**Bad:**
```cmake
# Legacy commands
include_directories()
link_directories()
add_subdirectory() with old policies
```

**Good:**
```cmake
# Modern CMake 3.14+
target_include_directories()
target_link_directories()
add_subdirectory(EXCLUDE_FROM_ALL)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
```

## Static Analysis Integration

### clang-tidy Configuration
```yaml
# .clang-tidy
Checks: >
  modernize-make-shared,
  modernize-use-nullptr,
  modernize-use-override,
  modernize-pass-by-value,
  modernize-return-braced-init-list,
  modernize-deprecated-headers,
  performance-*,,
  bugprone-*,
  cppcoreguidelines-*,
WarningsAsErrors: ''
HeaderFilterRegex: '^src/.*'
FormatStyle: file
```

### Integration with CMake
```cmake
find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if (CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    # Enable only when WAN_ENABLE_CLANG_TIDY is ON for CI
    if (WAN_ENABLE_CLANG_TIDY)
        message(STATUS "clang-tidy enabled")
    endif()
endif()
```

### clang-format Configuration
```yaml
# .clang-format
BasedOnStyle: Chromium
UseTab: Never
IndentWidth: 4
ColumnLimit: 120
```

## CI/CD Recommendations

### GitHub Actions Matrix Strategy
```yaml
strategy:
  matrix:
    include:
      - os: ubuntu-latest
        backend: cpu
      - os: ubuntu-latest
        backend: vulkan
      - os: macos-latest
        backend: metal
      - os: windows-latest
        backend: cpu
```

### Cache Configuration
```yaml
- uses: actions/cache@v4
  with:
    path: |
      ~/.ccache
      build
    key: ${{ runner.os }}-${{ matrix.backend }}-${{ hashFiles('**/CMakeLists.txt') }}
```

## Sources

- [CMake 3.20+ Documentation](https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html) - HIGH confidence
- [Modern CMake Documentation](https://cliutils.gitlab.io/modern-cmake/) - MEDIUM confidence
- [ggml Project](https://github.com/ggerganov/ggml) - HIGH confidence (analyzed actual codebase)
- [stable-diffusion.cpp Project](https://github.com/username/stable-diffusion.cpp) - HIGH confidence (analyzed actual codebase)
- [Catch2 v3 Documentation](https://github.com/catchorg/Catch2/tree/devel/docs) - MEDIUM confidence
