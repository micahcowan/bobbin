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
    mem_init(); // Loads ROM files. Nothing past this point
                // should be validating options or arguments.
    interfaces_start();

    cpu_reset();
    if (bobbin_test) {
        PC = 0x400;
    }

    for (;;) /* ever */ {
        trace_instr();
        cpu_step();
    }
}
