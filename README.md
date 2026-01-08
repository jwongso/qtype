# qtype – Human-Like Typing Simulator

**"Type Human, Stay Human"** • **"Real Fingers, Digital Proof"**

A cross-platform typing simulator that produces highly realistic human typing patterns with natural timing variations, digraph-based delays, fatigue effects, and optional imperfections. Includes AI-generated text detection, idle-based scrolling, and WebSocket remote control capabilities.

Built with Qt6 (C++17) and platform-specific input APIs for Linux, macOS, and Windows.

---

## Features

### 🎯 Human-Like Typing Behavior
- **Gamma-distributed timing**: Non-uniform, natural delays between keystrokes
- **Digraph correlation**: Common letter pairs typed faster (e.g., "th", "er")
- **Natural rhythm modulation**: Sinusoidal variation mimicking human typing patterns
- **Fatigue simulation**: Gradual slowdown over long passages
- **Micro-variations**: Stutters, idle pauses, and burst-typing events
- **Mouse movement**: Optional cursor jitter during typing

### 🔍 AI Content Detection
- **Non-ASCII scanner**: Detects em dashes (—), smart quotes (' ' " "), and Unicode characters
- **Pre-typing warnings**: Shows character positions and context before attempting to type
- **Safety alerts**: Warns about potential detection or keyboard mapping issues

### ⏱️ Idle Scrolling
- **System-level idle detection**: Platform-specific APIs for accurate user activity monitoring
  - Linux: XScreenSaver extension
  - macOS: Core Graphics event source
  - Windows: GetLastInputInfo API
- **Automatic scrolling**: Simulates natural scrolling after 30 seconds of inactivity
- **User activity tracking**: Resets immediately on any keyboard/mouse input

### 🎛️ Optional Imperfections
- **Neighbor-key typos**: Random incorrect keys near intended targets
- **Double-key presses**: Hardware bounce simulation
- **Automatic correction**: Self-fixes with configurable probability

### 🌐 WebSocket Architecture
- **Standalone Qt app**: Full-featured GUI for direct typing
- **WebSocket server**: Qt-based control center with remote management
- **WebSocket client**: Lightweight console app with no Qt dependency
  - Cross-platform native implementations
  - Remote control via server commands
  - Independent idle scrolling thread

### 🛡️ Safety & Stability
- **Watchdog timer**: Detects and prevents stalls
- **Reset protection**: Automatic stuck key recovery
- **ESC key abort**: Immediate graceful stop

---

## Platform Support

| Platform | Qt App | Client | Server | Input API | Idle Detection |
|----------|--------|--------|--------|-----------|----------------|
| **Linux** | ✅ | ✅ | ✅ | ydotool / XTest | XScreenSaver |
| **macOS** | ✅ | ✅ | ✅ | Core Graphics | CGEventSource |
| **Windows** | ✅ | ✅ | ✅ | Win32 SendInput | GetLastInputInfo |

### Platform-Specific Notes

#### Linux
- **X11**: Uses XTest extension for keyboard/mouse, XScreenSaver for idle detection
- **Wayland**: Requires `ydotool` for input injection, XScreenSaver for idle
- Libraries: `-lX11 -lXtst -lXss`

#### macOS
- Uses Core Graphics API (`CGEvent`) for all input simulation
- Requires Accessibility permissions in System Settings
- Framework: `-framework ApplicationServices`

#### Windows
- Uses Win32 API (`SendInput`, `GetLastInputInfo`)
- Static linking available for portable executables
- Cross-compilation supported from Linux via MinGW

---

## Building

### Unified Build Script

The `build_all.sh` script provides easy cross-platform compilation:

```bash
./build_all.sh [option]
```

**Options:**
- `linux` – Build for Linux (Qt app + WebSocket server/client)
- `windows` – Cross-compile for Windows from Linux
- `macos` – Build for macOS (requires macOS host with Qt6)
- `all` – Build all variants for current platform
- `clean` – Remove build artifacts (with optional binary removal)

**Examples:**
```bash
./build_all.sh linux        # Build everything for Linux
./build_all.sh clean        # Clean build files
./build_all.sh all          # Auto-detect and build for current OS
```

### Manual Build

#### Standalone Qt App
```bash
qmake qtype.pro
make -j$(nproc)
./qtype
```

