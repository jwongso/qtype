# Building qtype_client for Windows

## Requirements
- Windows 10 or later
- C++17 compatible compiler

## Option 1: Using MinGW-w64 (Recommended)

### Install MinGW-w64
Download from: https://www.mingw-w64.org/ or use MSYS2

**Important:** Use the POSIX threading model version of MinGW (not win32 threads).

### Compile
```bash
# On Windows with MinGW
g++ qtype_client.cpp -o qtype_client.exe -std=c++17 -static-libgcc -static-libstdc++ -lws2_32

# Cross-compile from Linux
x86_64-w64-mingw32-g++-posix qtype_client.cpp -o qtype_client.exe -std=c++17 -static-libgcc -static-libstdc++ -lws2_32
```

## Option 2: Using Visual Studio (MSVC)

### Install Visual Studio 2019 or later
With "Desktop development with C++" workload

### Compile from Developer Command Prompt
```bash
cl qtype_client.cpp /EHsc /std:c++17 /Fe:qtype_client.exe ws2_32.lib
```

## Running

1. Start the server (on Linux/Mac):
   ```bash
   ./qtype_server
   ```

2. On Windows, run the client:
   ```bash
   qtype_client.exe ws://SERVER_IP:9999
   ```

## Notes

- Windows uses `SendInput()` API for keyboard simulation
- Requires administrator privileges for some target applications
- Uses Unicode input for character support
- The executable is standalone (no Qt needed on client)

## Troubleshooting

### "Access Denied" or keys not working
- Run as Administrator
- Some protected applications (like UAC prompts) cannot receive simulated input

### Unicode characters not working
- Make sure the target application supports Unicode input
- Try running in a plain text editor first to test

### Connection issues
- Check firewall settings (allow port 9999)
- Make sure server IP and port are correct
- Server must be running before starting client
