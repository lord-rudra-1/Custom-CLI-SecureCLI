#!/bin/bash

# Single-file script to build and launch SecureSysCLI
# Usage: ./launch.sh

set -e  # Exit on error

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Build the project first
echo "Building SecureSysCLI..."

# Check if Makefile exists
if [ ! -f "Makefile" ]; then
    echo "Error: Makefile not found!"
    exit 1
fi

# Build the project (quiet mode)
if make clean > /dev/null 2>&1; then
    echo "Cleaned previous build"
fi

if ! make > /dev/null 2>&1; then
    echo "Build failed!"
    exit 1
fi

# Check if executable exists
if [ ! -f "project" ]; then
    echo "Error: Executable 'project' not found after build!"
    exit 1
fi

echo "Build successful!"
echo ""

# Detect OS
OS="$(uname -s)"

# macOS detection and handling
if [ "$OS" = "Darwin" ]; then
    # Check for iTerm2 first (popular macOS terminal)
    if [ -d "/Applications/iTerm.app" ] || command -v iterm2 &> /dev/null; then
        osascript -e "tell application \"iTerm2\"" \
                  -e "tell current window" \
                  -e "create tab with default profile" \
                  -e "tell current session of current tab" \
                  -e "write text \"cd '$SCRIPT_DIR' && ./project\"" \
                  -e "end tell" \
                  -e "end tell" \
                  -e "end tell" 2>/dev/null && exit 0
    fi
    
    # Fallback to Terminal.app on macOS
    osascript -e "tell application \"Terminal\"" \
              -e "do script \"cd '$SCRIPT_DIR' && ./project\"" \
              -e "end tell" 2>/dev/null && exit 0
    
    # If osascript fails, just run in current terminal
    echo "Running in current terminal (macOS)..."
    cd "$SCRIPT_DIR" && ./project
    exit 0
fi

# Linux terminal emulator detection (unchanged for Linux compatibility)
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

