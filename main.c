#include "main.h"


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
        
    } while(false); // put exit command here
    return EXIT_SUCCESS;
}
