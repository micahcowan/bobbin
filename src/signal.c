#include "bobbin-internal.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

volatile sig_atomic_t sigint_received = 0;

void handle_int(int s)
{
    ++sigint_received;
    signal(SIGINT, handle_int);
}

void signals_init(void)
{
    signal(SIGINT, handle_int);
}
