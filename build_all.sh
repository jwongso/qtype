#!/bin/bash
# Build script for qtype project
# Usage: ./build_all.sh [platform]
#   platform: linux, windows, macos, all, clean (default: all)

set -e  # Exit on error

PLATFORM="${1:-all}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY_DIR="$SCRIPT_DIR/binary"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== QType Build Script ===${NC}"
echo "Platform: $PLATFORM"
echo "Binary directory: $BINARY_DIR"
echo ""

# Create binary directory if it doesn't exist
mkdir -p "$BINARY_DIR"

build_linux() {
    echo -e "${YELLOW}Building for Linux...${NC}"
    
    # Build main Qt application
    if [ -f "$SCRIPT_DIR/build.sh" ]; then
        echo "Building qtype (Qt GUI)..."
        cd "$SCRIPT_DIR"
        ./build.sh
        if [ -f "qtype" ]; then
            cp qtype "$BINARY_DIR/qtype-linux-x64"
            chmod +x "$BINARY_DIR/qtype-linux-x64"
            echo -e "${GREEN}✓ qtype-linux-x64 built successfully${NC}"
        fi
    fi
    
    # Build WebSocket server
    echo "Building qtype_server (WebSocket server)..."
    cd "$SCRIPT_DIR/websocket"
    if [ -f "CMakeLists.txt" ]; then
        mkdir -p build_server
        cd build_server
        cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF 2>/dev/null || true
        make qtype_server 2>/dev/null || true
        if [ -f "qtype_server" ]; then
            cp qtype_server "$BINARY_DIR/qtype_server-linux-x64"
            chmod +x "$BINARY_DIR/qtype_server-linux-x64"
            echo -e "${GREEN}✓ qtype_server-linux-x64 built successfully${NC}"
        fi
        cd "$SCRIPT_DIR"
    fi
    
    # Build WebSocket client (no Qt dependency)
    echo "Building qtype_client (WebSocket client)..."
    g++ -o "$BINARY_DIR/qtype_client-linux-x64" \
        "$SCRIPT_DIR/websocket/qtype_client.cpp" \
        -lX11 -lXtst -lXss -std=c++17 -O2
    chmod +x "$BINARY_DIR/qtype_client-linux-x64"
    echo -e "${GREEN}✓ qtype_client-linux-x64 built successfully${NC}"
}

build_windows() {
    echo -e "${YELLOW}Building for Windows...${NC}"
    
    # Check if MinGW is installed
    if ! command -v x86_64-w64-mingw32-g++-posix &> /dev/null; then
        echo -e "${RED}Error: MinGW-w64 not found. Please install mingw-w64.${NC}"
        echo "On Ubuntu/Debian: sudo apt-get install mingw-w64"
        return 1
    fi
    
    # Build WebSocket client for Windows
    echo "Building qtype_client (WebSocket client) for Windows..."
    x86_64-w64-mingw32-g++-posix \
        -o "$BINARY_DIR/qtype_client-windows-x64.exe" \
        "$SCRIPT_DIR/websocket/qtype_client.cpp" \
        -static -std=c++17 -lws2_32 -O2 \
        -DWIN32_LEAN_AND_MEAN 2>&1 | grep -v "redefined" || true
    chmod +x "$BINARY_DIR/qtype_client-windows-x64.exe"
    echo -e "${GREEN}✓ qtype_client-windows-x64.exe built successfully${NC}"
}

