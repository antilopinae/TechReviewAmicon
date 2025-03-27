#!/bin/bash

# Switch to Release build (can be changed to Debug)
BUILD_TYPE="Debug"
BUILD_WITH_MEMCHECK="OFF" # ON/OFF

# Configure CMake build directory
if ! cmake -B build -S . -DCMAKE_BUILD_TYPE="$BUILD_TYPE"; then
    echo "Error during CMake configuration"
    exit 1
fi

# Set CMake options for auto memcheck-cover to all targets
memcheck_command=""
if [ "BUILD_WITH_MEMCHECK" = "ON" ]; then
    memcheck_command="--target memcheck-Server --target memcheck-Client --target memcheck-JournalUtility"
fi

# Build the project using CMake
if ! cmake --build build --config "$BUILD_TYPE" $memcheck_command; then
    echo "Error during CMake build"
    exit 1
fi

# Clean note.txt for JournalUtility
> note.txt

# Function to open a new terminal and run a command inside it
# Parameter 1: command to execute
# Parameter 2 (optional): "keep" to keep terminal open after command finishes
RunInTerminal() {
    local command="$1"
    local keep_open="$2"

    local keep_terminal_command=""
    if [ "$keep_open" = "keep" ]; then
        keep_terminal_command="; exec bash" # Append command to start a new bash shell after the command finishes
    fi

    # Try to find and use different terminal emulators
    if command -v x-terminal-emulator >/dev/null 2>&1; then
        x-terminal-emulator -e "bash -c '$command${keep_terminal_command}'" &
    elif command -v gnome-terminal >/dev/null 2>&1; then
        gnome-terminal -- bash -c "$command${keep_terminal_command}" &
    elif command -v konsole >/dev/null 2>&1; then
        konsole -e bash -c "$command${keep_terminal_command}" &
    elif command -v xfce4-terminal >/dev/null 2>&1; then
        xfce4-terminal -e "bash -c '$command${keep_terminal_command}'" &
    elif command -v xterm >/dev/null 2>&1; then
        xterm -e bash -c "$command${keep_terminal_command}" &
    else
        echo "A suitable terminal was not found..."
    fi
}

RunInTerminal "./build/Server" "keep"
RunInTerminal "./build/JournalUtility note.txt"
RunInTerminal "./build/Client"
