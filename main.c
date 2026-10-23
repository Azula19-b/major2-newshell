#include "main.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *trim_whitespace(char *text) {
    while (isspace((unsigned char)*text)) {
        ++text;
    }
    if (*text == '\0') {
        return text;
    }
    char *end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) {
        --end;
    }
    end[1] = '\0';
    return text;
}

static bool contains_operator(char **arguments, int argument_count) {
    for (int index = 0; index < argument_count; ++index) {
        if (strcmp(arguments[index], "|") == 0 || strcmp(arguments[index], "<") == 0 ||
            strcmp(arguments[index], ">") == 0 || strcmp(arguments[index], ">>") == 0) {
            return true;
        }
    }
    return false;
}

static int execute_tokens_internal(ShellState *state, char **arguments, int argument_count,
                                   int alias_depth) {
    if (argument_count == 0) {
        return 0;
    }

    const char *replacement = alias_lookup(state, arguments[0]);
    if (replacement != NULL) {
        if (alias_depth >= NEWSHELL_ALIAS_LIMIT) {
            fprintf(stderr, "newshell: alias expansion is too deep\n");
            return 1;
        }
        size_t length = strlen(replacement) + 1;
        for (int index = 1; index < argument_count; ++index) {
            length += strlen(arguments[index]) + 1;
        }
        char *expanded = malloc(length + 1);
        if (expanded == NULL) {
            perror("newshell: malloc");
            return 1;
        }
        strcpy(expanded, replacement);
        for (int index = 1; index < argument_count; ++index) {
            strcat(expanded, " ");
            strcat(expanded, arguments[index]);
        }

        char **expanded_arguments = NULL;
        int expanded_count = 0;
        int status = tokenize(expanded, &expanded_arguments, &expanded_count);
        free(expanded);
        if (status != 0) {
            return status;
        }
        status = execute_tokens_internal(state, expanded_arguments, expanded_count,
                                         alias_depth + 1);
        free_tokens(expanded_arguments);
        return status;
    }

    if (is_builtin(arguments[0]) && !contains_operator(arguments, argument_count)) {
        return execute_builtin(state, arguments, argument_count);
    }
    if (is_builtin(arguments[0]) && contains_operator(arguments, argument_count)) {
        fprintf(stderr, "newshell: built-in commands cannot be piped or redirected\n");
        return 1;
    }
    return execute_pipeline(state, arguments, argument_count);
}

static int execute_tokens(ShellState *state, char **arguments, int argument_count) {
    return execute_tokens_internal(state, arguments, argument_count, 0);
}

static int append_token(char **arguments, int *count, const char *token) {
    arguments[*count] = strdup(token);
    if (arguments[*count] == NULL) {
        perror("newshell: strdup");
        return 1;
    }
    ++(*count);
    arguments[*count] = NULL;
    return 0;
}

int tokenize(const char *line, char ***arguments, int *argument_count) {
    const size_t length = strlen(line);
    char **result = calloc(length + 2, sizeof(char *));
    char *token = malloc(length + 1);
    if (result == NULL || token == NULL) {
        perror("newshell: allocation");
        free(result);
        free(token);
        return 1;
    }

    int count = 0;
    size_t token_length = 0;
    bool token_started = false;
    bool single_quote = false;
    bool double_quote = false;

    for (size_t index = 0; index <= length; ++index) {
        const char character = line[index];
        if (character == '\0') {
            if (single_quote || double_quote) {
                fprintf(stderr, "newshell: unmatched quote\n");
                free(token);
                free_tokens(result);
                return 1;
            }
            if (token_started) {
                token[token_length] = '\0';
                if (append_token(result, &count, token) != 0) {
                    free(token);
                    free_tokens(result);
                    return 1;
                }
            }
            break;
        }

        if (character == '\\' && !single_quote) {
            if (index + 1 < length) {
                token[token_length++] = line[++index];
                token_started = true;
            } else {
                token[token_length++] = character;
                token_started = true;
            }
            continue;
        }
        if (character == '\'' && !double_quote) {
            single_quote = !single_quote;
            token_started = true;
            continue;
        }
        if (character == '"' && !single_quote) {
            double_quote = !double_quote;
            token_started = true;
            continue;
        }
        if (!single_quote && !double_quote && isspace((unsigned char)character)) {
            if (token_started) {
                token[token_length] = '\0';
                if (append_token(result, &count, token) != 0) {
                    free(token);
                    free_tokens(result);
                    return 1;
                }
                token_length = 0;
                token_started = false;
            }
            continue;
        }
        if (!single_quote && !double_quote &&
            (character == '|' || character == '<' || character == '>')) {
            if (token_started) {
                token[token_length] = '\0';
                if (append_token(result, &count, token) != 0) {
                    free(token);
                    free_tokens(result);
                    return 1;
                }
                token_length = 0;
                token_started = false;
            }
            char operator_text[3] = {character, '\0', '\0'};
            if (character == '>' && index + 1 < length && line[index + 1] == '>') {
                operator_text[1] = '>';
                ++index;
            }
            if (append_token(result, &count, operator_text) != 0) {
                free(token);
                free_tokens(result);
                return 1;
            }
            continue;
        }
        token[token_length++] = character;
        token_started = true;
    }

    free(token);
    *arguments = result;
    *argument_count = count;
    return 0;
}

