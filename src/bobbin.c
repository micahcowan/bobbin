#include "bobbin-internal.h"

#include <stdio.h>
#include <stdlib.h>

extern void machine_init(void);
extern void signals_init(void);

word current_pc_val;
word current_pc(void) {
    return current_pc_val;
}

void bobbin_run(void)
{
    signals_init();
    machine_init();
    interfaces_init();
    mem_init(); // Loads ROM files. Nothing past this point
                // should be validating options or arguments.
    interfaces_start();

    cpu_reset();

    for (;;) /* ever */ {
        // Provide hooks the opportunity to alter the PC, here
        unsigned int count = 0;
        const unsigned int max_count = 100;
        do {
            if (count++ >= max_count) {
                DIE(1,"Hooks changed PC %u times!\n", max_count);
            }
            current_pc_val = PC;
            rh_prestep();
        } while (current_pc_val != PC); // changed? loop back around!
        // Check to run debugger, here
        rh_step();
        cpu_step();
    }
}
