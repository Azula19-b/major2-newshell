#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

// Main Shell Functions/Runners
int batchMode(char *argv);
int interactiveMode();

// Built-In Command + Alias (Modify Functions as Needed)
int cdCMD();
int exitCMD();
int pathCMD();
int myhistoryCMD();
int aliasCMD();

//Redirection, Pipelining & Signal Control Definitions
void handleSIGINT();