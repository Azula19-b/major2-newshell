#include "main.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void close_pipes(int pipes[][2], int pipe_count) {
    for (int index = 0; index < pipe_count; ++index) {
        close(pipes[index][0]);
        close(pipes[index][1]);
    }
}

int execute_pipeline(ShellState *state, char **tokens, int token_count) {
    int starts[NEWSHELL_PIPELINE_LIMIT];
    int ends[NEWSHELL_PIPELINE_LIMIT];
    int command_count = 0;
    int start = 0;

    for (int index = 0; index <= token_count; ++index) {
        if (index == token_count || strcmp(tokens[index], "|") == 0) {
            if (index == start) {
                fprintf(stderr, "newshell: empty pipeline command\n");
                return 1;
            }
            if (command_count == NEWSHELL_PIPELINE_LIMIT) {
                fprintf(stderr, "newshell: pipeline limit is %d commands\n",
                        NEWSHELL_PIPELINE_LIMIT);
                return 1;
            }
            starts[command_count] = start;
            ends[command_count] = index;
            ++command_count;
            start = index + 1;
        }
    }

    char **commands[NEWSHELL_PIPELINE_LIMIT] = {0};
    Redirection redirections[NEWSHELL_PIPELINE_LIMIT];
    for (int index = 0; index < command_count; ++index) {
        if (prepare_command(tokens, starts[index], ends[index], &commands[index],
                            &redirections[index]) != 0) {
            for (int previous = 0; previous < index; ++previous) {
                free(commands[previous]);
            }
            return 1;
        }
    }

    int pipes[NEWSHELL_PIPELINE_LIMIT - 1][2];
    const int pipe_count = command_count - 1;
    for (int index = 0; index < pipe_count; ++index) {
        if (pipe(pipes[index]) != 0) {
            perror("newshell: pipe");
            for (int previous = 0; previous < index; ++previous) {
                close(pipes[previous][0]);
                close(pipes[previous][1]);
            }
            for (int command = 0; command < command_count; ++command) {
                free(commands[command]);
            }
            return 1;
        }
    }

    fflush(NULL);
    pid_t children[NEWSHELL_PIPELINE_LIMIT];
    int started = 0;
    for (int index = 0; index < command_count; ++index) {
        pid_t child = fork();
        if (child < 0) {
            perror("newshell: fork");
            break;
        }
        if (child == 0) {
            restore_child_signals();
            if (index > 0 && dup2(pipes[index - 1][0], STDIN_FILENO) < 0) {
                perror("newshell: pipeline input");
                _exit(1);
            }
            if (index < command_count - 1 && dup2(pipes[index][1], STDOUT_FILENO) < 0) {
                perror("newshell: pipeline output");
                _exit(1);
            }
            close_pipes(pipes, pipe_count);
            if (apply_redirections(&redirections[index]) != 0) {
                _exit(1);
            }
            if (is_builtin(commands[index][0])) {
                int argument_count = 0;
                while (commands[index][argument_count] != NULL) {
                    ++argument_count;
                }
                int status = execute_builtin(state, commands[index], argument_count);
                fflush(NULL);
                _exit(status);
            }
            execvp(commands[index][0], commands[index]);
            fprintf(stderr, "newshell: %s: %s\n", commands[index][0], strerror(errno));
            _exit(127);
        }
        children[started++] = child;
    }

    close_pipes(pipes, pipe_count);
    int last_status = 1;
    for (int index = 0; index < started; ++index) {
        int wait_status = 0;
        while (waitpid(children[index], &wait_status, 0) < 0 && errno == EINTR) {
        }
        if (index == started - 1) {
            if (WIFEXITED(wait_status)) {
                last_status = WEXITSTATUS(wait_status);
            } else if (WIFSIGNALED(wait_status)) {
                last_status = 128 + WTERMSIG(wait_status);
            }
        }
    }
    for (int index = 0; index < command_count; ++index) {
        free(commands[index]);
    }
    return started == command_count ? last_status : 1;
}
