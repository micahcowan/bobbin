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
    if (!debugging_flag && sigint_received < 2) return;
    sigint_received = 0;

    if (print_message) {
        print_message = false;

        // Tell interfaces to chill, we're handling things
        iface_enter_dbg(&dbg_inf, &dbg_outf);

        fprintf(dbg_outf, "\n\n*** Welcome to Bobbin Debugger ***\n");
        fprintf(dbg_outf, "  SPC = next intr, c = leave debugger,"
                " m = Apple II monitor, q = quit bobbin.\n-----\n");
    }
    bool unhandled = true;
    while(unhandled) {
        util_print_state(dbg_outf, current_pc(), &theCpu.regs);
        fputc('>', dbg_outf);
        fflush(dbg_outf);
        errno = 0;
        char *s = fgets(linebuf, sizeof linebuf, dbg_inf);
        int err = errno;
        if (s == NULL) {
            fputc('\n', dbg_outf);
            if (feof(dbg_inf)) {
                DIE(1,"\nEOF.\n");
            } else if (err == EINTR) {
                fputs("(Interrupt received. \"q\" to exit.)\n", dbg_outf);
                continue; // loop back around
            } else {
                DIE(1,"\nDebugger line input: %s\n", strerror(err));
            }
        }
        char *nl;
        if ((nl = strchr(linebuf, '\n')) != NULL) {
            *nl = '\0';
        }

#define HAVE(val) STREQ((val), linebuf)
        unhandled = false;
        if (HAVE("") || HAVE(" ")) {
            // Do nothing; execute the instruction and return here
            //  on the next one.
        } else if (HAVE("c")) {
            fputs("Continuing...\n", dbg_outf);
            debugging_flag = false;
        } else if (HAVE("m")) {
            // Swap ourselves out for the built-in Apple II system
            // monitor!
            fputs("Switching to monitor...\n", dbg_outf);
            // Behave as if it were a BRK.
            // Push stuff to stack...
            stack_push_sneaky(HI(PC));
            stack_push_sneaky(LO(PC));
            stack_push_sneaky(PFLAGS | PMASK(PUNUSED) | PMASK(PBRK));
            go_to(WORD(peek_sneaky(VEC_BRK),peek_sneaky(VEC_BRK+1)));
            // ^ Note: some autostart ROMs have
            // OLDRST instead of BREAK, in VEC_BRK, with the result that
            // PC and the other registers will NOT be printed on entry
            // into the system monitor.
            debugging_flag = false;
        } else if (HAVE("r") || HAVE("w")) {
            cpu_reset();
            debugging_flag = false;
        } else if (HAVE("rr")) {
            cpu_reset();
            debugging_flag = false;
            // Hard reset. Invalidate the user reset vector directly
            // (rather than doing open-apple emulation or something)
            // Just copy the high byte of the vector into the
            // "powered up" check; then it can't possibly be the
            // correctly XOR-ed version.
            byte b = peek_sneaky(LOC_SOFTEV+1);
            poke_sneaky(LOC_PWREDUP, b);
        } else if (HAVE("^C")) {
            // XXX in future this will be replaced by a "send" command
            // that can do other things besides just ^C. ^? or ^D for
            // instance
            // Send an interrupt back through to the emulation, and
            // continue.
            sigint_received = 1;
            debugging_flag = false;
        } else if (HAVE("q")) {
            fputs("Exiting.\n", dbg_outf);
            exit(0);
        } else {
            unhandled = true; // Loop back around / try again
            fputs("Unrecognized command!\n", dbg_outf);
        }
#undef HAVE
    }

    if (!debugging_flag)
        iface_exit_dbg(); // Interface can take over again.
}
