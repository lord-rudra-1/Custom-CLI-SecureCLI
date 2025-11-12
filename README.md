# Custom CLI - Secure CLI (Phase 4)

A custom command-line interface (CLI) implementation that provides file management, process management, terminal UI features, plugin architecture, scripting support, and an interactive dashboard with a QEMU-like interface.

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

### 5. Phase 4: Visualization, Plugins, and Polish

#### Dashboard (ncurses)
- **dashboard**: Launch interactive terminal dashboard
  - Real-time process list display
  - Memory usage monitoring
  - Command log viewer
  - System status (uptime, load average, time)
  - Color-coded panels with keyboard navigation
  - Press 'q' to quit, 'r' to refresh

#### Plugin Architecture
- **plugins**: List and manage plugins
  - Dynamic loading of `.so` shared libraries from `./plugins` directory
  - Uses `dlopen()`/`dlsym()` for runtime plugin loading
  - Plugin commands integrate seamlessly with built-in commands
  - Support for plugin initialization and description functions
  - Commands: `plugins`, `plugins reload`, `plugins load <file>`, `plugins unload <name>`

#### Scripting Support
- **source**: Execute `.cli` script files
  - Batch command execution
  - Variable support with `$VAR` expansion
  - Comments with `#`
  - Example: `source setup.cli`

#### Custom Shell Language
- Variable assignment: `set VAR value`
- Variable expansion: `$VAR` in commands
- Echo command for output
- Full integration with all built-in and plugin commands

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

### Build example plugin:
```bash
gcc -shared -fPIC -o plugins/example_plugin.so plugins/example_plugin.c
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
- **dashboard.c**: Ncurses dashboard implementation
- **plugin.c**: Plugin loading and management (dlopen/dlsym)
- **script.c**: Script execution and variable handling
- **auth.c**: Authentication and authorization
- **logger.c**: Command logging
- **config.c**: Configuration management

Each module has its corresponding header file for function declarations.

## Phase 4 New Modules

### `dashboard.c` & `dashboard.h`
- **Purpose**: Interactive ncurses-based terminal dashboard
- **Functionality**:
  - `dashboard_init()` - Initialize ncurses and create windows
  - `dashboard_update()` - Refresh all panels with current data
  - `dashboard_cleanup()` - Cleanup and restore terminal
  - Displays process list, memory usage, logs, and system status in panels

### `plugin.c` & `plugin.h`
- **Purpose**: Dynamic plugin loading system
- **Functionality**:
  - `plugin_init()` - Initialize plugin system
  - `plugin_load()` - Load a specific plugin from .so file
  - `plugin_load_all()` - Load all plugins from ./plugins directory
  - `plugin_get()` - Get plugin by name
  - `plugin_unload()` - Unload a plugin
  - Uses `dlopen()`/`dlsym()` for dynamic loading

### `script.c` & `script.h`
- **Purpose**: Script execution and variable management
- **Functionality**:
  - `script_execute()` - Execute a .cli script file
  - `script_is_cli_file()` - Check if file is a .cli script
  - Variable storage and expansion ($VAR syntax)
  - Command execution within script context

## Security Analysis

### Input Sanitization
- **Command Injection Prevention**: Input sanitization in `cmd_exec()` checks for dangerous characters (`;|><`$`)
- **Path Traversal Protection**: File operations validate paths before execution
- **Buffer Overflow Protection**: Uses `strncpy()` with size limits throughout

### Authentication & Authorization
- **User Authentication**: Password-based authentication with hashed storage
- **Role-Based Access Control**: Admin and user roles with different permissions
- **Admin-Only Commands**: `delete` and `killproc` require admin privileges
- **Command Logging**: All commands are logged for audit trails

### Process Security
- **Secure Process Execution**: Uses `fork()`/`execvp()` instead of `system()` to prevent shell injection
- **Signal Handling**: Proper signal forwarding to child processes
- **Job Isolation**: Background jobs are tracked and isolated

### Plugin Security
- **Dynamic Loading**: Plugins loaded at runtime with `dlopen()`/`dlsym()`
- **Plugin Isolation**: Plugins run in the same process but are isolated by design
- **Path Validation**: Plugin paths are validated before loading
- **Limited Plugin Directory**: Plugins only loaded from `./plugins` directory

### Script Security
- **Variable Expansion**: Limited variable expansion to prevent code injection
- **Script Validation**: Scripts are read line-by-line with size limits
- **Command Execution**: Scripts use the same secure command execution as interactive mode

### Recommendations
1. **File Permissions**: Ensure `users.db` and log files have restricted permissions (600)
2. **Plugin Verification**: Consider adding plugin signature verification
3. **Script Sandboxing**: Consider running scripts in a more isolated environment
4. **Input Validation**: Expand input validation to cover more edge cases
5. **Memory Safety**: Consider using static analysis tools (Valgrind, AddressSanitizer)

## Testing

### Unit Tests
The project includes a custom test harness in `tests/`:
- `test_harness.h` - Test framework macros
- `test_script.c` - Script module tests
- `test_plugin.c` - Plugin module tests

### Running Tests
```bash
cd tests
gcc -o test_script test_script.c test_harness.c ../script.c -I..
./test_script

gcc -o test_plugin test_plugin.c test_harness.c ../plugin.c -I.. -ldl
./test_plugin
```

## Documentation

### Doxygen Documentation
Generate documentation with:
```bash
doxygen Doxyfile
```
Documentation will be generated in `docs/html/`

### Plugin Development

To create a plugin:

1. Create a `.c` file in `plugins/`:
```c
#include <stdio.h>

void plugin_myplugin_command(int argc, char *argv[]) {
    printf("Hello from my plugin!\n");
}

char *plugin_myplugin_description(void) {
    return "My custom plugin";
}
```

2. Compile as shared library:
```bash
gcc -shared -fPIC -o plugins/myplugin.so plugins/myplugin.c
```

3. The plugin will be automatically loaded on next CLI startup, or use:
```bash
plugins load plugins/myplugin.so
```

### Script Development

Create a `.cli` script file:
```bash
# setup.cli
set DIR /tmp
set FILE test.txt
create $FILE
list $DIR
echo Setup complete!
```

Execute with:
```bash
source setup.cli
```

## Dependencies

- **ncurses**: For dashboard functionality (`libncurses-dev` on Debian/Ubuntu)
- **readline**: For command history and autocomplete (`libreadline-dev`)
- **OpenSSL**: For password hashing (`libssl-dev`)
- **dl**: Dynamic linking (usually included in glibc)

### Installing Dependencies (Ubuntu/Debian)
```bash
sudo apt-get install libncurses-dev libreadline-dev libssl-dev
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

SecureSysCLI@qemu:~$ dashboard
# Launches interactive dashboard (press 'q' to quit)

SecureSysCLI@qemu:~$ source example.cli
Executing script: example.cli
...

SecureSysCLI@qemu:~$ plugins
Loaded plugins (1):
  example_plugin - Example plugin demonstrating plugin architecture

SecureSysCLI@qemu:~$ exit
Exiting SecureSysCLI...
```
