//  signal.c
//
//  Copyright (c) 2023 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

volatile sig_atomic_t sigint_received = 0;
volatile sig_atomic_t sigwinch_received = 0;
volatile sig_atomic_t sigalrm_received = 0;

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

void handle_alarm(int s)
{
    sigalrm_received = true;
    signal(SIGALRM, handle_alarm);
}

void signals_init(void)
{
    signal(SIGINT, handle_int);
    signal(SIGWINCH, handle_winch);
    signal(SIGALRM, handle_alarm);
}

void unhandle_sigint(void)
{
    signal(SIGINT, SIG_DFL);
}
