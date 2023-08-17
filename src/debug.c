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

static char linebuf[256];

static bool debugging_flag = false;
static bool print_message = true;
static bool go_until_rts = false;
static byte stack_min;

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
    byte op = peek_sneaky(current_pc());
    if (go_until_rts && SP >= stack_min) {
        go_until_rts = false;
        printf("Returned.\n");
        return true;
    }

    Breakpoint *bp;
    word pc = current_pc();
    for (bp = bp_head; bp != NULL; bp = bp->next) {
        if (pc == bp->loc) {
            printf("Breakpoint at %04X.\n", current_pc());
            return true;
        }
    }
    return false;
}

static inline void preface_read(word loc)
{
    printf("\n%04X:", loc);
}

static void mlcmd_read(word first, word last)
{
    word next = first;
    preface_read(next);
    printf(" %02X", peek_sneaky(next));
    while(++next < last) {
        if (next % 8 == 0) preface_read(next);
        printf(" %02X", peek_sneaky(next));
    }
    printf("\n\n");
}

static void mlcmd_list(word first, word last)
{
    // Note: `last` is ignored, we always print 16 instr
    word next = first;
    putchar('\n');
    for (int i = 0; i != 16; ++i) {
        next = print_disasm(stdout, next, &theCpu.regs);
    }
}

// returns true if command in linebuf was handled.
static bool handle_monitor_like_cmd(void)
{
    unsigned long first, second = 0;
    char *str = linebuf, *end;

    // See if we've got a hex number
    first = strtoul(str, &end, 16);
    if (end == str) return false;
    second = first;
    str = end;

    // See if we got a second one
    if (*str == '.') {
        ++str;
        second = strtoul(str, &end, 16);
        if (end == str) return false; // can't end on "."
        str = end;
    }

    // Is there a command?
    if (STREQCASE(str, "L")) {
        mlcmd_list(first, second);
    } else if (*str == '\0') {
        mlcmd_read(first, second);
    } else {
        // Garbage at end of line: unrecognized command overall.
        return false;
    }

    return true;
}

void debugger(void)
{
    if (STREQ(cfg.interface, "tty")) return; // for now

    if (debugging_flag) {
        go_until_rts = 0;
    } else {
        if (sigint_received >= 2) {
            fputs("Debug entered via ^C^C.\n", stdout);
        } else if (bp_reached()) {
            // Handled
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
    bool loop = true;
    while(loop) {
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
        bool handled = command_do(linebuf, printf);
        if (handled) {
            loop = false;
            debugging_flag = false;
            break;
        }

        // not yet handled by non-debugger commands...
        loop = true;
        if (HAVE("") || HAVE(" ") || HAVE("s")) {
            // Do nothing; execute the instruction and return here
            //  on the next one.
            loop = false;
        } else if (HAVE("c")) {
            fputs("Continuing...\n", stdout);
            loop = debugging_flag = false;
        } else if (linebuf[0] == 'b' && linebuf[1] == ' ') {
            // FIXME: very crude.
            unsigned long bploc = strtoul(&linebuf[2], NULL, 16);
            breakpoint_set(bploc);
        } else if (HAVE("n")) {
            byte op = peek_sneaky(current_pc());
            if (op == 0x20) {
                // Run until after stack is back to the current size
                fputs("Skipping subroutine call...\n", stdout);
                go_until_rts = true;
                stack_min = SP;
                loop = debugging_flag = false;
            } else {
                // Treat as equivalent to "s"
                loop = false;
            }
        } else if (HAVE("rts")) {
            // Run until stack is two higher than current
            fputs("Continuing until RTS...\n", stdout);
            go_until_rts = true;
            stack_min = LO(SP+2);
            loop = debugging_flag = false;
        } else if (handle_monitor_like_cmd()) {
            // Handled. But loop.
        } else {
            fputs("Unrecognized command!\n", stdout);
        }
#undef HAVE
    }

    if (!debugging_flag)
        event_fire(EV_REHOOK); // Interface can take over again.
}
