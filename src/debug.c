#include "bobbin-internal.h"

#include <stdio.h>
#include <stdlib.h>

char linebuf[256];

void debugger(void)
{
    util_print_state();
    putchar('>');
    fflush(stdout);
    fgets(linebuf, sizeof linebuf, stdin);
    if (linebuf[0] == 'q')
        exit(0);
}
