# newshell

`newshell` is a compact Unix-style command interpreter written in C17. It supports interactive and batch execution, external programs, built-in shell commands, command history, aliases, I/O redirection, and pipelines of up to eight commands.

The project was developed as UNT CSCE 3600 Major Assignment 2 and completed as a cohesive systems-programming project. Its implementation focuses on process creation, file descriptors, POSIX signals, dynamic memory, parsing, and defensive error handling without third-party libraries.

## Features

- Interactive prompt displaying the current working directory.
- Batch mode for commands stored in a text file.
- External command execution through `fork()` and `execvp()`.
- Quoted arguments, escaped characters, and multiple commands separated by `;`.
- Pipelines with up to eight stages, such as `cat file | grep word | wc -l`.
- Input redirection with `<`.
- Output replacement with `>` and append mode with `>>`.
- A bounded 20-entry command history with listing, replay, and clearing.
- Runtime aliases with listing, replacement, and removal.
- Shell path inspection and replacement through the `path` built-in.
- `SIGINT` and `SIGTSTP` isolation so terminal signals affect child commands without terminating the shell.
- Non-zero statuses and clear diagnostics for malformed input and operating-system failures.

## Built-in commands

| Command | Behavior |
| --- | --- |
| `cd [directory]` | Changes the shell's working directory. With no argument, uses `HOME`. |
| `exit [status]` | Ends the shell with an optional status from 0 to 255. |
| `path [directory ...]` | Prints `PATH`, or replaces it with the colon-joined directories. |
| `myhistory` | Lists up to the 20 most recent input lines. |
| `myhistory index` | Re-executes a numbered history entry. |
| `myhistory -c` | Clears command history. |
| `alias name command ...` | Creates or replaces an alias. |
| `alias [name]` | Lists every alias or one selected alias. |
| `unalias name` | Removes an alias. |

State-changing built-ins execute in the shell process. They intentionally cannot be piped or redirected because doing so in a child process would not update the parent shell state.

## Project structure

```text
.
├── .github/workflows/ci.yml  # Linux build and integration tests
├── examples/demo.batch       # Example batch-mode script
├── tests/test_newshell.sh    # Portable end-to-end test suite
├── main.c                    # Input loop, tokenizer, dispatch, and batch mode
├── main.h                    # Shared types, constants, and interfaces
├── commands.c                # Built-ins, aliases, and command history
├── pipeline.c                # Process creation, pipes, and wait handling
├── redirection.c             # Redirection parsing and file-descriptor setup
├── signalControl.c           # Parent/child signal configuration
└── Makefile
```

## Requirements

- macOS or Linux
- A POSIX-compatible environment
- Apple Clang or GCC with C17 support
- `make`
- A POSIX shell for the automated tests

## Build

```bash
git clone https://github.com/Azula19-b/major2-newshell.git
cd major2-newshell
make
```

The build uses strict warnings and creates `./newshell`.

## Run

Start interactive mode:

```bash
./newshell
```

Example session:

```text
newshell:/Users/example/project> echo "hello world"
hello world
newshell:/Users/example/project> printf 'alpha\nbeta\nalpha\n' > words.txt
newshell:/Users/example/project> cat words.txt | grep alpha | wc -l
       2
newshell:/Users/example/project> alias greet echo Hello
newshell:/Users/example/project> greet Bahar
Hello Bahar
newshell:/Users/example/project> exit 0
```

Run commands from a batch file:

```bash
./newshell examples/demo.batch
```

Each non-empty batch line is processed exactly as interactive input. Semicolon-separated commands on the same line run from left to right.

## Syntax examples

```bash
# Multiple commands
pwd; echo finished

# Quoting and escaped semicolons
echo "a value with spaces"
echo escaped\;semicolon

# Input and output redirection
sort < unsorted.txt > sorted.txt
echo another-line >> sorted.txt

# Multi-stage pipeline
cat server.log | grep ERROR | wc -l

# History
myhistory
myhistory 3
myhistory -c
```

## Tests

Run the 19 end-to-end checks:

```bash
make test
```

The suite creates an isolated temporary directory and verifies:

- external commands and quoted arguments;
- command lists, quoted semicolons, and escaped semicolons;
- persistent `cd` behavior;
- input, output, and append redirection;
- a three-stage pipeline;
- alias creation, expansion, listing, and removal;
- path and history built-ins;
- history replay and interactive mode;
- missing batch files, parser errors, and exit-status propagation.

Run the same suite with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
make sanitize
```

GitHub Actions runs the standard build and test suite on pushes and pull requests.

## Architecture

The tokenizer converts an input segment into a null-terminated argument vector while preserving quoted text and recognizing pipe and redirection operators even when they touch neighboring words. The line processor separates only unquoted, unescaped semicolons.

Standalone built-ins are dispatched in the parent process so changes to the working directory, environment, aliases, and history persist. Other commands are passed to the pipeline executor. It creates the required pipes, forks one process per stage, connects standard input/output with `dup2()`, applies file redirections, restores default child signal handling, and invokes programs with `execvp()`.

The parent closes its copies of every pipe descriptor and waits for all children. The final pipeline stage determines the returned command status.

## Error handling

`newshell` reports and recovers from common errors including:

- missing batch files;
- unknown external commands;
- unmatched quotation marks;
- empty pipeline stages;
- missing redirection targets;
- duplicate input/output redirections;
- invalid history indexes and exit statuses;
- invalid alias names;
- failed allocation, `fork`, `pipe`, `open`, `dup2`, and `execvp` calls.

## Current limitations

- Environment-variable expansion, wildcard expansion, command substitution, and background jobs are not implemented.
- Aliases expand only when they are the first command token; aliases inside later pipeline stages are not expanded.
- Built-in commands cannot be piped or redirected.
- Job control is intentionally limited to protecting the shell from `SIGINT` and `SIGTSTP`; process groups and foreground terminal ownership are outside the project scope.
- The parser supports single/double quotes and backslash escaping but is not intended to reproduce every POSIX shell grammar rule.

## Team 8

- Michael McFarlin — Team Lead; command execution/history foundation and signal-control work
- Christian Kalenga — path and alias assignment area
- Bahar Ozer — `cd` and I/O-redirection assignment area
- Jeffrey Tei-Muno — exit and pipeline assignment area

The completed repository retains the original team attribution while integrating and hardening the unfinished modules into one tested application.
