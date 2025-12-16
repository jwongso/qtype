#!/bin/bash
# Build qtype for Windows from WSL

set -e

echo "Building qtype.exe for Windows from WSL..."
echo ""

# Check if MinGW cross-compiler is installed
if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "MinGW cross-compiler not found. Installing..."
    echo ""

    # Detect distro and install
    if [ -f /etc/debian_version ]; then
        # Debian/Ubuntu
        echo "Detected Debian/Ubuntu"
        sudo apt update
        sudo apt install -y mingw-w64
    elif [ -f /etc/fedora-release ]; then
        # Fedora
        echo "Detected Fedora"
        sudo dnf install -y mingw64-gcc-c++
    elif [ -f /etc/arch-release ]; then
        # Arch
        echo "Detected Arch Linux"
        sudo pacman -S mingw-w64-gcc
    else
        echo "Unknown distribution. Please install mingw-w64 manually:"
        echo "  Ubuntu/Debian: sudo apt install mingw-w64"
        echo "  Fedora: sudo dnf install mingw64-gcc-c++"
        echo "  Arch: sudo pacman -S mingw-w64-gcc"
        exit 1
    fi

    echo ""
    echo "MinGW installed successfully!"
    echo ""
fi

# Compile
echo "Compiling qtype_console.cpp..."
x86_64-w64-mingw32-g++ qtype_console.cpp \
    -o qtype.exe \
    -std=c++17 \
    -O2 \
    -static \
    -static-libgcc \
    -static-libstdc++

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful: qtype.exe"
    echo ""
    echo "File size: $(du -h qtype.exe | cut -f1)"
    echo ""
    echo "Usage:"
    echo "  ./qtype.exe -i input.txt"
    echo "  or copy to Windows: cp qtype.exe /mnt/c/Users/YourName/Desktop/"
    echo ""
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi
