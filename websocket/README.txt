╔════════════════════════════════════════════════════════════════╗
║         qtype_client - Cross-Platform Quick Reference         ║
╚════════════════════════════════════════════════════════════════╝

┌─ LINUX (Native) ──────────────────────────────────────────────┐
│ Dependencies:  sudo apt-get install libxtst-dev               │
│ Build:         cmake -B build -DBUILD_CLIENT=ON               │
│                cmake --build build                             │
│ Run:           ./build/qtype_client <server_ip>               │
│ Note:          Requires DISPLAY environment variable set       │
└───────────────────────────────────────────────────────────────┘

┌─ WINDOWS WSL ─────────────────────────────────────────────────┐
│ 1. Install X Server on Windows (VcXsrv/Xming)                │
│ 2. Start X Server with "Disable access control"              │
│ 3. In WSL:                                                     │
│    ./setup_wsl_display.sh     # Configure DISPLAY            │
│    sudo apt-get install libxtst-dev                           │
│    cmake -B build -DBUILD_CLIENT=ON                           │
│    cmake --build build                                         │
│ 4. Test: xeyes (should show window on Windows)               │
│ 5. Run:  ./build/qtype_client <server_ip>                    │
└───────────────────────────────────────────────────────────────┘

┌─ MACOS ───────────────────────────────────────────────────────┐
│ Build:         cmake -B build -DBUILD_CLIENT=ON               │
│                cmake --build build                             │
│ Permissions:   System Preferences → Security & Privacy →      │
│                Privacy → Accessibility → Add Terminal         │
│ Run:           ./build/qtype_client <server_ip>               │
└───────────────────────────────────────────────────────────────┘

┌─ TROUBLESHOOTING ─────────────────────────────────────────────┐
│ "Cannot open X display" (Linux/WSL):                          │
│   → echo $DISPLAY                                              │
│   → export DISPLAY=:0     # or run setup_wsl_display.sh      │
│                                                                 │
│ "XTest extension not found" (Build):                          │
│   → sudo apt-get install libxtst-dev                          │
│                                                                 │
│ WSL2 X11 not working:                                         │
│   → Check Windows Firewall allows VcXsrv                      │
│   → DISPLAY should be Windows IP (from resolv.conf)          │
│   → Test with: xeyes                                           │
│                                                                 │
│ MacOS simulation not working:                                 │
│   → Grant Accessibility permissions to Terminal               │
└───────────────────────────────────────────────────────────────┘

┌─ USAGE ───────────────────────────────────────────────────────┐
│ ./build/qtype_client <server_ip>                              │
│                                                                 │
│ Example: ./build/qtype_client 192.168.1.100                   │
│                                                                 │
│ The client will:                                               │
│   • Connect to server via WebSocket (port 9999)               │
│   • Wait for typing commands                                   │
│   • Simulate human-like typing                                 │
│   • Report status (busy/free) to server                       │
└───────────────────────────────────────────────────────────────┘

┌─ FILES ───────────────────────────────────────────────────────┐
│ qtype_client.cpp       - Main source code                     │
│ CMakeLists.txt         - Build configuration                  │
│ BUILD_CLIENT.md        - Detailed build instructions          │
│ QUICKSTART.md          - Quick start guide                    │
│ PORTING_SUMMARY.md     - Technical details of porting         │
│ setup_wsl_display.sh   - WSL DISPLAY helper script            │
│ README.txt             - This file                             │
└───────────────────────────────────────────────────────────────┘

┌─ PLATFORM SUPPORT ────────────────────────────────────────────┐
│ ✅ MacOS       - CGEvent API (ApplicationServices)            │
│ ✅ Linux       - X11/XTest extension                           │
│ ✅ WSL         - X11/XTest with X server on Windows           │
│ ❌ Windows     - Not yet implemented (use WSL)                │
└───────────────────────────────────────────────────────────────┘

For more information, see BUILD_CLIENT.md or PORTING_SUMMARY.md
