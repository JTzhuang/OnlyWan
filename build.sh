#!/bin/bash

# WAN-CPP Build Script
# Supports multiple backends: CUDA, Metal, Vulkan, OpenCL, CPU
# Usage: ./build.sh [backend] [options]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BACKEND="cuda"
BUILD_TYPE="Release"
BUILD_DIR="build_${BACKEND}"
JOBS=$(nproc)
ENABLE_TESTS=0
ENABLE_EXAMPLES=1
VERBOSE=0

# Parse arguments
print_usage() {
    cat << EOF
${BLUE}WAN-CPP Build Script${NC}

Usage: ./build.sh [backend] [options]

Backends:
  cuda      - NVIDIA CUDA (default)
  metal     - Apple Metal
  vulkan    - Vulkan
  opencl    - OpenCL
  cpu       - CPU only

Options:
  -t, --type TYPE       Build type: Debug, Release (default: Release)
  -j, --jobs N          Number of parallel jobs (default: $(nproc))
  -d, --dir DIR         Build directory (default: build_\${backend})
  --tests               Build tests
  --no-examples         Skip building examples
  -v, --verbose         Verbose output
  -c, --clean           Clean build directory before building
  -h, --help            Show this help message

Examples:
  ./build.sh cuda                    # Build with CUDA backend
  ./build.sh metal -t Debug          # Build with Metal backend in Debug mode
  ./build.sh cuda --tests -j 8       # Build with tests using 8 jobs
  ./build.sh cpu --clean             # Clean build with CPU backend

EOF
}

# Parse command line arguments
CLEAN_BUILD=0

while [[ $# -gt 0 ]]; do
    case $1 in
        cuda|metal|vulkan|opencl|cpu)
            BACKEND="$1"
            BUILD_DIR="build_${BACKEND}"
            shift
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -d|--dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --tests)
            ENABLE_TESTS=1
            shift
            ;;
        --no-examples)
            ENABLE_EXAMPLES=0
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=1
            shift
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            print_usage
            exit 1
            ;;
    esac
done

# Print build configuration
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}WAN-CPP Build Configuration${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "Backend:        ${GREEN}${BACKEND}${NC}"
echo -e "Build Type:     ${GREEN}${BUILD_TYPE}${NC}"
echo -e "Build Dir:      ${GREEN}${BUILD_DIR}${NC}"
echo -e "Parallel Jobs:  ${GREEN}${JOBS}${NC}"
echo -e "Build Tests:    ${GREEN}$([ $ENABLE_TESTS -eq 1 ] && echo 'Yes' || echo 'No')${NC}"
echo -e "Build Examples: ${GREEN}$([ $ENABLE_EXAMPLES -eq 1 ] && echo 'Yes' || echo 'No')${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Clean build directory if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake based on backend
echo -e "${YELLOW}Configuring CMake...${NC}"

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"

case $BACKEND in
    cuda)
        CMAKE_ARGS="${CMAKE_ARGS} -DWAN_CUDA=ON -DWAN_CUDA_GRAPHS=ON"
        echo -e "${GREEN}✓ CUDA backend enabled${NC}"
        echo -e "${GREEN}✓ CUDA Graph optimization enabled${NC}"
        ;;
    metal)
        CMAKE_ARGS="${CMAKE_ARGS} -DWAN_METAL=ON"
        echo -e "${GREEN}✓ Metal backend enabled${NC}"
        ;;
    vulkan)
        CMAKE_ARGS="${CMAKE_ARGS} -DWAN_VULKAN=ON"
        echo -e "${GREEN}✓ Vulkan backend enabled${NC}"
        ;;
    opencl)
        CMAKE_ARGS="${CMAKE_ARGS} -DWAN_OPENCL=ON"
        echo -e "${GREEN}✓ OpenCL backend enabled${NC}"
        ;;
    cpu)
        echo -e "${GREEN}✓ CPU backend (default)${NC}"
        ;;
esac

# Add optional features
if [ $ENABLE_TESTS -eq 1 ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DBUILD_TESTS=ON"
    echo -e "${GREEN}✓ Tests enabled${NC}"
fi

if [ $ENABLE_EXAMPLES -eq 0 ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DBUILD_EXAMPLES=OFF"
    echo -e "${YELLOW}✓ Examples disabled${NC}"
fi

if [ $VERBOSE -eq 1 ]; then
    CMAKE_ARGS="${CMAKE_ARGS} --debug-output"
fi

# Run CMake
echo ""
echo -e "${YELLOW}Running CMake...${NC}"
if [ $VERBOSE -eq 1 ]; then
    cmake .. ${CMAKE_ARGS}
else
    cmake .. ${CMAKE_ARGS} > /dev/null 2>&1 || {
        echo -e "${RED}CMake configuration failed${NC}"
        exit 1
    }
fi

# Build
echo ""
echo -e "${YELLOW}Building...${NC}"
if [ $VERBOSE -eq 1 ]; then
    cmake --build . --config ${BUILD_TYPE} --parallel ${JOBS}
else
    cmake --build . --config ${BUILD_TYPE} --parallel ${JOBS} > /dev/null 2>&1 || {
        echo -e "${RED}Build failed${NC}"
        exit 1
    }
fi

# Print build results
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}✓ Build completed successfully!${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# List output binaries
echo -e "${BLUE}Output binaries:${NC}"
if [ -f "bin/wan-cli" ]; then
    SIZE=$(du -h "bin/wan-cli" | cut -f1)
    echo -e "  ${GREEN}✓${NC} bin/wan-cli (${SIZE})"
fi

if [ -f "bin/wan-convert" ]; then
    SIZE=$(du -h "bin/wan-convert" | cut -f1)
    echo -e "  ${GREEN}✓${NC} bin/wan-convert (${SIZE})"
fi

if [ -f "lib/libwan-cpp.a" ]; then
    SIZE=$(du -h "lib/libwan-cpp.a" | cut -f1)
    echo -e "  ${GREEN}✓${NC} lib/libwan-cpp.a (${SIZE})"
fi

echo ""
echo -e "${BLUE}Build directory: ${GREEN}${BUILD_DIR}${NC}"
echo ""

# Print next steps
echo -e "${BLUE}Next steps:${NC}"
echo -e "  Run CLI:     ${GREEN}cd ${BUILD_DIR} && ./bin/wan-cli --help${NC}"
echo -e "  Run tests:   ${GREEN}cd ${BUILD_DIR} && ctest${NC}"
echo -e "  Install:     ${GREEN}cd ${BUILD_DIR} && cmake --install .${NC}"
echo ""
