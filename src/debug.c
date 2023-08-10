#include "bobbin-internal.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Breakpoint Breakpoint;
struct Breakpoint {
    word loc;
    bool enabled;
    Breakpoint *next;
};

Breakpoint *bp_head = NULL;

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

void breakpoint_set(word loc)
{
    Breakpoint *bp = xalloc(sizeof *bp);

    bp->loc = loc;
    bp->enabled = true;
    bp->next = NULL;

    // Find tail of chain
    if (bp_head == NULL) {
        bp_head = bp;
    } else {
        Breakpoint *tail;
        for (tail = bp_head; tail->next != NULL; tail = tail->next) {}
        tail->next = bp;
    }
}

static bool bp_reached(void)
{
    Breakpoint *bp;
    word pc = current_pc();
    for (bp = bp_head; bp != NULL; bp = bp->next) {
        if (pc == bp->loc) return true;
    }
    return false;
}

void debugger(void)
{
    if (STREQ(cfg.interface, "tty")) return; // for now

    if (debugging_flag) {
        // Nothing. Proceed.
    } else {
        if (sigint_received >= 2) {
            fputs("Debug entered via ^C^C.\n", stdout);
        } else if (bp_reached()) {
            printf("Breakpoint at %04X.\n", current_pc());
        } else {
            return;
        }
        event_fire(EV_UNHOOK);
        debugging_flag = true;
    }

    sigint_received = 0;

    if (print_message) {
        print_message = false;

        // Tell interfaces to chill, we're handling things
        event_fire(EV_UNHOOK);

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
        } else if (linebuf[0] == 'b' && linebuf[1] == ' ') {
            // FIXME: very crude.
            unsigned long bploc = strtoul(&linebuf[2], NULL, 16);
            breakpoint_set(bploc);
            handled = false;
        } else {
            handled = false; // Loop back around / try again
            fputs("Unrecognized command!\n", stdout);
        }
#undef HAVE
    }

    if (!debugging_flag)
        event_fire(EV_REHOOK); // Interface can take over again.
}
