#include "bobbin-internal.h"
#include "iface-simple.h"

#include <stdio.h>
#include <stdlib.h>

extern void signals_init(void);

void bobbin_run(void)
{
    signals_init();
    iface_simple_instr_init();
    mem_init();
    trfile = fopen("trace.log", "w");
    if (trfile == NULL) {
        perror("Couldn't open trace file");
        exit(2);
    }

    cpu_reset();
    if (bobbin_test) {
        PC = 0x400;
    }

    for (;;) /* ever */ {
        trace_instr();
        cpu_step();
    }
}
