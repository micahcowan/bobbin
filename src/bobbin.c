#include "bobbin-internal.h"

#include <stdio.h>
#include <stdlib.h>

extern void machine_init(void);
extern void signals_init(void);

void bobbin_run(void)
{
    signals_init();
    machine_init();
    interfaces_init();
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
