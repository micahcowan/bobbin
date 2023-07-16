#include "bobbin-internal.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

sig_atomic_t sigterm_received = 0;

void handle_term(int s)
{
    if (++sigterm_received >= 3)
    {
        _Exit(s);
    }
    signal(SIGINT, handle_term);
}

void signals_init(void)
{
    signal(SIGINT, handle_term);
}
