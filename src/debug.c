#include "bobbin-internal.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

char linebuf[256];

FILE *dbg_inf;
FILE *dbg_outf;

bool debugging_flag = false;
bool print_message = true;

bool debugging(void)
{
    return debugging_flag;
}

void dbg_on(void)
{
    debugging_flag = true;
    print_message = true;
}

void debugger(void)
{
    if (!debugging_flag) return;

    if (print_message) {
        print_message = false;

        // Tell interfaces to chill, we're handling things
        iface_enter_dbg(&dbg_inf, &dbg_outf);

        fprintf(dbg_outf, "\n\n*** Welcome to Bobbin Debugger ***\n");
        fprintf(dbg_outf, "  SPC = next intr, c = leave debugger,"
                " m = Apple II monitor, q = quit bobbin.\n");
    }
    util_print_state(dbg_outf, current_pc(), &theCpu.regs);
    fputc('>', dbg_outf);
    fflush(dbg_outf);
    errno = 0;
    char *s = fgets(linebuf, sizeof linebuf, dbg_inf);
    int err = errno;
    if (s == NULL) {
        if (ferror(dbg_inf)) {
            DIE(1,"\nDebugger line input: %s\n", strerror(err));
        } else {
            DIE(1,"\nEOF.\n");
        }
    }

    switch (linebuf[0]) {
        case '\0':
        case '\n':
        case ' ':
            // Do nothing; execute the instruction and return here
            //  on the next one.
            break;
        case 'c':
            fputs("Continuing...\n", dbg_outf);
            debugging_flag = false;
            break;
#if 0 // WIP
        case 'm':
            // Swap ourselves out for the built-in Apple II system
            // monitor!
            fputs("Switching to monitor...\n", dbg_outf);
            // Behave as if it were a BRK.
            // Push stuff to stack...

            go_to(MON_BREAK);
            debugging_flag = false;
            break;
#endif
        case 'q':
            fputs("Exiting.\n", dbg_outf);
            exit(0);
            break;
    }

    if (!debugging_flag)
        iface_exit_dbg(); // Interface can take over again.
}
