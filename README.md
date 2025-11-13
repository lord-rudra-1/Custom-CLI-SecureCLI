# SecureSysCLI - Custom Secure Command Line Interface

A comprehensive, feature-rich command-line interface (CLI) implementation with authentication, file management, process control, encryption, remote access, plugin architecture, scripting support, and an interactive dashboard.

## Project Overview

SecureSysCLI is a modular, secure command-line interface built in C that provides:
- **Authentication**: Role-based login (admin/user) with hidden password entry
- **File Management**: Complete file operations with permission checking
- **Process Management**: Background/foreground job control with signal handling
- **Cryptography**: AES-256-GCM encryption/decryption and SHA-256 checksums
- **Dashboard**: Real-time ncurses-based system monitoring
- **Advanced UI**: Readline integration with history, autocomplete, and arrow key navigation

---

## Complete File Documentation

### Core Application Files

#### `main.c` (267 lines)
**Purpose**: Main entry point and REPL (Read-Eval-Print Loop) implementation

**Key Functionality**:
- **Command Dispatcher**: Maps command names to function pointers in a `Command` struct array
- **Readline Integration**: Uses GNU readline library for:
  - **Arrow Key Navigation**: Up/Down arrows to browse command history
  - **Tab Completion**: Autocomplete for commands and filenames
  - **Command History**: Persistent history saved to `.securecli_history`
  - **Custom Prompt**: Dynamic prompt showing `SecureSysCLI@<username>:~$`
- **Signal Handling**: SIGINT (Ctrl+C) forwarding to foreground processes
- **Plugin System Integration**: Checks plugins if built-in command not found
- **Configuration Loading**: Loads config and user database at startup
- **Graceful Exit**: Saves history and cleans up plugins on exit

**Key Functions**:
- `main()`: Initializes all systems, handles login, runs main REPL loop
- `securecli_completion()`: Custom readline completion function
- `command_generator()`: Generates command name completions
- `build_prompt()`: Creates dynamic prompt with logged-in username
- `sigint_handler()`: Handles Ctrl+C appropriately (main loop vs foreground process)

---

#### `commands.c` & `commands.h` (524 lines)
**Purpose**: Implements all built-in CLI commands with input sanitization

