#include "bobbin-internal.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

volatile sig_atomic_t sigint_received = 0;
volatile sig_atomic_t sigwinch_received = 0;

void handle_int(int s)
{
    ++sigint_received;
    signal(SIGINT, handle_int);
}

void handle_winch(int s)
{
    sigwinch_received = true;
    signal(SIGWINCH, handle_winch);
}

void signals_init(void)
{
    signal(SIGINT, handle_int);
    signal(SIGWINCH, handle_winch);
}

void unhandle_sigint(void)
{
    signal(SIGINT, SIG_DFL);
}
