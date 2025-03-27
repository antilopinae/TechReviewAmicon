# TechTask: System Monitoring and Logging Application

## Project Overview

The project comprises three main components: a **Server**, a **Client**, and a **JournalUtility**.

The core objective is to create an application suite capable of monitoring the CPU usage of processes on a server and logging these monitoring activities. The Server component listens for requests on UDP ports, processes these requests by calculating CPU usage, and logs each request and its result. The Client is designed to interact with the Server in two distinct modes, and the JournalUtility utility is responsible for persisting the Server's operational journal to a file.

This project is intended for **Linux** environments.

## Project Components

### 1. Server

The Server is the central component that monitors process CPU usage and maintains a log of monitoring events.

**Key Components:**

*   **Shared Memory Region (Journal):**  Acts as a persistent "journal" to record all monitoring requests and server responses.
*   **Worker Processes (5):**  Consists of five worker processes, each dedicated to listening on a separate UDP port within the range of 8080-8084 (configurable). These workers handle incoming client requests.

**Functionality:**

*   **UDP Request Handling:**
    *   Listens for incoming requests on designated UDP ports.
    *   Expects each request to contain a single Process ID (PID) for monitoring.
    *   Each request is processed individually, targeting one PID per request.
*   **Process Monitoring and Logging Logic:**
    *   Upon receiving a PID, a worker process performs the following actions:
        *   **Timestamping:** Records the current **date and time** when the request is received.
        *   **CPU Usage Calculation:** Calculates the current **CPU usage percentage** of the process identified by the received PID.
        *   **Client Response:** Sends a response back to the Client containing the calculated **CPU usage percentage** of the requested PID.
        *   **Journal Logging:**  Writes an entry to the shared memory journal in the format: `"DATE TIME: PID %CPU"`. This log entry documents the monitoring event.
*   **Error and Exception Handling & Logging:**
    *   **Process Not Found (PID Not Found):** If a process with the provided PID does not exist:
        *   Response to Client: Sends the message `"not found"`.
        *   Journal Log Entry: Logs the event in the journal as: `"DATE TIME: PID not found"`.
    *   **Invalid Request Format (Invalid Request):** If the incoming request is malformed or not in the expected PID format:
        *   Response to Client: Sends the message `"invalid"`.
        *   Journal Log Entry: Logs the error in the journal as: `"DATE TIME: invalid request"`.

### 2. Client

The Client application facilitates interaction with the Server and offers two distinct modes of operation to test and utilize the server's monitoring capabilities.

**Operating Modes:**

*   **Standard Mode:**
    *   **Input:** Requires the user to provide the **IP address and port of the Server** and a specific **Process ID (PID)** to monitor.
    *   **Action:** Sends a monitoring request to the Server for the CPU usage of the specified PID at the given server address and port.
    *   **Output:** Displays the **response received from the Server** in the console, which includes the CPU usage or error messages.

*   **"Fuzzing" Mode:**
    *   **Input:** Requires the user to provide the **IP address of the Server** and a **range of ports** on the server to target.
    *   **Action:** Initiates a "fuzzing" process by continuously sending monitoring requests (for a set of predefined or generated PIDs, or simply iterating through PIDs) to **all ports within the specified range on the Server, without any delay**. This mode is intended for stress-testing or quickly sending multiple requests.
    *   **Stopping Condition:** The "fuzzing" mode continues indefinitely until manually stopped by the user, typically by sending a termination signal (e.g., using `kill` command or `Ctrl+C`).

### 3. Journal Utility

The Journal Utility is a standalone program designed to persist the Server's in-memory journal to a file for record-keeping and analysis.

## Running the Applications

To simplify the process of running the Server, Client, and JournalUtility, use `build_and_run.sh`.

1.  **Using `build_and_run.sh`:**
    This script combines the build process with running the applications. It first builds the project and then runs the executables in separate terminals.

    ```bash
    ./build_and_run.sh
    ```

    The `build_and_run.sh` script will:
    *   **Clean and rebuild the project** using CMake (Debug build by default, configurable in the script).
    *   Clean the `note.txt` file.
    *   Open three new terminal windows and execute the Server, Client, and JournalUtility applications.

### Customizing Terminal Behavior in `build_and_run.sh`

Both scripts utilize the `RunInTerminal()` function to launch applications in new terminals. You can customize whether a terminal window remains open after the application finishes by using an optional second argument `"keep"` with the `RunInTerminal()` function call within these scripts.

## Files Included

*   **`server/server.c`:** C source code for the UDP Server application.
*   **`client/client.c`:** C source code for the UDP Client application.
*   **`server/journal_utility.c`:** C source code for the JournalUtility application.
*   **`CMakeLists.txt`:** CMake build configuration file for the entire project.
*   **`build_and_run.sh`:** Bash script to build the project and then run Server, Client, and JournalUtility in separate terminals.
*   **`cmake/`:** Directory containing custom CMake modules.

## Dependencies

*   **CMake (>= 3.20.0):** Required for building the project. Ensure CMake is installed on your system and accessible in your PATH.
*   **C Compiler:** A standard C compiler such as GCC or Clang is necessary to compile the C source code.
*   **Standard Linux C Libraries:** The project relies on standard C libraries, which are typically available on most systems with a C compiler. No external C libraries are explicitly required.

