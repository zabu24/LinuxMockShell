msh – the Mini Shell

msh is a lightweight, educational shell implementation written in C. It was built as part of an academic project to explore how shells work under the hood, including parsing, pipelines, job control, and file redirection. While simplified compared to a full-featured shell like Bash, msh supports core functionality such as foreground/background execution, I/O redirection, and built-in commands.

Features

Command Parsing

Supports sequences (;), pipelines (|), backgrounding (&), and file redirection (>, >>, 2>, 2>>).

Parses commands into structured data (sequences, pipelines, commands, arguments).

Command Execution

Executes external programs with arguments using fork, exec, and pipes.

Supports multiple commands in a pipeline.

Built-in commands:

cd – change directory (supports relative, absolute, and ~).

exit – terminate the shell.

Job Control

Foreground and background execution.

Signal handling for Ctrl-C (SIGINT) and Ctrl-Z (SIGTSTP).

Basic jobs and fg commands to manage background tasks.

File Redirection

Redirect standard output and error to files.

Append (>>) and overwrite (>) supported.

Example Usage
# Single command
> ls -l

# Pipelined commands
> ls -1 | grep msh

# Sequences
> ls ; cd .. ; pwd

# Background tasks
> sleep 10 &

# Redirection
> echo "Hello" > output.txt
> cat missingfile 2> error.log

Project Design

The project is structured into incremental milestones, each adding more shell functionality:

Parsing – Input parsing into pipelines, commands, and arguments.

Execution – Running commands, pipelines, and built-in commands.

Job Control – Foreground/background execution, signal handling, and job management.

Redirection – Redirecting standard output and error to files.

Why This Project Matters

This project provides hands-on experience with:

Low-level UNIX system calls (fork, exec, waitpid, pipe, dup2, kill, sigaction).

Process management and inter-process communication.

Signal handling and job control.

Command parsing and shell design principles.

It demonstrates how shells are built from the ground up, translating user input into running processes with controlled input/output streams.
