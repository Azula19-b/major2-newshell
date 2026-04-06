#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

// Main Shell Functions/Runners
int batchMode(char **argv);
int interactiveMode();

// Built-In Command + Alias (Modify Functions as Needed)
int cd();
int exit();
int path();
int myhistory();
int alias();

//Redirection, Pipelining & Signal Control Definitions