build_macos() {
    echo -e "${YELLOW}Building for macOS...${NC}"
    
    # Check if we're on macOS
    if [[ "$OSTYPE" != "darwin"* ]]; then
        echo -e "${RED}Error: macOS builds must be run on macOS${NC}"
        return 1
    fi
    
    # Check for Homebrew Qt6
    if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
        echo -e "${RED}Error: Qt6 not found. Please install via Homebrew:${NC}"
        echo "brew install qt@6"
        return 1
    fi
    
    # Find qmake
    QMAKE="qmake6"
    if ! command -v qmake6 &> /dev/null; then
        QMAKE="qmake"
    fi
    
    # Build main Qt application
    echo "Building qtype (Qt GUI)..."
    cd "$SCRIPT_DIR"
    if [ -f "qtype.pro" ]; then
        $QMAKE qtype.pro
        make clean &> /dev/null || true
        make -j$(sysctl -n hw.ncpu)
        if [ -f "qtype.app/Contents/MacOS/qtype" ]; then
            cp "qtype.app/Contents/MacOS/qtype" "$BINARY_DIR/qtype-macos-x64"
            chmod +x "$BINARY_DIR/qtype-macos-x64"
            echo -e "${GREEN}✓ qtype-macos-x64 built successfully${NC}"
        elif [ -f "qtype" ]; then
            cp qtype "$BINARY_DIR/qtype-macos-x64"
            chmod +x "$BINARY_DIR/qtype-macos-x64"
            echo -e "${GREEN}✓ qtype-macos-x64 built successfully${NC}"
        fi
    fi
    
    # Build WebSocket server
    echo "Building qtype_server (WebSocket server)..."
    cd "$SCRIPT_DIR/websocket"
    if [ -f "CMakeLists.txt" ]; then
        mkdir -p build_server
        cd build_server
        cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6) 2>/dev/null || \
        cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF 2>/dev/null || true
        make qtype_server 2>/dev/null || true
        if [ -f "qtype_server" ]; then
            cp qtype_server "$BINARY_DIR/qtype_server-macos-x64"
            chmod +x "$BINARY_DIR/qtype_server-macos-x64"
            echo -e "${GREEN}✓ qtype_server-macos-x64 built successfully${NC}"
        fi
        cd "$SCRIPT_DIR"
    fi
    
    # Build WebSocket client (uses CGEvent framework)
    echo "Building qtype_client (WebSocket client)..."
    clang++ -o "$BINARY_DIR/qtype_client-macos-x64" \
        "$SCRIPT_DIR/websocket/qtype_client.cpp" \
        -framework ApplicationServices \
        -std=c++17 -O2
    chmod +x "$BINARY_DIR/qtype_client-macos-x64"
    echo -e "${GREEN}✓ qtype_client-macos-x64 built successfully${NC}"
}

clean_all() {
    echo -e "${YELLOW}Cleaning all build artifacts...${NC}"
    
    # Clean main build directory
    if [ -d "$SCRIPT_DIR/build" ]; then
        echo "Removing build/"
        rm -rf "$SCRIPT_DIR/build"
    fi
    
    # Clean websocket build directory
    if [ -d "$SCRIPT_DIR/websocket/build_server" ]; then
        echo "Removing websocket/build_server/"
        rm -rf "$SCRIPT_DIR/websocket/build_server"
    fi
    
    # Clean qmake generated files (macOS)
    if [ -f "$SCRIPT_DIR/Makefile" ]; then
        echo "Removing Makefile"
        rm -f "$SCRIPT_DIR/Makefile"
    fi
    if [ -f "$SCRIPT_DIR/.qmake.stash" ]; then
        rm -f "$SCRIPT_DIR/.qmake.stash"
    fi
    if [ -d "$SCRIPT_DIR/qtype.app" ]; then
        echo "Removing qtype.app/"
        rm -rf "$SCRIPT_DIR/qtype.app"
    fi
    
    # Clean object files and temporary files
    find "$SCRIPT_DIR" -name "*.o" -delete 2>/dev/null || true
    find "$SCRIPT_DIR" -name "*.d" -delete 2>/dev/null || true
    find "$SCRIPT_DIR" -name "moc_*" -delete 2>/dev/null || true
    
    # Clean binary directory (optional - ask user)
    if [ -d "$BINARY_DIR" ]; then
        echo -e "${YELLOW}Remove binaries in $BINARY_DIR? [y/N]${NC}"
        read -r response
        if [[ "$response" =~ ^[Yy]$ ]]; then
            echo "Removing binary/"
            rm -rf "$BINARY_DIR"
            mkdir -p "$BINARY_DIR"
        fi
    fi
    
    echo -e "${GREEN}✓ Clean complete${NC}"
}

# Main build logic
case "$PLATFORM" in
    linux)
        build_linux
        ;;
    windows)
        build_windows
        ;;
    macos)
        build_macos
        ;;
    clean)
        clean_all
        ;;
    all)
        if [[ "$OSTYPE" == "darwin"* ]]; then
            # On macOS, build all macOS variants
            build_macos
        elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
            # On Linux, build Linux and Windows (via cross-compilation)
            build_linux
            build_windows
        else
            echo -e "${YELLOW}Warning: Unknown OS, attempting Linux builds...${NC}"
            build_linux
        fi
        ;;
    *)
        echo -e "${RED}Error: Unknown platform '$PLATFORM'${NC}"
        echo "Usage: $0 [linux|windows|macos|clean|all]"
        exit 1
        ;;
esac

# Only show summary if not cleaning
if [ "$PLATFORM" != "clean" ]; then
    echo ""
    echo -e "${GREEN}=== Build Complete ===${NC}"
    echo "Binaries are in: $BINARY_DIR"
    ls -lh "$BINARY_DIR" | grep -v "^total" | grep -v "README.md"
fi
