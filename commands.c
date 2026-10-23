#include "main.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void shell_state_init(ShellState *state) {
    memset(state, 0, sizeof(*state));
    state->running = true;
}

void shell_state_destroy(ShellState *state) {
    for (size_t index = 0; index < state->history_count; ++index) {
        free(state->history[index]);
    }
    for (size_t index = 0; index < state->alias_count; ++index) {
        free(state->aliases[index].name);
        free(state->aliases[index].value);
    }
}

void history_add(ShellState *state, const char *line) {
    if (state->history_count == NEWSHELL_HISTORY_LIMIT) {
        free(state->history[0]);
        memmove(&state->history[0], &state->history[1],
                (NEWSHELL_HISTORY_LIMIT - 1) * sizeof(state->history[0]));
        --state->history_count;
    }
    state->history[state->history_count] = strdup(line);
    if (state->history[state->history_count] == NULL) {
        perror("newshell: history");
        return;
    }
    ++state->history_count;
}

static void history_clear(ShellState *state) {
    for (size_t index = 0; index < state->history_count; ++index) {
        free(state->history[index]);
        state->history[index] = NULL;
    }
    state->history_count = 0;
}

static int builtin_cd(char **arguments, int argument_count) {
    if (argument_count > 2) {
        fprintf(stderr, "cd: too many arguments\n");
        return 1;
    }
    const char *destination = argument_count == 2 ? arguments[1] : getenv("HOME");
    if (destination == NULL) {
        fprintf(stderr, "cd: HOME is not set\n");
        return 1;
    }
    if (chdir(destination) != 0) {
        fprintf(stderr, "cd: %s: %s\n", destination, strerror(errno));
        return 1;
    }
    return 0;
}

static int builtin_exit(ShellState *state, char **arguments, int argument_count) {
    if (argument_count > 2) {
        fprintf(stderr, "exit: too many arguments\n");
        return 1;
    }
    int status = state->exit_status;
    if (argument_count == 2) {
        char *end = NULL;
        errno = 0;
        long parsed = strtol(arguments[1], &end, 10);
        if (errno != 0 || *arguments[1] == '\0' || *end != '\0' || parsed < 0 || parsed > 255) {
            fprintf(stderr, "exit: status must be an integer from 0 to 255\n");
            return 1;
        }
        status = (int)parsed;
    }
    state->exit_status = status;
    state->running = false;
    return status;
}

static int builtin_path(char **arguments, int argument_count) {
    if (argument_count == 1) {
        const char *path = getenv("PATH");
        printf("%s\n", path != NULL ? path : "");
        return 0;
    }

    size_t length = 1;
    for (int index = 1; index < argument_count; ++index) {
        length += strlen(arguments[index]) + 1;
    }
    char *path = calloc(length, 1);
    if (path == NULL) {
        perror("path: allocation");
        return 1;
    }
    for (int index = 1; index < argument_count; ++index) {
        if (index > 1) {
            strcat(path, ":");
        }
        strcat(path, arguments[index]);
    }
    int status = 0;
    if (setenv("PATH", path, 1) != 0) {
        perror("path: setenv");
        status = 1;
    }
    free(path);
    return status;
}

static bool valid_alias_name(const char *name) {
    if (*name == '\0' || (!isalpha((unsigned char)*name) && *name != '_')) {
        return false;
    }
    for (const char *cursor = name + 1; *cursor != '\0'; ++cursor) {
        if (!isalnum((unsigned char)*cursor) && *cursor != '_') {
            return false;
        }
    }
    return true;
}

const char *alias_lookup(const ShellState *state, const char *name) {
    for (size_t index = 0; index < state->alias_count; ++index) {
        if (strcmp(state->aliases[index].name, name) == 0) {
            return state->aliases[index].value;
        }
    }
    return NULL;
}

