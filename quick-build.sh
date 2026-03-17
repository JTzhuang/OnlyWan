#!/bin/bash

# WAN-CPP Quick Build Script
# Fast build with common configurations

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_SCRIPT="${SCRIPT_DIR}/build.sh"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_menu() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}WAN-CPP Quick Build${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
    echo "Select build configuration:"
    echo ""
    echo -e "  ${GREEN}1${NC}) CUDA Release (optimized, default)"
    echo -e "  ${GREEN}2${NC}) CUDA Debug (with debug symbols)"
    echo -e "  ${GREEN}3${NC}) Metal Release (Apple Silicon)"
    echo -e "  ${GREEN}4${NC}) CPU Release (no GPU)"
    echo -e "  ${GREEN}5${NC}) CUDA Release + Tests"
    echo -e "  ${GREEN}6${NC}) Clean rebuild (CUDA Release)"
    echo -e "  ${GREEN}7${NC}) Custom configuration"
    echo -e "  ${GREEN}0${NC}) Exit"
    echo ""
}

while true; do
    print_menu
    read -p "Enter choice [0-7]: " choice

    case $choice in
        1)
            echo -e "${YELLOW}Building CUDA Release...${NC}"
            bash "$BUILD_SCRIPT" cuda -t Release -j $(nproc)
            break
            ;;
        2)
            echo -e "${YELLOW}Building CUDA Debug...${NC}"
            bash "$BUILD_SCRIPT" cuda -t Debug -j $(nproc)
            break
            ;;
        3)
            echo -e "${YELLOW}Building Metal Release...${NC}"
            bash "$BUILD_SCRIPT" metal -t Release -j $(nproc)
            break
            ;;
        4)
            echo -e "${YELLOW}Building CPU Release...${NC}"
            bash "$BUILD_SCRIPT" cpu -t Release -j $(nproc)
            break
            ;;
        5)
            echo -e "${YELLOW}Building CUDA Release with Tests...${NC}"
            bash "$BUILD_SCRIPT" cuda -t Release -j $(nproc) --tests
            break
            ;;
        6)
            echo -e "${YELLOW}Clean rebuild CUDA Release...${NC}"
            bash "$BUILD_SCRIPT" cuda -t Release -j $(nproc) --clean
            break
            ;;
        7)
            echo -e "${YELLOW}Custom configuration${NC}"
            read -p "Backend (cuda/metal/vulkan/opencl/cpu) [cuda]: " backend
            backend=${backend:-cuda}
            read -p "Build type (Debug/Release) [Release]: " build_type
            build_type=${build_type:-Release}
            read -p "Number of jobs [$(nproc)]: " jobs
            jobs=${jobs:-$(nproc)}
            read -p "Build tests? (y/n) [n]: " tests

            cmd="bash \"$BUILD_SCRIPT\" $backend -t $build_type -j $jobs"
            [ "$tests" = "y" ] && cmd="$cmd --tests"

            eval "$cmd"
            break
            ;;
        0)
            echo -e "${GREEN}Exiting...${NC}"
            exit 0
            ;;
        *)
            echo -e "${YELLOW}Invalid choice. Please try again.${NC}"
            ;;
    esac
done
