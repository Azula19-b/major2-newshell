#include "main.h"

void handleSignals(int iSig) {
    printf("Warning: Use exit to end the shell\n");
    printf("Caught signal: %d\n",iSig);
}