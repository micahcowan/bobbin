#include "bobbin-internal.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

char linebuf[256];

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
    else debugging_flag = true;
    if (STREQ(cfg.interface, "tty")) return; // for now
    sigint_received = 0;

    if (print_message) {
        print_message = false;

        // Tell interfaces to chill, we're handling things
        iface_unhook();

        fprintf(stdout, "\n\n*** Welcome to Bobbin Debugger ***\n");
        fprintf(stdout, "  SPC = next intr, c = leave debugger"
                " (continue execution), m = Apple II monitor\n"
                "  q = quit bobbin, r or w = warm reset, rr = cold reset\n"
                "-----\n");
    }
    bool handled = false;
    while(!handled) {
        util_print_state(stdout, current_pc(), &theCpu.regs);
        fputc('>', stdout);
        fflush(stdout);
        errno = 0;
        char *s = fgets(linebuf, sizeof linebuf, stdin);
        int err = errno;
        if (s == NULL) {
            fputc('\n', stdout);
            if (feof(stdin)) {
                DIE(1,"\nEOF.\n");
            } else if (err == EINTR) {
                fputs("(Interrupt received. \"q\" to exit.)\n", stdout);
                sigint_received = 0;
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
        handled = command_do(linebuf, printf);
        debugging_flag = !handled;

        if (handled)
            break;

        // not yet handled by non-debugger commands...
        handled = true; // ...but for now assume it will have been
        if (HAVE("") || HAVE(" ")) {
            // Do nothing; execute the instruction and return here
            //  on the next one.
        } else if (HAVE("c")) {
            fputs("Continuing...\n", stdout);
            debugging_flag = false;
        } else {
            handled = false; // Loop back around / try again
            fputs("Unrecognized command!\n", stdout);
        }
#undef HAVE
    }

    if (!debugging_flag)
        iface_rehook(); // Interface can take over again.
}
