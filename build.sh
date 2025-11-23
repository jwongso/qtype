#!/bin/bash
# qtype build script

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default options
BUILD_TESTS=OFF
BUILD_TYPE=Release
ENABLE_SANITIZERS=OFF
CLEAN=false
RUN_TESTS=false
INSTALL=false
VERBOSE=false

# Function to print colored output
print_info() {
    echo -e "${BLUE}ℹ ${NC}$1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# Function to show usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build script for qtype typing automation tool

OPTIONS:
    -t, --tests          Build with unit tests
    -d, --debug          Build in Debug mode (default: Release)
    -s, --sanitizers     Enable AddressSanitizer and UBSan
    -c, --clean          Clean build directory before building
    -r, --run-tests      Run tests after building
    -i, --install        Install after building (may need sudo)
    -v, --verbose        Verbose build output
    -h, --help           Show this help message

EXAMPLES:
    $0                              # Basic release build
    $0 --tests --run-tests          # Build and run tests
    $0 --debug --sanitizers         # Debug build with sanitizers
    $0 --clean --tests --verbose    # Clean build with tests, verbose

EOF
    exit 0
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--tests)
            BUILD_TESTS=ON
            shift
            ;;
        -d|--debug)
            BUILD_TYPE=Debug
            shift
            ;;
        -s|--sanitizers)
            ENABLE_SANITIZERS=ON
            shift
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -r|--run-tests)
            RUN_TESTS=true
            shift
            ;;
        -i|--install)
            INSTALL=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            print_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Print build configuration
echo ""
print_info "qtype Build Configuration"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Build Type:    $BUILD_TYPE"
echo "Tests:         $BUILD_TESTS"
echo "Sanitizers:    $ENABLE_SANITIZERS"
echo "Clean Build:   $CLEAN"
echo "Run Tests:     $RUN_TESTS"
echo "Install:       $INSTALL"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check dependencies
print_info "Checking dependencies..."

if ! command -v cmake &> /dev/null; then
    print_error "cmake not found. Please install: sudo dnf install cmake"
    exit 1
fi

if ! command -v qmake6 &> /dev/null && ! command -v qmake-qt6 &> /dev/null; then
    print_error "Qt6 not found. Please install: sudo dnf install qt6-qtbase-devel"
    exit 1
fi

if [ "$BUILD_TESTS" = "ON" ]; then
    if ! ldconfig -p | grep -q libgtest; then
        print_warning "GTest not found. Install with: sudo dnf install gtest-devel"
        print_warning "Continuing without tests..."
        BUILD_TESTS=OFF
    fi
fi

print_success "Dependencies OK"

# Save original directory (do this early, before any cd commands)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Clean if requested
if [ "$CLEAN" = true ]; then
    print_info "Cleaning build directory..."
    if [ -d "${SCRIPT_DIR}/build" ]; then
        rm -rf "${SCRIPT_DIR}/build"
        print_success "Clean complete"
    else
        print_info "Build directory already clean"
    fi
fi

# Create build directory
print_info "Creating build directory..."
mkdir -p "${SCRIPT_DIR}/build"
cd "${SCRIPT_DIR}/build"

# If CMakeCache.txt exists and we're not cleaning, check for generator mismatch
if [ -f "CMakeCache.txt" ] && [ "$CLEAN" = false ]; then
    print_warning "Found existing CMake cache"
    print_info "Removing CMake cache to avoid generator conflicts..."
    rm -f CMakeCache.txt
    rm -rf CMakeFiles
fi

# Configure with CMake
print_info "Configuring with CMake..."

CMAKE_ARGS=(
    -G "Unix Makefiles"
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE
    -DBUILD_TESTS=$BUILD_TESTS
    -DENABLE_SANITIZERS=$ENABLE_SANITIZERS
)

if cmake "${CMAKE_ARGS[@]}" "${SCRIPT_DIR}" ; then
    print_success "Configuration complete"
else
    print_error "Configuration failed"
    exit 1
fi

# Build
print_info "Building..."

# Check which build system was generated
if [ -f "build.ninja" ]; then
    print_info "Using Ninja build system"
    BUILD_CMD="ninja"
    if ! command -v ninja &> /dev/null; then
        print_error "Ninja not found but build.ninja exists"
        print_error "Install with: sudo dnf install ninja-build"
        exit 1
    fi
elif [ -f "Makefile" ]; then
    print_info "Using Make build system"
    BUILD_CMD="make"
    
    # Detect number of cores for parallel build
    if command -v nproc &> /dev/null; then
        CORES=$(nproc)
    else
        CORES=2
    fi
    BUILD_CMD="make -j$CORES"
else
    print_error "No build system found (no Makefile or build.ninja)"
    print_error "Current directory: $(pwd)"
    print_error "Files: $(ls -la)"
    exit 1
fi

if [ "$VERBOSE" = true ]; then
    BUILD_CMD="$BUILD_CMD VERBOSE=1"
fi

if $BUILD_CMD; then
    print_success "Build complete"
else
    print_error "Build failed"
    exit 1
fi

# Run tests if requested
if [ "$RUN_TESTS" = true ] && [ "$BUILD_TESTS" = "ON" ]; then
    echo ""
    print_info "Running tests..."
    if ctest --output-on-failure; then
        print_success "All tests passed"
    else
        print_error "Some tests failed"
        exit 1
    fi
fi

# Install if requested
if [ "$INSTALL" = true ]; then
    echo ""
    print_info "Installing..."
    if sudo make install; then
        print_success "Installation complete"
    else
        print_error "Installation failed"
        exit 1
    fi
fi

# Print summary
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
print_success "Build Summary"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Executable:    $(pwd)/qtype"

if [ "$BUILD_TESTS" = "ON" ]; then
    echo "Tests:         $(pwd)/qtype_tests"
fi

echo ""
print_info "To run the application:"
echo "  $(pwd)/qtype"

if [ "$BUILD_TESTS" = "ON" ]; then
    echo ""
    print_info "To run tests:"
    echo "  ctest"
    echo "  ./qtype_tests"
fi

echo ""
print_success "Done!"
echo ""