**Key Functionality**:
- **Input Sanitization**: `is_input_safe()` checks for dangerous characters (`;|><`$`)
- **Command Implementations**: All 30+ built-in commands
- **Permission Checking**: Admin-only commands check `is_admin()`
- **Command Logging**: All commands logged via `log_command()`

**Commands Implemented**:
- `cmd_hello()`: Simple greeting
- `cmd_help()`: Displays help with plugin list
- `cmd_clear()`: Clears terminal and shows banner
- `cmd_exec()`: Secure fork/exec with input sanitization
- `cmd_whoami()`: Shows current user and role
- `cmd_encrypt()`: File encryption with password prompt
- `cmd_decrypt()`: File decryption with password prompt
- `cmd_checksum()`: SHA-256 file checksum computation
- `cmd_server()`: Start TLS server (admin only)
- `cmd_client()`: Connect to TLS server
- `cmd_dashboard()`: Launch ncurses dashboard
- `cmd_source()`: Execute `.cli` script files
- `cmd_plugins()`: List, load, unload, reload plugins
- `cmd_close()`: Exit and close terminal window

**Security Features**:
- Input sanitization prevents command injection
- Password input hidden (no echo)
- Memory clearing of sensitive data (passwords)

---

#### `file_management.c` & `file_management.h` (165 lines)
**Purpose**: File system operations with permission checking

**Commands Implemented**:
- `cmd_list()`: Lists directory contents with permissions (rwx format)
  - **Default Behavior**: If no directory specified, lists current directory (`.`)
  - Shows 9-character permission string (rwxrwxrwx format)
- `cmd_create()`: Creates empty files
- `cmd_copy()`: Copies files with binary-safe buffer (1024 bytes)
- `cmd_delete()`: Deletes files (admin only, logged)
- `cmd_write()`: Writes text content to file (overwrites existing)
  - Joins all arguments after filename with spaces
  - Adds newline at end
- `cmd_show()`: Displays file contents (like `cat`)
  - Reads file in 512-byte chunks
  - Handles read errors gracefully

**Features**:
- Permission-based access control
- Error handling with `perror()`
- Binary-safe file operations

---

#### `process_management.c` & `process_management.h` (221 lines)
**Purpose**: Process and job control with background/foreground support

**Key Functionality**:
- **Job Tracking**: Maintains array of up to 100 background jobs
- **Background Process Isolation**: Redirects stdin/stdout/stderr to `/dev/null`
- **Signal Handling**: Proper SIGINT forwarding to foreground processes
- **Process Status Checking**: Verifies if processes are still running

**Commands Implemented**:
- `cmd_run()`: Execute programs (supports `&` for background)
  - **Foreground Mode**: Waits for process, forwards Ctrl+C
  - **Background Mode**: Detaches with `setsid()`, redirects I/O to `/dev/null`
- `cmd_pslist()`: Lists all tracked background jobs with PID and status
  - Shows running vs terminated processes
- `cmd_fgproc()`: Brings background job to foreground
  - Checks if process still exists before waiting
  - Waits for process completion
  - Shows exit status
- `cmd_bgproc()`: Starts program in background
  - Creates new session with `setsid()`
  - Redirects all I/O to `/dev/null`
  - Tracks job in job table
- `cmd_killproc()`: Kills process by PID (admin only)
  - Sends SIGTERM
  - Reaps zombie processes
  - Removes from job table

**Internal Functions**:
- `add_job()`: Adds process to job tracking table
- `remove_job()`: Removes process from job table

---

#### `terminal.c` & `terminal.h` (76 lines)
**Purpose**: Terminal UI, styling, and banner display

**Key Functionality**:
- **ANSI Color Support**: Full color management with reset
- **Banner Display**: Figlet-style "Secure CLI" ASCII art
- **Dynamic Prompt**: Shows logged-in username (not hostname)
- **Terminal Detection**: Only initializes if stdout is a TTY

**Functions**:
- `terminal_init()`: Initializes terminal, clears screen, shows banner
- `terminal_clear()`: Clears screen using ANSI escape codes
- `terminal_print_banner()`: Displays colored figlet banner
- `terminal_print_prompt()`: Prints colored prompt with username
- `terminal_set_color()`: Sets ANSI color
- `terminal_reset_color()`: Resets to default color

**Color Scheme**:
- Bright Cyan: Banner top lines
- Bright Green: Banner middle, prompt name
- Yellow: Banner bottom, prompt directory
- White: Separators

---

### Authentication & Security

#### `auth.c` & `auth.h` (108 lines)
**Purpose**: User authentication and role management

**Key Functionality**:
- **Password Hashing**: SHA-256 hashing of passwords
- **User Database**: Stores users in `users.db` file (username, hash, role)
- **Login System**: Interactive login with hidden password input
- **Role Checking**: `is_admin()` function for permission checks

**Functions**:
- `load_users()`: Loads users from `users.db` file
- `login()`: Interactive login prompt
  - Reads username (visible)
  - Reads password (hidden, no echo)
  - Hashes password and compares with stored hash
  - Sets `current_user` global variable
- `is_admin()`: Checks if current user has admin role
- `read_password()`: Helper to read password without echoing

**Security Features**:
- Passwords never stored in plaintext
- Password input hidden using `termios`
- SHA-256 hashing for password storage

---

#### `logger.c` & `logger.h` (45 lines)
**Purpose**: Command logging and audit trail

**Key Functionality**:
- **Timestamped Logging**: All commands logged with timestamp
- **Persistent Storage**: Logs saved to `securecli.log`
- **Format**: `[YYYY-MM-DD HH:MM:SS] <command>`

**Functions**:
- `log_command()`: Logs command with timestamp to file
- `logger_init()`: Initializes log file with header

**Log Format**:
```
[2024-01-15 14:30:45] hello
[2024-01-15 14:30:50] list .
[2024-01-15 14:31:00] UNAUTHORIZED delete attempt
```

---

#### `config.c` & `config.h` (34 lines)
**Purpose**: Configuration file management

**Key Functionality**:
- **Config File**: `.securecli_config` with key-value pairs
- **Default Values**: Provides defaults if config missing
- **Configurable Options**:
  - `color`: Default color scheme
  - `startup_dir`: Directory to change to on startup
  - `show_banner`: Whether to show banner (0/1)

**Functions**:
- `load_config()`: Loads configuration from file

**Config File Format**:
```
color = green
startup_dir = /home/user
show_banner = 1
```

---

#### `crypto.c` & `crypto.h` (247 lines)
**Purpose**: Cryptographic operations using OpenSSL

**Key Functionality**:
- **AES-256-GCM Encryption**: Industry-standard encryption
- **PBKDF2 Key Derivation**: 100,000 iterations for key stretching
- **SHA-256 Checksums**: File integrity verification
- **Secure Memory**: Clears sensitive data from memory

**Functions**:
- `encrypt_file()`: Encrypts file with password
  - Generates random salt and IV
  - Derives key using PBKDF2-HMAC-SHA256 (100k iterations)
  - Uses AES-256-GCM mode
  - Writes salt, IV, ciphertext, and authentication tag
- `decrypt_file()`: Decrypts file with password
  - Reads salt, IV, and tag
  - Derives key and verifies authentication tag
  - Returns generic error on failure (doesn't leak details)
- `sha256_file_hex()`: Computes SHA-256 hash of file
  - Returns hex string (64 characters)

**Security Features**:
- Random salt and IV for each encryption
- Authentication tag prevents tampering
- Memory clearing of keys and passwords
- Generic error messages (no information leakage)



### Scripting

#### `script.c` & `script.h` (223 lines)
**Purpose**: Custom scripting language for batch operations

**Key Functionality**:
- **Script Files**: Executes `.cli` script files
- **Variable Support**: `set VAR value` and `$VAR` expansion
- **Comments**: Lines starting with `#` are ignored
- **Command Execution**: Executes all CLI commands within scripts

**Functions**:
- `script_execute()`: Executes a `.cli` script file
  - Reads file line by line
  - Skips comments and empty lines
  - Expands variables
  - Executes commands
- `script_is_cli_file()`: Checks if file is a `.cli` script
- `set_variable()`: Sets a variable value
- `get_variable()`: Gets a variable value
- `expand_variables()`: Expands `$VAR` syntax in commands
- `execute_command_line()`: Executes command within script context

**Script Features**:
- Variable assignment: `set DIR /tmp`
- Variable expansion: `list $DIR`
- Comments: `# This is a comment`
- Echo command: `echo Hello World`
- All built-in commands available

**Script Example**:
```bash
# setup.cli
set DIR /tmp
set FILE test.txt
create $FILE
list $DIR
echo Setup complete!
```

---

### Dashboard

#### `dashboard.c` & `dashboard.h` (223 lines)
**Purpose**: Interactive ncurses-based system monitoring dashboard

**Key Functionality**:
- **Real-Time Monitoring**: Live system information
- **Four Panels**: Process list, memory usage, command log, system status
- **Keyboard Navigation**: 'q' to quit, 'r' to refresh
- **Color-Coded**: Different colors for different panels

**Functions**:
- `dashboard_init()`: Initializes ncurses and creates windows
- `dashboard_update()`: Refreshes all panels with current data
- `dashboard_cleanup()`: Cleans up and restores terminal
- `dashboard_is_active()`: Checks if dashboard is active
- `get_memory_info()`: Reads memory info from `/proc/meminfo`
- `read_log_lines()`: Reads recent log entries

**Dashboard Panels**:
1. **Process Panel** (Top Left): Shows top processes by CPU usage (PID, CPU%, MEM%)
2. **Memory Panel** (Top Right): Shows memory usage (Total, Used, Free, Available)
3. **Log Panel** (Bottom Left): Shows recent command log entries
4. **Status Panel** (Bottom Right): Shows uptime, current time, load average

**Controls**:
- `q`: Quit dashboard
- `r`: Refresh all panels

---

### Utility Files

#### `signals.h` (11 lines)
**Purpose**: Signal handling declarations

**Key Functionality**:
- Declares `foreground_pid` global variable
- Used for SIGINT forwarding to child processes

---

#### `launch.sh` (30 lines)
**Purpose**: Launch script for the CLI application

**Key Functionality**:
- Detects available terminal emulator (gnome-terminal, xterm, konsole, etc.)
- Launches CLI in new terminal window
- Sets `SECURECLI_LAUNCHED=1` environment variable for `close` command
- Falls back to default terminal if none found

**Supported Terminals**:
- gnome-terminal
- xterm
- konsole
- xfce4-terminal
- mate-terminal
- lxterminal

---

#### `Makefile` (40 lines)
**Purpose**: Build configuration

**Key Functionality**:
- Compiles all source files
- Links with required libraries (crypto, readline, ssl, ncurses, dl)
- Provides targets: `clean`, `run`, `plugin-example`, `test-script`, `test-plugin`, `docs`

**Build Targets**:
- `make`: Builds the project
- `make clean`: Removes object files and executable
- `make run`: Builds and runs via launch script
- `make plugin-example`: Builds example plugin
- `make test-script`: Builds script tests
- `make test-plugin`: Builds plugin tests
- `make docs`: Generates Doxygen documentation

---

## Feature Summary

### Core Features

1. **Authentication System**
   - Password-based login with SHA-256 hashing
   - Role-based access control (admin/user)
   - Hidden password input
   - User database in `users.db`

2. **Command History & Navigation**
   - **Arrow Key Navigation**: Up/Down arrows browse command history
   - **Tab Completion**: Autocomplete for commands and filenames
   - **Persistent History**: Saved to `.securecli_history`
   - **History Search**: Readline's built-in search capabilities

3. **File Management**
   - List files with permissions
   - Create, copy, delete files
   - Write text to files
   - Display file contents
   - Permission-based access control

4. **Process Management**
   - Run programs in foreground/background
   - Job tracking (up to 100 jobs)
   - Bring background jobs to foreground
   - Kill processes
   - Signal handling (Ctrl+C forwarding)

5. **Cryptography**
   - AES-256-GCM file encryption
   - PBKDF2 key derivation (100k iterations)
   - SHA-256 file checksums
   - Secure memory clearing


8. **Scripting Language**
    - Custom `.cli` script files
    - Variable support (`$VAR`)
    - Comments (`#`)
    - Batch command execution

9. **Interactive Dashboard**
    - Real-time process monitoring
    - Memory usage display
    - Command log viewer
    - System status (uptime, load, time)

10. **Terminal UI**
    - Figlet-style ASCII banner
    - Colored prompts
    - Dynamic username display
    - ANSI color support

11. **Command Logging**
    - All commands logged with timestamps
    - Persistent log file
    - Audit trail

12. **Configuration System**
    - Configurable startup directory
    - Banner display toggle
    - Color scheme settings

---

## All Commands

### Basic Commands
- `hello` - Print greeting message
- `help` - Show help with all commands and loaded plugins
- `clear` - Clear terminal and show banner
- `whoami` - Show current user and role
- `exit` / `quit` - Exit the CLI
- `close` - Exit and close terminal window (if launched via script)

### File Management
- `list [dir]` - List files with permissions (default: current directory)
- `create <filename>` - Create an empty file
- `copy <src> <dst>` - Copy a file
- `delete <file>` - Delete a file (admin only)
- `write <file> <text>` - Write text to a file
- `show <file>` - Display file contents (like `cat`)

### Process Management
- `run <program> [&]` - Run a program (append `&` for background)
- `pslist` - Show all background jobs
- `fgproc <jobid>` - Bring background job to foreground
- `bgproc <program> [args]` - Start program in background
- `killproc <pid>` - Kill a process by PID (admin only)

### System Execution
- `exec <program> [args]` - Execute a system program securely (with input sanitization)

### Cryptography
- `encrypt <in> <out>` - Encrypt a file with password (AES-256-GCM)
- `decrypt <in> <out>` - Decrypt a file with password
- `checksum <file>` - Compute SHA-256 checksum of a file

### Advanced Features
- `dashboard` - Launch interactive ncurses dashboard
- `source <script.cli>` - Execute a `.cli` script file

---

## User Interface Features

### Readline Integration

1. **Command History**
   - **Up Arrow**: Previous command
   - **Down Arrow**: Next command
   - **Persistent Storage**: History saved to `.securecli_history`
   - **History Search**: Readline's incremental search (Ctrl+R)

2. **Tab Completion**
   - **Command Names**: Tab completes command names
   - **Filenames**: Tab completes filenames for file operations
   - **Custom Completion**: Command-specific completion function

3. **Line Editing**
   - **Left/Right Arrows**: Move cursor
   - **Home/End**: Move to start/end of line
   - **Backspace/Delete**: Delete characters
   - **Ctrl+A**: Move to beginning
   - **Ctrl+E**: Move to end
   - **Ctrl+U**: Clear line
   - **Ctrl+K**: Delete to end of line

4. **Dynamic Prompt**
   - Shows logged-in username: `SecureSysCLI@<username>:~$`
   - Color-coded (green name, cyan username, yellow directory)
   - Updates based on current user

### Terminal Features

1. **Banner Display**
   - Figlet-style "Secure CLI" ASCII art
   - Color-coded (cyan, green, yellow)
   - Configurable via `show_banner` config option

2. **Color Support**
   - ANSI color codes throughout
   - Color-coded prompts
   - Color-coded command output
   - Dashboard with color panels

3. **Screen Management**
   - Terminal clearing
   - Cursor positioning
   - Dashboard full-screen mode

---

## Security Features

### Authentication & Authorization

1. **Password Security**
   - SHA-256 hashing (never stored in plaintext)
   - Hidden password input (no echo)
   - Memory clearing after use

2. **Role-Based Access Control**
   - Admin and user roles
   - Admin-only commands enforced
   - Default deny policy

3. **Access Control Lists**
   - Per-user, per-command rules
   - Wildcard support
   - Persistent storage
   - Secure by default

### Input Security

1. **Input Sanitization**
   - Dangerous character filtering (`;|><`$`)
   - Prevents command injection
   - Buffer overflow protection

2. **Path Validation**
   - File operation path checking
   - Plugin path validation
   - Script path validation

### Process Security

1. **Secure Execution**
   - Uses `fork()`/`execvp()` instead of `system()`
   - Prevents shell injection
   - Signal handling isolation

2. **Sandboxing**
   - Isolated execution environment
   - Chroot-based isolation
   - Namespace support

### Cryptographic Security

1. **Encryption**
   - AES-256-GCM (authenticated encryption)
   - PBKDF2 key derivation (100k iterations)
   - Random salt and IV
   - Authentication tags

2. **Integrity**
   - SHA-256 checksums
   - File tampering detection

### Audit & Logging

1. **Command Logging**
   - All commands logged with timestamps
   - Unauthorized attempts logged
   - Persistent audit trail

2. **Error Handling**
   - Generic error messages (no information leakage)
   - Secure error reporting

---

## Building and Running

### Prerequisites

Install dependencies (Ubuntu/Debian):
```bash
sudo apt-get install libncurses-dev libreadline-dev libssl-dev build-essential
```

### Build

```bash
make
```

### Run

```bash
# Direct execution
./project

# Or use launch script (opens in new terminal)
make run
# or
./launch.sh
```

### Clean

```bash
make clean
```

### Build Example Plugin

```bash
make plugin-example
```

### Run Tests

```bash
make test-script
./tests/test_script

make test-plugin
./tests/test_plugin
```

### Generate Documentation

```bash
make docs
# Documentation in docs/html/
```

---

## Architecture

### Modular Design

The project follows a modular architecture with clear separation of concerns:

```
main.c                    - Entry point, REPL, command dispatch
├── commands.c            - Command implementations
├── file_management.c    - File operations
├── process_management.c - Process/job control
├── terminal.c           - UI and styling
├── auth.c               - Authentication
├── logger.c             - Command logging
├── config.c             - Configuration
├── crypto.c             - Cryptography
├── remote.c             - TLS server/client
├── plugin.c             - Plugin system
├── script.c             - Scripting engine
└── dashboard.c          - Interactive dashboard
```

### Data Flow

1. **User Input**: Readline reads input with history/completion
2. **Tokenization**: Input split into command and arguments
3. **Command Dispatch**: Command matched to function or plugin
4. **Execution**: Command executed with logging
5. **Output**: Results displayed to user

### Global State

- `current_user`: Currently logged-in user (from `auth.c`)
- `foreground_pid`: PID of foreground process (for signal handling)
- `in_main_loop`: Flag indicating if in main loop (for signal handling)
- Plugin registry: Loaded plugins (from `plugin.c`)
- Job table: Background processes (from `process_management.c`)

---

## Dependencies

### Required Libraries

- **libreadline**: Command history, autocomplete, arrow key navigation
- **libncurses**: Interactive dashboard
- **libcrypto** (OpenSSL): Encryption, hashing, PBKDF2
- **libssl** (OpenSSL): TLS server/client
- **libdl**: Dynamic plugin loading

### System Requirements

- Linux (uses Linux-specific features: `/proc`, etc.)
- POSIX-compliant system
- Terminal with ANSI color support
- OpenSSL 1.1+ or 3.0+

### Optional Tools

- **figlet**: For generating banner text (not required at runtime)
- **doxygen**: For generating documentation
- **gcc**: C compiler with C99 support

---

## File Structure

```
Custom-CLI-SecureCLI/
├── main.c                 - Main entry point and REPL
├── commands.c/h           - Command implementations
├── file_management.c/h   - File operations
├── process_management.c/h - Process control
├── terminal.c/h           - UI and styling
├── auth.c/h               - Authentication
├── logger.c/h             - Command logging
├── config.c/h             - Configuration
├── crypto.c/h             - Cryptography
├── remote.c/h             - TLS remote access
├── plugin.c/h             - Plugin system
├── script.c/h             - Scripting engine
├── dashboard.c/h           - Interactive dashboard
├── signals.h              - Signal handling
├── Makefile               - Build configuration
├── launch.sh              - Launch script
├── README.md              - This file
├── users.db                - User database (created at runtime)
├── securecli.log           - Command log (created at runtime)
├── .securecli_history      - Command history (created at runtime)
├── .securecli_config       - Configuration file (optional)
├── plugins/                - Plugin directory
│   └── example_plugin.c    - Example plugin
├── tests/                  - Test files
│   ├── test_harness.c/h    - Test framework
│   ├── test_script.c       - Script tests
│   └── test_plugin.c       - Plugin tests
└── docs/                   - Documentation
    └── mainpage.dox        - Doxygen main page
```

---

## Example Usage

### Basic Operations

```bash
SecureSysCLI@admin:~$ hello
Hello from SecureSysCLI! 👋

SecureSysCLI@admin:~$ list
rwxr-xr-x file1.txt
rw-r--r-- file2.txt

SecureSysCLI@admin:~$ create test.txt
Created file: test.txt

SecureSysCLI@admin:~$ write test.txt Hello World
Wrote to file: test.txt

SecureSysCLI@admin:~$ show test.txt
Hello World

SecureSysCLI@admin:~$ copy test.txt backup.txt
Copied test.txt -> backup.txt
```

### Process Management

```bash
SecureSysCLI@admin:~$ run sleep 10 &
[bg] 12345 started: sleep

SecureSysCLI@admin:~$ pslist
[0] PID 12345 running sleep

SecureSysCLI@admin:~$ fgproc 0
Bringing job 0 (PID 12345) to foreground...
Process exited with status 0
```

### Cryptography

```bash
SecureSysCLI@admin:~$ encrypt secret.txt secret.enc
Enter password: 
Encrypted secret.txt -> secret.enc

SecureSysCLI@admin:~$ checksum secret.txt
a1b2c3d4e5f6...  secret.txt
```

### Scripting

```bash
SecureSysCLI@admin:~$ cat setup.cli
# setup.cli
set DIR /tmp
set FILE test.txt
create $FILE
list $DIR
echo Setup complete!

SecureSysCLI@admin:~$ source setup.cli
Executing script: setup.cli
Created file: test.txt
rwxr-xr-x test.txt
Setup complete!
Script execution completed.
```

### Dashboard

```bash
SecureSysCLI@admin:~$ dashboard
# Launches full-screen dashboard
# Press 'q' to quit, 'r' to refresh
```

---

## Notes

- **Unused Files**: `cli.c` and `project.c` are old duplicate versions and not used in the build
- **Build Artifacts**: `*.o` files and `project` executable should not be committed (use `.gitignore`)
- **Security**: This is a demonstration project. For production use, additional security hardening is recommended
- **Privileges**: Running the TLS server on privileged ports may require elevated permissions

---

## License

This project is provided as-is for educational and demonstration purposes.
