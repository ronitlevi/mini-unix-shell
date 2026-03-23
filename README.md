# Mini Unix Shell (C)

A Unix-like shell implementation in C that supports process execution, pipes, signals, and job control.

This project was developed as part of an Operating Systems course and demonstrates how command interpreters interact with the OS using low-level system calls.

## Overview

This project implements a simplified command-line interpreter (shell) similar to traditional Unix shells.

The shell parses user input, executes commands using fork/exec, manages foreground and background processes, and supports features such as pipes, signals, and command history.

The implementation demonstrates how operating systems manage processes and how user-level programs interact with OS services.

## Features

### Command Execution
- Execute external programs using `fork()` and `execvp()`
- Support for foreground and background execution (`&`)
- Built-in `cd` command

### Process Management
- Track running processes
- Display process list (`procs`)
- Process states: RUNNING / SUSPENDED / TERMINATED

### Signal Handling
- Send signals to processes:
  - `wakeup` (SIGCONT)
  - `suspend` (SIGTSTP)
  - `nuke` (SIGINT)
- Proper signal propagation and handling

### Pipes and Redirection
- Support for single pipe (`|`) between commands
- Input/output redirection (`<`, `>`)
- File descriptor manipulation using `dup()`

### Command History
- Stores recent commands (circular buffer)
- Execute last command (`!!`)
- Execute specific command (`!n`)

## Technologies

- C
- Linux system calls
- fork / exec / waitpid
- signals (kill, SIGINT, SIGTSTP, SIGCONT)
- pipes and file descriptors
- memory management

## How to Run

1. Compile the project:
   ```bash
   make myshell
   ```

2. Run the shell:
   ```bash
   ./myshell
   ```

3. Example commands:
   ```bash
   ls -l
   sleep 5 &
   ls | wc -l
   cd ..
   procs
   ```

## What I Learned

- How command-line interpreters (shells) work internally
- How processes are created and managed using fork/exec
- How to handle signals and control process execution
- How pipes enable inter-process communication
- How to build interactive systems using low-level OS primitives

## Contribution

This project was developed independently as part of an Operating Systems course.

I designed, implemented, tested, and debugged all components of the system.
