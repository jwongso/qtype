#!/bin/bash
# setup_wsl_display.sh - Configure DISPLAY for WSL

echo "=== WSL X11 Display Setup ==="
echo ""

# Detect WSL version
if grep -qi microsoft /proc/version; then
    if grep -qi "WSL2" /proc/version; then
        WSL_VERSION=2
    else
        WSL_VERSION=1
    fi
else
    echo "Error: This doesn't appear to be WSL"
    exit 1
fi

echo "Detected: WSL${WSL_VERSION}"
echo ""

# Set DISPLAY based on version
if [ "$WSL_VERSION" -eq 1 ]; then
    export DISPLAY=:0
    DISPLAY_CMD='export DISPLAY=:0'
    echo "For WSL1, using: DISPLAY=:0"
else
    # WSL2 - need to get Windows host IP
    WINDOWS_IP=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}')
    export DISPLAY="${WINDOWS_IP}:0"
    DISPLAY_CMD="export DISPLAY=\$(cat /etc/resolv.conf | grep nameserver | awk '{print \$2}'):0"
    echo "For WSL2, using: DISPLAY=${DISPLAY}"
fi

echo ""
echo "Current DISPLAY: $DISPLAY"
echo ""

# Test if X server is accessible
echo "Testing X11 connection..."
if command -v xset &> /dev/null; then
    if xset q &>/dev/null; then
        echo "✅ X11 connection successful!"
    else
        echo "❌ Cannot connect to X server"
        echo ""
        echo "Make sure:"
        echo "  1. VcXsrv (or another X server) is running on Windows"
        echo "  2. 'Disable access control' is checked in VcXsrv settings"
        echo "  3. Windows Firewall allows VcXsrv connections"
        exit 1
    fi
else
    echo "⚠️  'xset' not found, cannot test connection"
    echo "   Install with: sudo apt-get install x11-xserver-utils"
fi

echo ""
echo "To make this permanent, add to ~/.bashrc:"
echo "  echo '${DISPLAY_CMD}' >> ~/.bashrc"
echo ""
echo "Run this command to add it now:"
echo "  source <(echo '${DISPLAY_CMD}')"
echo ""

# Offer to add to bashrc
read -p "Add DISPLAY export to ~/.bashrc? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    # Check if already exists
    if grep -q "export DISPLAY.*resolv.conf" ~/.bashrc || grep -q "export DISPLAY=:0" ~/.bashrc; then
        echo "DISPLAY export already exists in ~/.bashrc"
    else
        echo "" >> ~/.bashrc
        echo "# X11 Display for WSL" >> ~/.bashrc
        echo "${DISPLAY_CMD}" >> ~/.bashrc
        echo "✅ Added to ~/.bashrc"
    fi
fi

echo ""
echo "Setup complete! You can now run:"
echo "  ./build/qtype_client <server_ip>"
