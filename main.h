#ifndef NEWSHELL_MAIN_H
#define NEWSHELL_MAIN_H

#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stddef.h>

#define NEWSHELL_HISTORY_LIMIT 20
#define NEWSHELL_ALIAS_LIMIT 20
#define NEWSHELL_PIPELINE_LIMIT 8

typedef struct {
    char *name;
    char *value;
} Alias;

typedef struct {
    char *history[NEWSHELL_HISTORY_LIMIT];
    size_t history_count;
    Alias aliases[NEWSHELL_ALIAS_LIMIT];
    size_t alias_count;
    bool running;
    int exit_status;
} ShellState;

typedef struct {
    const char *input_path;
    const char *output_path;
    bool append;
} Redirection;

void shell_state_init(ShellState *state);
void shell_state_destroy(ShellState *state);
int run_batch(ShellState *state, const char *path);
int run_interactive(ShellState *state);
int process_line(ShellState *state, const char *line, bool record_history);

int tokenize(const char *line, char ***arguments, int *argument_count);
void free_tokens(char **arguments);

void history_add(ShellState *state, const char *line);
bool is_builtin(const char *name);
int execute_builtin(ShellState *state, char **arguments, int argument_count);
const char *alias_lookup(const ShellState *state, const char *name);

int execute_pipeline(ShellState *state, char **tokens, int token_count);
int prepare_command(char **tokens, int start, int end, char ***arguments,
                    Redirection *redirection);
int apply_redirections(const Redirection *redirection);

void install_shell_signals(void);
void restore_child_signals(void);

#endif
