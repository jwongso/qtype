# qtype – Human-Like Typing Simulator

## "Type Human, Stay Human OR Real Fingers, Digital Proof"

This project provides a desktop application that simulates highly human-like typing patterns, including natural timing variation, digraph-based delays, fatigue effects, and optional intentional imperfections such as typos and double-key presses.  
It is designed for realistic keystroke dynamics rather than automation speed.

The program is built with Qt (C++17) and uses `ydotool` for low-level input injection on Linux.

<img width="1145" height="1408" alt="image" src="https://github.com/user-attachments/assets/ee2ce113-034e-4cb3-867a-b7e2024ec5b2" />



## Features

### Human-like typing behavior
- Gamma-distributed key timing (non-uniform delay).
- Digraph correlation (common letter pairs typed faster).
- Natural rhythm modulation based on sinusoidal variation.
- Fatigue factor that gradually slows typing over long passages.
- Micro-stutters, idle pauses, and burst-typing events.

### Optional human imperfections
- Neighbor-key typos (random incorrect key close to the intended one).
- Double-key presses simulating hardware bounce.
- Automatic self-correction with configurable probability.

### Safety and stability
- Watchdog timer to detect stalls.
- Reset protection for stuck keys.
- Graceful stop via the ESC key.

### UI controls
- Delay range customization.
- Multiple timing profiles (Advanced, Fast, Slow & Tired, Professional).
- Fine-grained control over typo frequency, double-keypress frequency, and correction probability.

## Platform Notes

### Linux
- This application relies on `ydotool` for key injection, which requires Wayland.
- This project is **not supported on X11**.
  X11 does not provide sufficiently reliable or undetectable low-level key simulation for this tool.

### macOS
- macOS support exists in the code via the CGEvent API, but **has not yet been tested**.
- Behavior may differ depending on system security settings (Accessibility permissions, event tap restrictions).

### Windows
- Windows is not currently supported.

## Requirements

### Build Dependencies
- Qt 6 (Core, Widgets)
- C++17-compatible compiler
- CMake or qmake (whichever you prefer)

### Runtime Dependencies (Linux)
- Wayland session
- `ydotoold` running as root or system service
- User access to `/run/ydotoold/` socket (via systemd service or permissions adjustment)

## Installing ydotool (Linux)

Most distributions package ydotool. Example (Fedora):

```bash
sudo dnf install ydotool
sudo systemctl enable --now ydotoold.service
```

Verify that the daemon is running:
```bash
systemctl status ydotoold
```

Building

Using qmake:
```bash
qmake
make -j$(nproc)
```

Or with CMake:
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Run the application
```bash
./qtype
```

## Usage

1. Paste or type text into the input area.

2. Choose a timing profile.

3. Adjust delay ranges or enable/disable imperfections.

4. Press Start. A 5-second countdown will begin.

5. The program begins typing into whichever window is currently focused.

6. Press ESC to stop immediately.

## Disclaimer

This tool is intended for research, testing, and personal workflow enhancement.
Users are responsible for ensuring compliance with terms of service for any platforms or applications where it is used.
