#include "bobbin-internal.h"

#include <stdio.h>
#include <stdlib.h>

extern void machine_init(void);
extern void signals_init(void);

void bobbin_run(void)
{
    trfile = fopen("trace.log", "w");
    if (trfile == NULL) {
        perror("Couldn't open trace file");
        exit(2);
    }
    setvbuf(trfile, NULL, _IOLBF, 0);

    signals_init();
    machine_init();
    mem_init();
    interfaces_init();

    cpu_reset();
    if (bobbin_test) {
        PC = 0x400;
    }

    for (;;) /* ever */ {
        trace_instr();
        cpu_step();
    }
}
