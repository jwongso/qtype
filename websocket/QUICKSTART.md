# qtype_client - Quick Start Guide

## Summary of Changes

The qtype_client has been ported from MacOS-only to support:
✅ **MacOS** (original platform)
✅ **Linux** (native X11 systems)  
✅ **Windows WSL** (with X server)

## Key Changes Made

1. **Cross-platform keyboard simulation**:
   - MacOS: Uses CGEvent API (ApplicationServices framework)
   - Linux/WSL: Uses X11/XTest extension

2. **Conditional compilation**: Uses `#ifdef` to detect platform at compile time

3. **Updated CMakeLists.txt**: Automatically detects platform and links appropriate libraries

4. **Character type changes**: Changed from `UniChar` (MacOS-specific) to standard `unsigned char`

## Quick Build & Run

### Linux (your current system)
```bash
# Install dependencies (already done)
sudo apt-get install libxtst-dev

# Build
cmake -B build -DBUILD_CLIENT=ON
cmake --build build

# Run (requires server)
./build/qtype_client <server_ip>
```

### WSL Setup
```bash
# 1. Install X server on Windows (VcXsrv recommended)
#    Download from: https://sourceforge.net/projects/vcxsrv/

# 2. Start VcXsrv on Windows with:
#    - Display number: 0
#    - "Disable access control" checked

# 3. In WSL, set DISPLAY variable:
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0

# 4. Test X11 connection:
sudo apt-get install x11-apps
xeyes  # Should open a window on Windows

# 5. Build and run client (same as Linux)
cmake -B build -DBUILD_CLIENT=ON
cmake --build build
./build/qtype_client <server_ip>
```

### MacOS
```bash
# Build
cmake -B build -DBUILD_CLIENT=ON
cmake --build build

# Or compile directly:
clang++ qtype_client.cpp -o qtype_client -std=c++17 \
    -framework ApplicationServices -framework CoreFoundation

# Run
./qtype_client <server_ip>

# Note: May need to grant Accessibility permissions in System Preferences
```

## Testing

To test the keyboard simulation without a server, you can modify the code temporarily or test individual components. The client needs:

1. An X server running (Linux/WSL)
2. Proper DISPLAY environment variable set
3. A qtype_server to connect to

## Platform Detection

The code automatically detects your platform:
- `__APPLE__` → MacOS build
- `__linux__` → Linux/WSL build  
- `WIN32` → Windows (not fully implemented)

## Troubleshooting

**"Cannot open X display"** (Linux/WSL):
```bash
echo $DISPLAY  # Should show :0 or similar
export DISPLAY=:0  # Or for WSL2: use resolv.conf method
```

**"XTest extension not found"** (Build error):
```bash
sudo apt-get install libxtst-dev
```

**Keyboard simulation not working** (MacOS):
- System Preferences → Security & Privacy → Privacy → Accessibility
- Add your terminal app to the allowed list

## Current Build Status

✅ Successfully built on Linux (Ubuntu on WSL/native)
✅ Links properly to X11 and XTest libraries
✅ Displays correct usage information
✅ Ready to connect to qtype_server

## Next Steps

1. Start qtype_server on another machine (or same machine)
2. Run client: `./build/qtype_client <server_ip>`
3. Server will send typing commands via WebSocket
4. Client simulates human-like typing
