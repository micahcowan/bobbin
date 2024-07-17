//  bobbin.c
//
//  Copyright (c) 2023 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
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
    periph_init();
    mem_init(); // Loads ROM files. Nothing past this point
                // should be validating options or arguments.
    setup_watches();
    interfaces_start();
    struct timing_t *timing = timing_init();

    event_fire(EV_RESET);

    if (cfg.start_loc_set && !cfg.delay_set) {
        PC = cfg.start_loc;
    }

    for (;;) /* ever */ {
        if (!cfg.turbo) {
            timing_adjust(timing);
        }
        if (check_watches()) frame_count = 0;
        cycle_count = 0;
        do {
            // Provide hooks the opportunity to alter the PC, here
            do {
                current_pc_val = PC;
                event_fire(EV_PRESTEP);
                debugger();
            } while (current_pc_val != PC); // dbgr is allowed to change pc, too
                                            // and since it was  user-requested,
                                            // doesn't get count-limited.

            event_fire(EV_STEP);
            cpu_step();
        } while (cycle_count < CYCLES_PER_FRAME);
        frame_count += cycle_count / CYCLES_PER_FRAME;
        text_flash = frame_count % 30 >= 15;
        event_fire(EV_FRAME);
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
