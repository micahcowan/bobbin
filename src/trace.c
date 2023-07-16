#include "bobbin-internal.h"
#include "iface-simple.h"

#include <stdio.h>
#include <stdlib.h>

word current_instruction = 0;

// static int traceon = 1;
static int traceon = 0;

void trace_instr(void)
{
    current_instruction = PC;

    if (traceon)
        util_print_state(stderr);

    iface_simple_instr_hook();
}

int trace_mem_get_byte(word loc)
{
    return iface_simple_getb_hook(loc);
}
