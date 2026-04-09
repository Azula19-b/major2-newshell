#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

// Main Shell Functions/Runners
int batchMode(char *argv);
int interactiveMode();
char* getInputLine();
int getArgCount(char **argv);
char** getArgArr(char *inputLine);

void executeCommands(char **argv, int argc);
int* findPipes(char **argv, int argc);
int* findRedirections(char **argv, int argc);

// Built-In Command + Alias (Modify Functions as Needed)
int cdCMD(char **argv, int argc); // Bahar - built-in cd command (changes working directory)
int exitCMD();
int pathCMD();
int myhistoryCMD();
int aliasCMD();

//Redirection, Pipelining & Signal Control Definitions
int executeWithRedirection(char **argv, int argc, int redirectIndex, int redirectType); // Handles < and > redirection for external commands
char **buildCleanArgs(char **argv, int argc, int redirectIndex); // Removes redirection symbols before execvp
void handleSignals(int iSig);
