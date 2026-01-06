# qtype - Pre-compiled Binaries

This folder contains ready-to-use compiled binaries for different platforms.

## Available Binaries

### Standalone Qt Application
- **qtype-linux-x64** (178 KB)
  - Full-featured Qt GUI application
  - Includes all improvements: mouse movement, keyboard layouts, non-ASCII detection
  - Platform: Linux x64
  - Dependencies: Qt6 libraries (Core, Widgets)
  - Usage: `./qtype-linux-x64`

### WebSocket Server (Control Multiple Clients)
- **qtype_server-linux-x64** (213 KB)
  - Qt-based WebSocket server with GUI
  - Send typing commands to multiple remote clients
  - Platform: Linux x64
  - Dependencies: Qt6 libraries (Core, Widgets, Network, WebSockets)
  - Usage: `./qtype_server-linux-x64`
  - Default port: 9999

### WebSocket Clients (Pure C++, NO Qt dependency)
- **qtype_client-linux-x64** (80 KB)
  - Console client for Linux
  - Dependencies: X11, XTest
  - Usage: `./qtype_client-linux-x64 ws://SERVER_IP:9999`

- **qtype_client-windows-x64.exe** (2.7 MB)
  - Console client for Windows
  - Standalone executable (statically linked)
  - No dependencies required
  - Usage: `qtype_client-windows-x64.exe ws://SERVER_IP:9999`

## Quick Start

### Option 1: Standalone Application (Linux only)
```bash
./qtype-linux-x64
```

### Option 2: Server-Client Architecture (Multi-platform)

1. **Start server on Linux:**
   ```bash
   ./qtype_server-linux-x64
   ```
   Note the IP address shown (e.g., 192.168.1.100:9999)

2. **Start client on target machine:**
   ```bash
   # On Linux
   ./qtype_client-linux-x64 ws://192.168.1.100:9999
   
   # On Windows
   qtype_client-windows-x64.exe ws://192.168.1.100:9999
   ```

3. **Use server GUI to:**
   - Type text in the text area
   - Configure timing, profiles, keyboard layout
   - Click "Start" to send to connected clients

## Features

All binaries include:
- ✅ Thread-safe random number generation
- ✅ Named constants (no magic numbers)
- ✅ Memory leak fixes with smart pointers
- ✅ Advanced timing models (gamma distribution, fatigue)
- ✅ Realistic imperfections (typos, corrections)

Standalone app additionally has:
- ✅ Mouse movement simulation
- ✅ Keyboard layout selection (US, UK, German, French)
- ✅ Non-ASCII character detection and warnings

## Platform Support

| Binary | Linux | Windows | macOS |
|--------|-------|---------|-------|
| qtype-linux-x64 | ✅ | ❌ | ❌ |
| qtype_server-linux-x64 | ✅ | ❌ | ❌ |
| qtype_client-linux-x64 | ✅ | ❌ | ❌ |
| qtype_client-windows-x64.exe | ❌ | ✅ | ❌ |

**Note:** macOS binaries not included. Compile from source on macOS.

## Troubleshooting

### Linux clients
- If "Permission denied": `chmod +x qtype-*`
- If Qt libraries missing: `sudo apt install qt6-base-dev`

### Windows client
- Run as Administrator for some applications
- Some protected apps (UAC prompts) cannot receive input
- Make sure firewall allows port 9999

## Compilation Info

- **Date:** January 7, 2026
- **Compilers:**
  - Linux: GCC with Qt6
  - Windows: MinGW-w64 with POSIX threads
- **Standards:** C++17
- **Threading:** POSIX threads (thread_local support)
