#include "bobbin-internal.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <time.h>
#include <unistd.h>

extern void machine_init(void);
extern void signals_init(void);

uintmax_t frame_count = 0;
bool text_flash;

word current_pc_val;
word current_pc(void) {
    return current_pc_val;
}

static void handle_io_opts(void);

void bobbin_run(void)
{
    setlocale(LC_ALL, "");

    signals_init();
    machine_init();
    handle_io_opts();
    events_init();
    interfaces_init();
    mem_init(); // Loads ROM files. Nothing past this point
                // should be validating options or arguments.
    setup_watches();
    interfaces_start();

    event_fire(EV_RESET);

    if (cfg.start_loc_set && !cfg.delay_set) {
        PC = cfg.start_loc;
    }

    for (;;) /* ever */ {
        if (check_watches()) frame_count = 0;
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
                    event_fire(EV_PRESTEP);
                } while (current_pc_val != PC); // changed? loop back around!

                debugger();
            } while (current_pc_val != PC); // dbgr is allowed to change pc, too
                                            // and since it was  user-requested,
                                            // doesn't get count-limited.

            event_fire(EV_STEP);
            cpu_step();
        } while (cycle_count < CYCLES_PER_FRAME);
        frame_count += cycle_count / CYCLES_PER_FRAME;
        text_flash = frame_count % 60 >= 30;
        event_fire(EV_FRAME);
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

static void handle_io_opts(void)
{
    if (cfg.inputfile && !STREQ(cfg.inputfile, "-") && !cfg.detokenize) {
        close(STDIN_FILENO);
        errno = 0;
        int fd = open(cfg.inputfile, O_RDONLY);
        if (fd < 0) {
            DIE(1, "-i: Couldn't open \"%s\": %s\n", cfg.inputfile,
                strerror(errno));
        } else if (fd != STDIN_FILENO) {
            DIE(1, "-i: open failed to use lowest descriptor.\n");
        }
    }

    if (cfg.outputfile && !STREQ(cfg.outputfile, "-")) {
        close(STDOUT_FILENO);
        errno = 0;
        int fd = creat(cfg.outputfile, 0666);
        if (fd < 0) {
            DIE(1, "-o: Couldn't open \"%s\": %s\n", cfg.outputfile,
                strerror(errno));
        } else if (fd != STDOUT_FILENO) {
            DIE(1, "-o: open failed to use lowest descriptor.\n");
        }
    }
}
