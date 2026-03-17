cmake_minimum_required(VERSION 3.20)
project(wan-cpp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type if not specified
if (NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# MSVC specific settings
if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
endif()

# Set output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Check if we're standalone
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(WAN_STANDALONE ON)
else()
    set(WAN_STANDALONE OFF)
endif()

#
# Option list
#

# Backend options
option(WAN_CUDA     "wan-cpp: CUDA backend"      OFF)
option(WAN_METAL    "wan-cpp: Metal backend"     OFF)
option(WAN_VULKAN   "wan-cpp: Vulkan backend"    OFF)
option(WAN_OPENCL   "wan-cpp: OpenCL backend"    OFF)
option(WAN_SYCL     "wan-cpp: SYCL backend"      OFF)
option(WAN_HIPBLAS  "wan-cpp: ROCm backend"      OFF)
option(WAN_MUSA     "wan-cpp: MUSA backend"      OFF)

# General options
option(WAN_BUILD_SHARED_LIBS "wan-cpp: build shared libs" OFF)
option(WAN_BUILD_EXAMPLES     "wan-cpp: build examples"    ${WAN_STANDALONE})
option(WAN_EMBED_VOCAB "wan-cpp: embed vocabulary arrays in binary (increases build time ~2min, binary +85MB)" OFF)
option(WAN_CUDA_GRAPHS "wan-cpp: enable CUDA graph capture/replay (requires persistent compute buffer)" ON)

# Map wan-cpp options to GGML options
if(WAN_CUDA)
    message("-- Use CUDA as backend for wan-cpp")
    set(GGML_CUDA ON)
    add_definitions(-DWAN_USE_CUDA)
    if(WAN_CUDA_GRAPHS)
        add_definitions(-DGGML_CUDA_USE_GRAPHS)
        message("-- CUDA graph capture/replay enabled")
    endif()
endif()

if(WAN_METAL)
    message("-- Use Metal as backend for wan-cpp")
    set(GGML_METAL ON)
    add_definitions(-DWAN_USE_METAL)
endif()

if(WAN_VULKAN)
    message("-- Use Vulkan as backend for wan-cpp")
    set(GGML_VULKAN ON)
    add_definitions(-DWAN_USE_VULKAN)
endif()

if(WAN_OPENCL)
    message("-- Use OpenCL as backend for wan-cpp")
    set(GGML_OPENCL ON)
    add_definitions(-DWAN_USE_OPENCL)
endif()

if(WAN_SYCL)
    message("-- Use SYCL as backend for wan-cpp")
    set(GGML_SYCL ON)
    add_definitions(-DWAN_USE_SYCL)
endif()

if(WAN_HIPBLAS)
    message("-- Use HIPBLAS as backend for wan-cpp")
    set(GGML_HIP ON)
    add_definitions(-DWAN_USE_CUDA)
    if(WAN_CUDA_GRAPHS)
        add_definitions(-DGGML_CUDA_USE_GRAPHS)
        message("-- CUDA graph capture/replay enabled (HIPBLAS)")
    endif()
endif()

if(WAN_MUSA)
    message("-- Use MUSA as backend for wan-cpp")
    set(GGML_MUSA ON)
    add_definitions(-DWAN_USE_CUDA)
    if(WAN_CUDA_GRAPHS)
        add_definitions(-DGGML_CUDA_USE_GRAPHS)
        message("-- CUDA graph capture/replay enabled (MUSA)")
    endif()
endif()

# Define library name
set(WAN_LIB wan-cpp)

# Gather all source files
file(GLOB WAN_LIB_HEADERS
    "src/*.h"
    "src/*.hpp"
    "include/wan-cpp/*.h"
    "include/wan-cpp/*.hpp"
)

# Main library source files (explicit — avoids duplicate symbol pickup)
set(WAN_LIB_SOURCES
    src/util.cpp
    src/model.cpp
    src/name_conversion.cpp
    src/tokenize_util.cpp
    src/vocab/vocab.cpp
    src/api/wan-api.cpp
    src/api/wan_config.cpp
    src/api/wan_t2v.cpp
    src/api/wan_i2v.cpp
    src/api/wan_loader.cpp
)

# Get version from git if available
find_program(GIT_EXE NAMES git git.exe NO_CMAKE_FIND_ROOT_PATH)
if(GIT_EXE)
    execute_process(COMMAND ${GIT_EXE} describe --tags --abbrev=7 --dirty=+
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE WAN_BUILD_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(COMMAND ${GIT_EXE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE WAN_BUILD_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

if(NOT WAN_BUILD_VERSION)
    set(WAN_BUILD_VERSION unknown)
endif()
message(STATUS "wan-cpp version ${WAN_BUILD_VERSION}")

if(NOT WAN_BUILD_COMMIT)
    set(WANAN_BUILD_COMMIT unknown)
endif()
message(STATUS "wan-cpp commit ${WAN_BUILD_COMMIT}")

# Create library target
if(WAN_BUILD_SHARED_LIBS)
    message("-- Build shared library")
    set(BUILD_SHARED_LIBS ON)
    add_library(${WAN_LIB} SHARED ${WAN_LIB_HEADERS} ${WAN_LIB_SOURCES})
    add_definitions(-DWAN_BUILD_SHARED_LIB)
    target_compile_definitions(${WAN_LIB} PRIVATE -DWAN_BUILD_DLL)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    message("-- Build static library")
    set(BUILD_SHARED_LIBS OFF)
    add_library(${WAN_LIB} STATIC ${WAN_LIB_HEADERS} ${WAN_LIB_SOURCES})
endif()

# Vocab embedding — OFF by default to avoid 127MB compile overhead
if(WAN_EMBED_VOCAB)
    target_compile_definitions(${WAN_LIB} PRIVATE WAN_EMBED_VOCAB)
    message(STATUS "wan-cpp: vocab embedding ON (slow build)")
else()
    message(STATUS "wan-cpp: vocab embedding OFF (runtime mmap)")
endif()

# Configure library version properties
set_target_properties(${WAN_LIB} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Define max name constant for GGML
add_definitions(-DGGML_MAX_NAME=128)

# Add GGML as a subdirectory
if (NOT TARGET ggml)
    message(STATUS "Adding GGML from ${CMAKE_CURRENT_SOURCE_DIR}/ggml")
    add_subdirectory(ggml)
endif()

# Add thirdparty dependencies
add_subdirectory(thirdparty)

# Target configurations
target_link_libraries(${WAN_LIB} PUBLIC ggml zip)

# Include directories
target_include_directories(${WAN_LIB} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/wan-cpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/vocab>
)

# Set C++ standard
target_compile_features(${WAN_LIB} PUBLIC cxx_std_17)

# Public headers for install
set(WAN_PUBLIC_HEADERS include/wan-cpp/wan.h)
set_target_properties(${WAN_LIB} PROPERTIES PUBLIC_HEADER "${WAN_PUBLIC_HEADERS}")

# SYCL specific configuration
if(WAN_SYCL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing -fsycl")
    if(WIN32)
        set(SYCL_COMPILE_OPTIONS /fp:precise)
    else()
        set(SYCL_COMPILE_OPTIONS -fp-model=precise)
    endif()
    message("-- Turn off fast-math for host in SYCL backend")
    target_compile_options(${WAN_LIB} PRIVATE ${SYCL_COMPILE_OPTIONS})
endif()

# Build examples if requested
if(WAN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install configuration
install(TARGETS ${WAN_LIB} LIBRARY PUBLIC_HEADER)

# Print configuration summary
message(STATUS "")
message(STATUS "=== wan-cpp Configuration ===")
message(STATUS "Build type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard:       C++17")
message(STATUS "Build shared libs:  ${WAN_BUILD_SHARED_LIBS}")
message(STATUS "CUDA backend:       ${WAN_CUDA}")
message(STATUS "Metal backend:      ${WAN_METAL}")
message(STATUS "Vulkan backend:     ${WAN_VULKAN}")
message(STATUS "OpenCL backend:     ${WAN_OPENCL}")
message(STATUS "SYCL backend:       ${WAN_SYCL}")
message(STATUS "HIPBLAS backend:    ${WAN_HIPBLAS}")
message(STATUS "MUSA backend:       ${WAN_MUSA}")
message(STATUS "==============================")
message(STATUS "")
