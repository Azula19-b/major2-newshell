#include "main.h"
/*
 * Built-in cd command - Bahar
 * Changes the current working directory of the shell.
 */
int cdCMD(char **argv, int argc) {
    char *targetPath = NULL;

    // If no argument → go to HOME directory
    if (argc == 1) {
        targetPath = getenv("HOME"); // Get HOME environment variable

        // Check if HOME exists
        if (targetPath == NULL) {
            fprintf(stderr, "cd: HOME is not set\n");
            return -1;
        }
    }
    // If one argument, use given path
    else if (argc == 2) {
        targetPath = argv[1];
    }
    // More than one argument, error
    else {
        fprintf(stderr, "cd: too many arguments\n");
        return -1;
    }

    // Change current directory
    if (chdir(targetPath) != 0) {
        perror("cd"); // Print system error
        return -1;
    }

    return 0; // Success
}
