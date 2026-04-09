#include "main.h"

struct lineData {
    char **argv;
    int argc;
};

int main(int argc, char **argv) {
    
    //Signal Handler Setup
    signal(SIGINT,handleSignals);
    signal(SIGTSTP,handleSignals);
    signal(SIGTERM,handleSignals);

    if(argc == 2) {
        //run batch mode
        batchMode(argv[1]); //provide batch file path to batchMode function
    }
    else if(argc == 1) {
        //run interactive mode
        interactiveMode();
    }
    else {
        // handle improper input
        printf("usage: newshell [batchFile]\n");
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}

// Main Shell Runners (Batch Mode, Interactive Mode & Command Execution)

int batchMode(char *argv) {
    int batchFile = open(argv, O_RDONLY); //Open Batch File

    if(batchFile < 0) {
        perror("Error: Failed to Open Batch File.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int interactiveMode() {
    do {
        printf("newshell:~> "); //put current file path here eventually
        char* line = getInputLine(); // Eventually pull input line into this variable
        // printf("Input Line: %s\n", line);
        char** argv = getArgArr(line); //Array of strings not unlike what is given by the main function
        int argc = getArgCount(argv); // Argument Count for argv

        // printf("argc: %i\n", argc);

        // for(int i = 0; i < argc; i++) {
        //     printf("Argument %i: %s\n",i,argv[i]);
        // }
        if(argc > 0)
            executeCommands(argv, argc);
        else
            printf("\n");

        if(argc == 0) // End Case until exit command is made
            break;
        
    } while(true); // put exit command here
    return EXIT_SUCCESS;
}

void executeCommands(char **argv, int argc) {
    // Needs to know piping and redirection and figure out where it is
    int *pipeIndecies = findPipes(argv,argc);
    int *redirectionIndecies = findRedirections(argv,argc);

    if(pipeIndecies[0] != -1 && pipeIndecies[1] == -1) {
        // Singular Pipe Found Case
        printf("One Pipe Found at argument: %i\n",pipeIndecies[0]);
    }
    else if(pipeIndecies[0] != -1 && pipeIndecies[1] != -1) {
        // Multi Pipe Case
        printf("Two Pipes Found at %i and %i\n",pipeIndecies[0],pipeIndecies[1]);
    }
    else if(redirectionIndecies[0] != -1) {
        // Bahar - handle I/O redirection for external commands
        executeWithRedirection(argv, argc, redirectionIndecies[0], redirectionIndecies[1]);
    }
    else {
        // Normal Execution Case
        if(strcmp(argv[0], "cd") == 0) {
            // Bahar - built-in cd must run in shell process
            cdCMD(argv, argc);
        }
        else if(strcmp(argv[0],"myhistory") == 0) {
            // Run myhistory command
        }
        else if(strcmp(argv[0],"path") == 0) {
            // Run path command
        }
        else if(strcmp(argv[0],"alias") == 0) {
            // Run alias command
        }
        else if(strcmp(argv[0],"exit") == 0) {
            // Run exit command
        }
        else {
            printf("Invalid command input in argv[0]: %s\n",argv[0]);
            printf("Available builtins: cd, myhistory, path, alias, exit.\n");
        }
    }

    free(pipeIndecies);
    free(redirectionIndecies);
}

// Helper Functions Below

// Outputs index of each pipe
int* findPipes(char **argv, int argc) {
    int *ret = malloc(2 * sizeof(int)); // Only need to handle 2 pipes for project
    if(!ret) {
        perror("Failed to Allocate Memory");
        exit(EXIT_FAILURE);
    }

    ret[0] = ret[1] = -1;
    int pipeIndex = 0;

    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i],"|") == 0) {
            if(pipeIndex >= 2) {
                printf("newshell only supports 2 pipes at any time.\n");
                exit(EXIT_FAILURE);
            }
            ret[pipeIndex] = i;
            pipeIndex++;
        }
    }

    return ret;
}

// Outputs position in ret[0] and type in ret[1]
// Type 0 = >
// Type 1 = <
int* findRedirections(char **argv, int argc) {
    int *ret = malloc(2 * sizeof(int)); // Only one redirect per line
    if(!ret) {
        perror("Failed to Allocate Memory");
        exit(EXIT_FAILURE);
    }

    ret[0] = ret[1] = -1;

    for(int i = 0; i < argc; i++) {
        if((strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0) && (ret[0] != -1 && ret[1] != -1)) { // Handle more than 1 redirect
            printf("newshell only supports one redirect at a time.\n");
            exit(EXIT_FAILURE);
        }
        else if(strcmp(argv[i],">>") == 0) { // Handle Appending Support
            printf("newshell does not support appending.\n");
            exit(EXIT_FAILURE);
        }

        if(strcmp(argv[i], ">") == 0) {
            ret[0] = i;
            ret[1] = 0;
        }
        else if(strcmp(argv[i], "<") == 0) {
            ret[0] = i;
            ret[1] = 1;
        }
    }

    return ret;
}

char* getInputLine() { // Simpler getline implementation
    char *line = NULL;
    size_t bufferSize = 0;

    if(getline(&line, &bufferSize, stdin) == -1) {
        if(feof(stdin)) {
            exit(EXIT_SUCCESS);
        }
        else {
            perror("Line reader error");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

char** getArgArr(char* line) {
    int arrayLength = 10;
    int i = 0;
    char **args = malloc(arrayLength * sizeof(char *));

    if(!args) {
        perror("Failed to Allocate Memory");
        exit(EXIT_FAILURE);
    }

    char *arg = strtok(line," \t\n"); // delimit for whitespaces

    while(arg != NULL) {
        // iterate through all tokens until strtok stops
        args[i] = arg;
        i++;

        if(i >= arrayLength) {
            arrayLength += 10;
            args = realloc(args,arrayLength);

            if(!args) {
                perror("Failed to Allocate Memory");
                exit(EXIT_FAILURE);
            }
        }
        arg = strtok(NULL," \t\n");
    }
    args[i] = NULL;
    return args;
}

int getArgCount(char** argv) {
    int i = 0;
    while(argv[i] != NULL) {
        i++;
    }
    return i;
}