#### WebSocket Server
```bash
cd websocket
mkdir build_server && cd build_server
cmake .. -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF
make -j$(nproc)
./qtype_server
```

#### WebSocket Client
```bash
cd websocket
g++ qtype_client.cpp -o qtype_client -std=c++17 \
    -lX11 -lXtst -lXss -pthread  # Linux
# Or for macOS:
clang++ qtype_client.cpp -o qtype_client -std=c++17 \
    -framework ApplicationServices -pthread
```

---

## Requirements

### Build Dependencies
- Qt 6 (Core, Widgets) – for Qt app and WebSocket server
- C++17-compatible compiler (g++, clang++, or MinGW)
- CMake 3.16+ or qmake

### Runtime Dependencies

#### Linux
- **X11**: `libX11`, `libXtst`, `libXss` (XScreenSaver extension)
- **Wayland**: `ydotoold` service running
  ```bash
  sudo dnf install ydotool          # Fedora
  sudo apt install ydotool          # Ubuntu/Debian
  sudo systemctl enable --now ydotoold.service
  ```

#### macOS
- Accessibility permissions for the application
- System Settings → Privacy & Security → Accessibility

#### Windows
- No additional runtime dependencies (static linking available)

---

## Usage

### Standalone Qt App

1. Launch `qtype`
2. Paste text into the input area
3. Choose a timing profile (Advanced, Fast, Slow & Tired, Professional)
4. Enable/disable features:
   - Typos, double-key presses, corrections
   - Mouse movement during typing
   - Idle-based scrolling
5. Press **Start** (5-second countdown)
6. Switch to target window
7. Press **ESC** to stop anytime

### WebSocket Architecture

#### Start Server
```bash
./qtype_server
```
- Default port: 8765
- Web interface shows connection status
- Configure all typing parameters via UI
- Send text to connected clients

#### Start Client
```bash
./qtype_client <server-ip> <port>
# Example:
./qtype_client 192.168.1.100 8765
```
- Connects to server automatically
- Receives typing commands remotely
- Independent idle scrolling (if enabled by server)
- Press Ctrl+C to disconnect

---

## AI Content Detection

The scanner detects common AI-generated text markers:

| Character | Type | Unicode | Detection |
|-----------|------|---------|-----------|
| `—` | Em dash | U+2014 | ✅ Blocked |
| `'` `'` | Smart quotes | U+2018/2019 | ✅ Blocked |
| `"` `"` | Smart quotes | U+201C/201D | ✅ Blocked |
| Extra spaces | Whitespace | Multiple spaces | ⚠️ Warning |

**Why this matters:**
- US keyboards can only type ASCII characters: `'` `"` `-`
- Smart quotes and em dashes appear from:
  - AI-generated text (GPT, Claude, etc.)
  - Copy-pasted Word documents
  - Mobile keyboard auto-correction
  - macOS text substitution

The tool **warns before attempting to type** these characters, preventing detection issues.

---

## Prebuilt Binaries

Check the `binary/` folder for compiled executables:
- `qtype-linux-x64` – Standalone Qt app
- `qtype_server-linux-x64` – WebSocket server
- `qtype_client-linux-x64` – WebSocket client (Linux)
- `qtype_client-windows-x64.exe` – WebSocket client (Windows)

Make executable and run:
```bash
chmod +x binary/qtype-linux-x64
./binary/qtype-linux-x64
```

---

## Project Structure

```
qtype/
├── main.cpp                    # Standalone Qt application
├── typing_engine.h             # Core typing engine and simulators
├── qtype.pro                   # qmake project file
├── CMakeLists.txt              # CMake configuration
├── build_all.sh                # Unified build script
├── websocket/
│   ├── qtype_server.cpp        # Qt WebSocket server
│   ├── qtype_client.cpp        # Cross-platform console client
│   ├── CMakeLists.txt          # WebSocket CMake config
│   └── *.md                    # WebSocket documentation
├── binary/                     # Prebuilt executables
└── tests/
    └── tests.cpp               # Unit tests
```

---

## Disclaimer

This tool is intended for **research, testing, and personal workflow enhancement**.

Users are responsible for ensuring compliance with terms of service for any platforms or applications where it is used. The AI content detection feature is designed to help users avoid unintentional markers, not to circumvent detection systems maliciously.

**Use responsibly and ethically.**
