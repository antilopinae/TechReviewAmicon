#!/bin/bash

# Switch to Release build (can be changed to Debug)
BUILD_TYPE="Release"
BUILD_TESTS="OFF"

read -p "Do you want to build the tests? (yes/no): " build_tests_answer
if [[ "$build_tests_answer" == "yes" ]]; then
    BUILD_TESTS="ON"
fi

# Configure CMake build directory
if ! cmake -B build -S . -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_BUILD_TEST="$BUILD_TESTS"; then
    echo "Error during CMake configuration"
    exit 1
fi

# Build the project using CMake
if ! cmake --build build --config "$BUILD_TYPE"; then
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

read -p "Do you want to run tests? (yes/no): " run_tests_answer
if [[ "$run_tests_answer" == "yes" ]]; then
    if [ -d "build/test" ]; then
        find build/test -type f -executable | while read -r test_executable; do
            echo "Running test: $test_executable"
            "$test_executable"
        done
    else
        echo "Test directory does not exist. No tests to run."
    fi
fi

RunInTerminal "./build/server/server '127.0.0.1' 5 8080 1048576 /tmp/sockjournal" "keep"
RunInTerminal "./build/server/journal_utility /tmp/sockjournal note.txt" "keep"
RunInTerminal "./build/client/client '127.0.0.1' nfuzz 1 8080 1 200" "keep"