static int builtin_alias(ShellState *state, char **arguments, int argument_count) {
    if (argument_count == 1) {
        for (size_t index = 0; index < state->alias_count; ++index) {
            printf("alias %s='%s'\n", state->aliases[index].name,
                   state->aliases[index].value);
        }
        return 0;
    }
    if (argument_count == 2) {
        const char *value = alias_lookup(state, arguments[1]);
        if (value == NULL) {
            fprintf(stderr, "alias: %s: not found\n", arguments[1]);
            return 1;
        }
        printf("alias %s='%s'\n", arguments[1], value);
        return 0;
    }
    if (!valid_alias_name(arguments[1])) {
        fprintf(stderr, "alias: invalid name '%s'\n", arguments[1]);
        return 1;
    }

    size_t value_length = 1;
    for (int index = 2; index < argument_count; ++index) {
        value_length += strlen(arguments[index]) + 1;
    }
    char *value = calloc(value_length, 1);
    if (value == NULL) {
        perror("alias: allocation");
        return 1;
    }
    for (int index = 2; index < argument_count; ++index) {
        if (index > 2) {
            strcat(value, " ");
        }
        strcat(value, arguments[index]);
    }

    for (size_t index = 0; index < state->alias_count; ++index) {
        if (strcmp(state->aliases[index].name, arguments[1]) == 0) {
            free(state->aliases[index].value);
            state->aliases[index].value = value;
            return 0;
        }
    }
    if (state->alias_count == NEWSHELL_ALIAS_LIMIT) {
        fprintf(stderr, "alias: limit of %d aliases reached\n", NEWSHELL_ALIAS_LIMIT);
        free(value);
        return 1;
    }
    Alias *alias = &state->aliases[state->alias_count];
    alias->name = strdup(arguments[1]);
    alias->value = value;
    if (alias->name == NULL) {
        perror("alias: allocation");
        free(value);
        alias->value = NULL;
        return 1;
    }
    ++state->alias_count;
    return 0;
}

static int builtin_unalias(ShellState *state, char **arguments, int argument_count) {
    if (argument_count != 2) {
        fprintf(stderr, "Usage: unalias name\n");
        return 1;
    }
    for (size_t index = 0; index < state->alias_count; ++index) {
        if (strcmp(state->aliases[index].name, arguments[1]) == 0) {
            free(state->aliases[index].name);
            free(state->aliases[index].value);
            memmove(&state->aliases[index], &state->aliases[index + 1],
                    (state->alias_count - index - 1) * sizeof(state->aliases[0]));
            --state->alias_count;
            memset(&state->aliases[state->alias_count], 0, sizeof(state->aliases[0]));
            return 0;
        }
    }
    fprintf(stderr, "unalias: %s: not found\n", arguments[1]);
    return 1;
}

static int builtin_history(ShellState *state, char **arguments, int argument_count) {
    if (argument_count == 1) {
        for (size_t index = 0; index < state->history_count; ++index) {
            printf("%2zu  %s\n", index + 1, state->history[index]);
        }
        return 0;
    }
    if (argument_count == 2 && strcmp(arguments[1], "-c") == 0) {
        history_clear(state);
        return 0;
    }
    if (argument_count != 2) {
        fprintf(stderr, "Usage: myhistory [index|-c]\n");
        return 1;
    }

    char *end = NULL;
    long selected = strtol(arguments[1], &end, 10);
    if (*arguments[1] == '\0' || *end != '\0' || selected < 1 ||
        (size_t)selected > state->history_count) {
        fprintf(stderr, "myhistory: invalid history index\n");
        return 1;
    }
    char *command = strdup(state->history[(size_t)selected - 1]);
    if (command == NULL) {
        perror("myhistory: allocation");
        return 1;
    }
    int status = process_line(state, command, false);
    free(command);
    return status;
}

bool is_builtin(const char *name) {
    return strcmp(name, "cd") == 0 || strcmp(name, "exit") == 0 ||
           strcmp(name, "path") == 0 || strcmp(name, "alias") == 0 ||
           strcmp(name, "unalias") == 0 || strcmp(name, "myhistory") == 0;
}

int execute_builtin(ShellState *state, char **arguments, int argument_count) {
    if (strcmp(arguments[0], "cd") == 0) {
        return builtin_cd(arguments, argument_count);
    }
    if (strcmp(arguments[0], "exit") == 0) {
        return builtin_exit(state, arguments, argument_count);
    }
    if (strcmp(arguments[0], "path") == 0) {
        return builtin_path(arguments, argument_count);
    }
    if (strcmp(arguments[0], "alias") == 0) {
        return builtin_alias(state, arguments, argument_count);
    }
    if (strcmp(arguments[0], "unalias") == 0) {
        return builtin_unalias(state, arguments, argument_count);
    }
    if (strcmp(arguments[0], "myhistory") == 0) {
        return builtin_history(state, arguments, argument_count);
    }
    return 1;
}
