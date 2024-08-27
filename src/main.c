//  main.c
//
//  Copyright (c) 2023-2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <stddef.h> // NULL

extern void do_config(int, char **);

const char *program_name;

int main(int argc, char **argv)
{
    events_init(); // make sure events are initialized before dlypc
    dlypc_init();

    program_name = *argv;
    do_config(argc, argv);

    bobbin_run();

    return 0;
}
