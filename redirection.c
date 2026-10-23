#include "main.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool is_redirection(const char *token) {
    return strcmp(token, "<") == 0 || strcmp(token, ">") == 0 ||
           strcmp(token, ">>") == 0;
}

int prepare_command(char **tokens, int start, int end, char ***arguments,
                    Redirection *redirection) {
    char **result = calloc((size_t)(end - start) + 1, sizeof(char *));
    if (result == NULL) {
        perror("newshell: allocation");
        return 1;
    }
    memset(redirection, 0, sizeof(*redirection));

    int count = 0;
    for (int index = start; index < end; ++index) {
        if (!is_redirection(tokens[index])) {
            result[count++] = tokens[index];
            continue;
        }
        if (index + 1 >= end || is_redirection(tokens[index + 1]) ||
            strcmp(tokens[index + 1], "|") == 0) {
            fprintf(stderr, "newshell: redirection requires a file name\n");
            free(result);
            return 1;
        }
        if (strcmp(tokens[index], "<") == 0) {
            if (redirection->input_path != NULL) {
                fprintf(stderr, "newshell: multiple input redirections are not supported\n");
                free(result);
                return 1;
            }
            redirection->input_path = tokens[index + 1];
        } else {
            if (redirection->output_path != NULL) {
                fprintf(stderr, "newshell: multiple output redirections are not supported\n");
                free(result);
                return 1;
            }
            redirection->output_path = tokens[index + 1];
            redirection->append = strcmp(tokens[index], ">>") == 0;
        }
        ++index;
    }
    result[count] = NULL;
    if (count == 0) {
        fprintf(stderr, "newshell: missing command\n");
        free(result);
        return 1;
    }
    *arguments = result;
    return 0;
}

int apply_redirections(const Redirection *redirection) {
    if (redirection->input_path != NULL) {
        int input = open(redirection->input_path, O_RDONLY);
        if (input < 0) {
            fprintf(stderr, "newshell: %s: %s\n", redirection->input_path,
                    strerror(errno));
            return 1;
        }
        if (dup2(input, STDIN_FILENO) < 0) {
            perror("newshell: dup2 input");
            close(input);
            return 1;
        }
        close(input);
    }
    if (redirection->output_path != NULL) {
        int flags = O_WRONLY | O_CREAT | (redirection->append ? O_APPEND : O_TRUNC);
        int output = open(redirection->output_path, flags, 0644);
        if (output < 0) {
            fprintf(stderr, "newshell: %s: %s\n", redirection->output_path,
                    strerror(errno));
            return 1;
        }
        if (dup2(output, STDOUT_FILENO) < 0) {
            perror("newshell: dup2 output");
            close(output);
            return 1;
        }
        close(output);
    }
    return 0;
}
