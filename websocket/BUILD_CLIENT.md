# qtype_client - Cross-Platform Build Instructions

The qtype_client now supports MacOS, Linux, and Windows WSL.

## Platform-Specific Requirements

### MacOS
- Uses CoreGraphics (CGEvent API) for keyboard simulation
- Requires Accessibility permissions in System Preferences

**Build:**
```bash
# Using CMake
cmake -B build -DBUILD_CLIENT=ON
cmake --build build

# Or directly with clang++
clang++ qtype_client.cpp -o qtype_client -std=c++17 \
    -framework ApplicationServices -framework CoreFoundation
```

### Linux (Native)
- Uses X11/XTest for keyboard simulation
- Requires X11 development libraries

**Install Dependencies:**
```bash
sudo apt-get update
sudo apt-get install libx11-dev libxtst-dev
```

**Build:**
```bash
# Using CMake
cmake -B build -DBUILD_CLIENT=ON
cmake --build build

# Or directly with g++
g++ qtype_client.cpp -o qtype_client -std=c++17 -lX11 -lXtst
```

**Run:**
```bash
# Make sure DISPLAY is set
echo $DISPLAY  # Should show something like :0 or :1

# If not set:
export DISPLAY=:0

# Run the client
./build/qtype_client <server_ip>
```

### Windows WSL (Windows Subsystem for Linux)
- Uses X11/XTest (same as Linux)
- Requires an X server running on Windows

**Install X Server on Windows:**
Choose one of these:
- **VcXsrv** (Recommended): https://sourceforge.net/projects/vcxsrv/
- **Xming**: https://sourceforge.net/projects/xming/
- **X410**: Available in Microsoft Store (paid)

**Install Dependencies in WSL:**
```bash
sudo apt-get update
sudo apt-get install libx11-dev libxtst-dev
```

**Configure X Server (VcXsrv):**
1. Start VcXsrv with these settings:
   - Display number: 0
   - Start no client
   - ✓ Disable access control (for testing)
   
2. In WSL, set DISPLAY environment variable:
```bash
# For WSL1:
export DISPLAY=:0

# For WSL2:
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0

# Add to ~/.bashrc to make permanent:
echo 'export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0' >> ~/.bashrc
```

**Build:**
```bash
# Using CMake
cmake -B build -DBUILD_CLIENT=ON
cmake --build build

# Or directly with g++
g++ qtype_client.cpp -o qtype_client -std=c++17 -lX11 -lXtst
```

**Test X11 Connection:**
```bash
# Install xeyes for testing
sudo apt-get install x11-apps

# This should open a window on your Windows desktop
xeyes
```

**Run Client:**
```bash
./build/qtype_client <server_ip>
```

## Usage

```bash
# Connect to server
./qtype_client 192.168.1.100

# The client will:
# 1. Connect to the server via WebSocket
# 2. Wait for typing commands
# 3. Simulate human-like typing when commanded
# 4. Report status back to server
```

## Troubleshooting

### Linux/WSL: "Cannot open X display"
- **Check DISPLAY variable:** `echo $DISPLAY`
- **Verify X server is running** (WSL only)
- **Test with:** `xeyes` or `xclock`
- **Check firewall settings** (WSL2)

### Linux/WSL: "Permission denied" for keyboard simulation
- X11 typically allows this by default
- If issues, check X server access control settings

### MacOS: Keyboard simulation not working
- Go to System Preferences → Security & Privacy → Privacy → Accessibility
- Add Terminal or your application to the allowed list

### WSL2 Specific Issues
If X11 forwarding doesn't work:
```bash
# Windows Firewall may block connections
# Add Windows Defender firewall rule for VcXsrv

# Alternative: Use WSL1 which has simpler networking
wsl --set-version Ubuntu 1
```

## Performance Notes

- **MacOS**: Direct CGEvent API, very responsive
- **Linux**: X11/XTest, minimal overhead
- **WSL**: Small network overhead due to X forwarding, but generally works well

## Alternative for WSL: xdotool

If you prefer not to compile against X11, you can modify the code to use `xdotool`:

```bash
sudo apt-get install xdotool

# Modify client to call:
system("xdotool type 'your text'");
```

However, the current X11/XTest implementation is more efficient and gives better control.
