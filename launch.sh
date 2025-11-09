#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Detect available terminal emulator and launch project in new window
if command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="SecureSysCLI - QEMU Terminal" -- bash -c "cd '$SCRIPT_DIR' && ./project; exec bash"
elif command -v xterm &> /dev/null; then
    xterm -title "SecureSysCLI - QEMU Terminal" -e bash -c "cd '$SCRIPT_DIR' && ./project; exec bash"
elif command -v konsole &> /dev/null; then
    konsole --title "SecureSysCLI - QEMU Terminal" -e bash -c "cd '$SCRIPT_DIR' && ./project; exec bash"
elif command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="SecureSysCLI - QEMU Terminal" -e bash -c "cd '$SCRIPT_DIR' && ./project; exec bash"
elif command -v mate-terminal &> /dev/null; then
    mate-terminal --title="SecureSysCLI - QEMU Terminal" -e bash -c "cd '$SCRIPT_DIR' && ./project; exec bash"
elif command -v lxterminal &> /dev/null; then
    lxterminal --title="SecureSysCLI - QEMU Terminal" -e bash -c "cd '$SCRIPT_DIR' && ./project; exec bash"
else
    # Fallback: try to use $TERMINAL environment variable or default
    if [ -n "$TERMINAL" ]; then
        $TERMINAL -e bash -c "cd '$SCRIPT_DIR' && ./project; exec bash"
    else
        echo "No suitable terminal emulator found. Please run: ./project"
        echo "Or install one of: gnome-terminal, xterm, konsole, xfce4-terminal"
        exit 1
    fi
fi

