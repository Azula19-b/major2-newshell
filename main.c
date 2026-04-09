#include "main.h"

struct lineData {
    char **argv;
    int argc;
};

int main(int argc, char **argv) {
    
    //Signal Handler Setup
    void (*intRet)(int) = signal(SIGINT,handleSignals);
    void (*tstpRet)(int) = signal(SIGTSTP,handleSignals);
    void (*termRet)(int) = signal(SIGTERM,handleSignals);

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
        char** argv = getArgArr(line); //Array of strings not unlike what is given by the main function
        int argc = getArgCount(line); // Argument Count for argv
        printf("Input Line: %s\n", line);
        printf("argc: %i\n", argc);
        for(int i = 0; i < argc; i++) {
            printf("Argument %i: %s\n",i,argv[i]);
        }
        if(argc == 0) 
            break;
    } while(false); // put exit command here
    return EXIT_SUCCESS;
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

    return args;
}

int getArgCount(char* line) {
    int lineLength = strlen(line);
    int ret = 0;
    for(int i = 0; i < lineLength; i++) {
        if(line[i] == ' ' || line[i] == '\n' || line[i] == '\t' || line[i] == '\0') {
            ret++;
        }
    }

    return ret;
}

