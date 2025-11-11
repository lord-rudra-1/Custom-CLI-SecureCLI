# Custom CLI - Secure CLI

A custom command-line interface (CLI) implementation that provides file management, process management, and terminal UI features with a QEMU-like interface.

## Project Structure

### Core Source Files

#### `main.c`
- **Purpose**: Main entry point and REPL (Read-Eval-Print Loop) implementation
- **Functionality**:
  - Initializes the terminal with QEMU-like styling
  - Implements the main command loop that reads user input
  - Tokenizes input commands and dispatches them to appropriate handlers
  - Handles exit/quit commands
  - Command dispatcher that maps command names to their functions

#### `commands.c` & `commands.h`
- **Purpose**: Implements basic CLI commands
- **Functionality**:
  - `cmd_hello()` - Prints a greeting message
  - `cmd_help()` - Displays help information for all available commands
  - `cmd_exec()` - Executes shell commands using `system()` call
  - `cmd_clear()` - Clears the terminal screen and displays banner

#### `file_management.c` & `file_management.h`
- **Purpose**: Implements file system operations
- **Functionality**:
  - `cmd_list()` - Lists files in a directory with their permissions (rwx format)
  - `cmd_create()` - Creates an empty file
  - `cmd_copy()` - Copies a file from source to destination
  - `cmd_delete()` - Deletes a file using `unlink()`

#### `process_management.c` & `process_management.h`
- **Purpose**: Implements process and job control operations
- **Functionality**:
  - `cmd_run()` - Executes a program (supports background execution with `&`)
  - `cmd_pslist()` - Lists all background jobs with their PIDs and status
  - `cmd_fgproc()` - Brings a background job to the foreground
  - `cmd_bgproc()` - Starts a program in the background
  - `cmd_killproc()` - Kills a process by PID
  - Internal job tracking system (add_job, remove_job) to manage up to 100 background processes

#### `terminal.c` & `terminal.h`
- **Purpose**: Terminal UI and styling functions
- **Functionality**:
  - `terminal_init()` - Initializes terminal and displays banner
  - `terminal_clear()` - Clears the terminal screen
  - `terminal_print_banner()` - Displays QEMU-like ASCII art banner
  - `terminal_print_prompt()` - Displays colored prompt (SecureSysCLI@qemu:~$)
  - Color management functions for ANSI color codes

### Build and Configuration Files

#### `Makefile`
- **Purpose**: Build configuration for the project
- **Functionality**:
  - Compiles all source files into object files
  - Links object files into the `project` executable
  - Provides `clean` target to remove build artifacts
  - Provides `run` target that executes the launch script

#### `launch.sh`
- **Purpose**: Launch script for the CLI application
- **Functionality**:
  - Detects available terminal emulator (gnome-terminal, xterm, konsole, etc.)
  - Launches the CLI in a new terminal window with appropriate title
  - Falls back to default terminal if no specific emulator is found

## Implemented Functionalities

### 1. Basic Commands
- **hello**: Simple greeting command
- **help**: Displays help information for all commands
- **clear**: Clears terminal and shows banner
- **exec**: Execute arbitrary shell commands

### 2. File Management
- **list**: List directory contents with file permissions
- **create**: Create empty files
- **copy**: Copy files from source to destination
- **delete**: Delete files

### 3. Process Management
- **run**: Execute programs (supports foreground and background with `&`)
- **pslist**: List all tracked background jobs
- **fgproc**: Bring background jobs to foreground
- **bgproc**: Start programs in background
- **killproc**: Kill processes by PID

### 4. Terminal Features
- QEMU-like styled banner and prompt
- ANSI color support for enhanced UI
- Terminal initialization and clearing

## Useless/Redundant Files

The following files are **NOT USED** in the current build and should be removed:

### `cli.c`
- **Status**: ❌ **USELESS - Duplicate/Old Version**
- **Reason**: Contains duplicate implementations of commands and its own `main()` function. Not included in the Makefile. All functionality is properly modularized in other files.

### `project.c`
- **Status**: ❌ **USELESS - Duplicate/Old Version**
- **Reason**: Another duplicate file with its own `main()` and all command implementations. Not included in the Makefile. This appears to be an old monolithic version before the code was split into modules.

### Build Artifacts (Should be in .gitignore)
- `a.out` - Old compiled executable
- `*.o` files (`main.o`, `commands.o`, `file_management.o`, `process_management.o`, `terminal.o`) - Object files generated during compilation
- `project` - The actual executable (should be built, not committed)

## Building and Running

### Build the project:
```bash
make
```

### Run the project:
```bash
make run
# or directly:
./project
```

### Clean build artifacts:
```bash
make clean
```

## Command Usage Examples

```
SecureSysCLI@qemu:~$ hello
Hello from SecureSysCLI! 👋

SecureSysCLI@qemu:~$ list .
rwxr-xr-x file1.txt
rw-r--r-- file2.txt

SecureSysCLI@qemu:~$ create newfile.txt
Created file: newfile.txt

SecureSysCLI@qemu:~$ run sleep 10 &
[bg] 12345 started: sleep

SecureSysCLI@qemu:~$ pslist
[0] PID 12345 running sleep

SecureSysCLI@qemu:~$ exit
Exiting SecureSysCLI...
```

## Architecture

The project follows a modular architecture:
- **main.c**: Entry point and command dispatcher
- **commands.c**: Basic CLI commands
- **file_management.c**: File operations
- **process_management.c**: Process/job control
- **terminal.c**: UI and terminal styling

Each module has its corresponding header file for function declarations.
