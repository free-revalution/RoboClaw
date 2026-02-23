#!/bin/bash
#
# RoboClaw One-Command Installation Script
# Supports macOS and Linux
#
# Usage:
#   curl -sSL https://raw.githubusercontent.com/free-revalution/RoboClaw/main/install.sh | bash
#
# Or with options:
#   ./install.sh [--prefix=~/.local] [--yes]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
PREFIX="${HOME}/.local"
INSTALL_DIR="${PREFIX}/bin"
YES=false
SKIP_DEPS=false
SKIP_BUILD=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --prefix=*)
            PREFIX="${1#*=}"
            INSTALL_DIR="${PREFIX}/bin"
            shift
            ;;
        --yes|-y)
            YES=true
            shift
            ;;
        --skip-deps)
            SKIP_DEPS=true
            shift
            ;;
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [--prefix=PATH] [--yes] [--skip-deps] [--skip-build]"
            echo ""
            echo "Options:"
            echo "  --prefix=PATH    Installation prefix (default: ~/.local)"
            echo "  --yes, -y        Auto-confirm all prompts"
            echo "  --skip-deps      Skip dependency installation"
            echo "  --skip-build     Skip building (assume already built)"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Platform detection
OS_TYPE="$(uname -s)"
ARCH_TYPE="$(uname -m)"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   RoboClaw Installation Script${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Platform: ${OS_TYPE} ${ARCH_TYPE}"
echo "Installation directory: ${INSTALL_DIR}"
echo ""

# Function to detect package manager
detect_package_manager() {
    if command -v brew &> /dev/null; then
        echo "homebrew"
    elif command -v apt-get &> /dev/null; then
        echo "apt"
    elif command -v yum &> /dev/null; then
        echo "yum"
    elif command -v pacman &> /dev/null; then
        echo "pacman"
    else
        echo "unknown"
    fi
}

# Function to install dependencies
install_dependencies() {
    echo -e "${GREEN}Installing dependencies...${NC}"

    PKG_MANAGER=$(detect_package_manager)

    case "${OS_TYPE}" in
        Darwin)
            # macOS
            if [ "${PKG_MANAGER}" = "homebrew" ]; then
                if [ "$YES" = false ]; then
                    read -p "Install dependencies via Homebrew? [y/N] " -n 1 -r
                    echo
                    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                        echo -e "${YELLOW}Skipping dependency installation.${NC}"
                        return 0
                    fi
                fi

                brew install cmake ninja nlohmann-json
            else
                echo -e "${RED}Homebrew not found. Please install from https://brew.sh${NC}"
                exit 1
            fi
            ;;

        Linux)
            # Linux
            if [ "${PKG_MANAGER}" = "apt" ]; then
                if [ "$YES" = false ]; then
                    read -p "Install dependencies via apt? [y/N] " -n 1 -r
                    echo
                    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                        echo -e "${YELLOW}Skipping dependency installation.${NC}"
                        return 0
                    fi
                fi

                sudo apt-get update
                sudo apt-get install -y cmake ninja-build nlohmann-json3-dev \
                    build-essential g++ git
            elif [ "${PKG_MANAGER}" = "yum" ]; then
                sudo yum install -y cmake ninja-build nlohmann-json-devel \
                    gcc-c++ git
            elif [ "${PKG_MANAGER}" = "pacman" ]; then
                sudo pacman -S cmake ninja nlohmann-json gcc git
            else
                echo -e "${YELLOW}Unknown package manager. Please install manually:${NC}"
                echo "  - CMake 3.20+"
                echo "  - Ninja"
                echo "  - nlohmann-json 3.11+"
                echo "  - C++20 compiler (GCC 10+ or Clang 12+)"
            fi
            ;;

        *)
            echo -e "${RED}Unsupported platform: ${OS_TYPE}${NC}"
            exit 1
            ;;
    esac
}

# Function to build RoboClaw
build_roboclaw() {
    echo -e "${GREEN}Building RoboClaw...${NC}"

    # Create build directory
    mkdir -p build
    cd build

    # Configure
    echo "Configuring build..."
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_INSTALL_PREFIX="${PREFIX}"

    # Build
    echo "Compiling..."
    cmake --build . --config Release -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

    cd ..
}

# Function to install RoboClaw
install_roboclaw() {
    echo -e "${GREEN}Installing RoboClaw...${NC}"

    cd build
    cmake --install . --component roboclaw
    cd ..

    # Create symlink if not in standard path
    if [[ ":$PATH:" != *":${INSTALL_DIR}:"* ]]; then
        echo -e "${YELLOW}Adding ${INSTALL_DIR} to PATH...${NC}"

        # Detect shell config file
        SHELL_CONFIG=""
        if [ -n "${ZSH_VERSION}" ]; then
            SHELL_CONFIG="${HOME}/.zshrc"
        elif [ -n "${BASH_VERSION}" ]; then
            SHELL_CONFIG="${HOME}/.bashrc"
        fi

        if [ -n "${SHELL_CONFIG}" ]; then
            echo "" >> "${SHELL_CONFIG}"
            echo "# RoboClaw" >> "${SHELL_CONFIG}"
            echo "export PATH=\"${INSTALL_DIR}:\$PATH\"" >> "${SHELL_CONFIG}"
            echo -e "${GREEN}Added to PATH in ${SHELL_CONFIG}${NC}"
            echo -e "${YELLOW}Run 'source ${SHELL_CONFIG}' or restart your shell${NC}"
        fi
    fi
}

# Function to verify installation
verify_installation() {
    echo -e "${GREEN}Verifying installation...${NC}"

    if [ -x "${INSTALL_DIR}/roboclaw" ]; then
        echo -e "${GREEN}✓ RoboClaw installed successfully!${NC}"
        echo ""
        echo "Run 'roboclaw' to start the agent."
    else
        echo -e "${RED}✗ Installation verification failed${NC}"
        exit 1
    fi
}

# Main installation flow
main() {
    # Install dependencies
    if [ "$SKIP_DEPS" = false ]; then
        install_dependencies
    fi

    # Build
    if [ "$SKIP_BUILD" = false ]; then
        build_roboclaw
    fi

    # Install
    install_roboclaw

    # Verify
    verify_installation

    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}   Installation Complete!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "Documentation: https://github.com/free-revalution/RoboClaw"
    echo "Issues: https://github.com/free-revalution/RoboClaw/issues"
    echo ""
}

# Run main function
main "$@"
