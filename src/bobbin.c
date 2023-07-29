#include "bobbin-internal.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern void machine_init(void);
extern void signals_init(void);

uintmax_t frame_count = 0;

word current_pc_val;
word current_pc(void) {
    return current_pc_val;
}

void bobbin_run(void)
{
    setlocale(LC_ALL, "");

    signals_init();
    machine_init();
    interfaces_init();
    mem_init(); // Loads ROM files. Nothing past this point
                // should be validating options or arguments.
    interfaces_start();

    cpu_reset();

    for (;;) /* ever */ {
        struct timespec preframe;
        if (!cfg.turbo) {
            clock_gettime(CLOCK_MONOTONIC, &preframe);
        }
        cycle_count = 0;
        do {
            // Provide hooks the opportunity to alter the PC, here
            const unsigned int max_count = 100;

            do {
                unsigned int count = 0;
                do {
                    if (count++ >= max_count) {
                        DIE(1,"Hooks changed PC %u times!\n", max_count);
                    }
                    current_pc_val = PC;
                    rh_prestep();
                } while (current_pc_val != PC); // changed? loop back around!

                debugger();
            } while (current_pc_val != PC); // dbgr is allowed to change pc, too
                                            // and since it was  user-requested,
                                            // doesn't get count-limited.

            rh_step();
            cpu_step();
        } while (cycle_count < CYCLES_PER_FRAME);
        frame_count += cycle_count / CYCLES_PER_FRAME;
        iface_frame(frame_count % 60 >= 30);
        if (!cfg.turbo) {
            struct timespec postframe;
            clock_gettime(CLOCK_MONOTONIC, &postframe);
            long elapsed;
            if (postframe.tv_sec == preframe.tv_sec) {
                elapsed = postframe.tv_nsec - preframe.tv_nsec;
            } else if (postframe.tv_sec == preframe.tv_sec + 1) {
                elapsed = postframe.tv_nsec - preframe.tv_nsec + 1000000000;
            } else {
                elapsed = -1; // we've already surpassed,
                             // no further waiting to do.
            }

            postframe.tv_sec = 0; postframe.tv_nsec = NS_PER_FRAME - elapsed;
            (void) nanosleep(&postframe, NULL);
        }
        cycle_count %= CYCLES_PER_FRAME;
    }
}