void free_tokens(char **arguments) {
    if (arguments == NULL) {
        return;
    }
    for (size_t index = 0; arguments[index] != NULL; ++index) {
        free(arguments[index]);
    }
    free(arguments);
}

int process_line(ShellState *state, const char *line, bool record_history) {
    char *copy = strdup(line);
    if (copy == NULL) {
        perror("newshell: strdup");
        return 1;
    }
    char *whole_line = trim_whitespace(copy);
    if (*whole_line == '\0') {
        free(copy);
        return 0;
    }
    if (record_history) {
        history_add(state, whole_line);
    }

    int status = 0;
    char *segment_start = whole_line;
    bool single_quote = false;
    bool double_quote = false;
    bool escaped = false;

    for (char *cursor = whole_line;; ++cursor) {
        const char character = *cursor;
        const bool was_escaped = escaped;
        if (escaped) {
            escaped = false;
        } else if (character == '\\' && !single_quote) {
            escaped = true;
        } else if (character == '\'' && !double_quote) {
            single_quote = !single_quote;
        } else if (character == '"' && !single_quote) {
            double_quote = !double_quote;
        }

        if ((character == ';' && !single_quote && !double_quote && !was_escaped) ||
            character == '\0') {
            if (character == ';') {
                *cursor = '\0';
            }
            char *segment = trim_whitespace(segment_start);
            if (*segment != '\0') {
                char **arguments = NULL;
                int argument_count = 0;
                status = tokenize(segment, &arguments, &argument_count);
                if (status == 0) {
                    status = execute_tokens(state, arguments, argument_count);
                    state->exit_status = status;
                }
                free_tokens(arguments);
            }
            if (character == '\0' || !state->running) {
                break;
            }
            segment_start = cursor + 1;
        }
    }
    free(copy);
    return status;
}

int run_batch(ShellState *state, const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "newshell: cannot open '%s': %s\n", path, strerror(errno));
        return 1;
    }
    char *line = NULL;
    size_t capacity = 0;
    int status = 0;
    while (state->running && getline(&line, &capacity, file) != -1) {
        status = process_line(state, line, true);
    }
    if (ferror(file)) {
        fprintf(stderr, "newshell: failed while reading '%s'\n", path);
        status = 1;
    }
    free(line);
    fclose(file);
    return state->running ? status : state->exit_status;
}

int run_interactive(ShellState *state) {
    char *line = NULL;
    size_t capacity = 0;
    while (state->running) {
        char *current_directory = getcwd(NULL, 0);
        printf("newshell:%s> ", current_directory != NULL ? current_directory : "?");
        free(current_directory);
        fflush(stdout);

        if (getline(&line, &capacity, stdin) == -1) {
            if (ferror(stdin)) {
                perror("newshell: input");
                free(line);
                return 1;
            }
            putchar('\n');
            break;
        }
        process_line(state, line, true);
    }
    free(line);
    return state->exit_status;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [batch-file]\n", argv[0]);
        return 2;
    }

    ShellState state;
    shell_state_init(&state);
    install_shell_signals();
    int status = argc == 2 ? run_batch(&state, argv[1]) : run_interactive(&state);
    shell_state_destroy(&state);
    return status;
}
