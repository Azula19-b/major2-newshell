#include "main.h"
/*
 * Builds a new argument array without the redirection
 * symbol and its file name.
 * Example:
 * ls > out.txt  --->  cleanArgs = {"ls", NULL}
 */
char **buildCleanArgs(char **argv, int argc, int redirectIndex) {
    char **cleanArgs = malloc((argc + 1) * sizeof(char *));
    int i;
    int j = 0;

    // Check memory allocation
    if (cleanArgs == NULL) {
        perror("Failed to allocate memory");
        return NULL;
    }

    for (i = 0; i < argc; i++) {
        // Skip the redirection operator
        if (i == redirectIndex) {
            continue;
        }

        // Skip the file name after the operator
        if (i == redirectIndex + 1) {
            continue;
        }
        
        // Copy normal command arguments
        cleanArgs[j] = argv[i];
        j++;
    }

    // End the new argument list
    cleanArgs[j] = NULL;

    return cleanArgs;
}

/*
 * Executes an external command with one redirection.
 *
 * redirectType:
 * 0 = output redirection (>)
 * 1 = input redirection  (<)
 */
int executeWithRedirection(char **argv, int argc, int redirectIndex, int redirectType) {
    pid_t pid;
    int status = 0;
    int fd;
    char *fileName;
    char **cleanArgs;

    // Make sure a file name exists after < or >
    if (redirectIndex + 1 >= argc) {
        fprintf(stderr, "redirection error: missing file name\n");
        return -1;
    }

    // Built-in commands are not redirected in this project
    if (strcmp(argv[0], "cd") == 0 ||
        strcmp(argv[0], "exit") == 0 ||
        strcmp(argv[0], "path") == 0 ||
        strcmp(argv[0], "myhistory") == 0 ||
        strcmp(argv[0], "alias") == 0) {
        fprintf(stderr, "redirection error: built-in commands do not support redirection\n");
        return -1;
    }

    // Store the target file name
    fileName = argv[redirectIndex + 1];

    // Build a clean argv for execvp()
    cleanArgs = buildCleanArgs(argv, argc, redirectIndex);
    if (cleanArgs == NULL) {
        return -1;
    }

    // Create child process
    pid = fork();

    if (pid < 0) {
        perror("fork");
        free(cleanArgs);
        return -1;
    }

    // Child process
    if (pid == 0) {
        // Child should use default signal behavior
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTERM, SIG_DFL);

        // Handle output redirection: command > file
        if (redirectType == 0) {
            fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open output file");
                exit(EXIT_FAILURE);
            }

            // Replace standard output with file
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2 output");
                close(fd);
                exit(EXIT_FAILURE);
            }
        }
        // Handle input redirection: command < file
        else if (redirectType == 1) {
            fd = open(fileName, O_RDONLY);
            if (fd < 0) {
                perror("open input file");
                exit(EXIT_FAILURE);
            }

            // Replace standard input with file
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("dup2 input");
                close(fd);
                exit(EXIT_FAILURE);
            }
        }
        // Invalid redirection type
        else {
            fprintf(stderr, "redirection error: invalid redirection type\n");
            exit(EXIT_FAILURE);
        }

        // Close original file descriptor after dup2()
        close(fd);

        // Execute the external command
        execvp(cleanArgs[0], cleanArgs);

        // Only runs if execvp() fails
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // Parent waits for child to finish
    waitpid(pid, &status, 0);

    // Free allocated memory in parent
    free(cleanArgs);

    return 0;
}
