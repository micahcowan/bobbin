//  debug.c
//
//  Copyright (c) 2023 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Breakpoint Breakpoint;
struct Breakpoint {
    word loc;
    bool is_watchpoint;
    byte val;
    bool enabled;
    Breakpoint *next;
};

static Breakpoint *bp_head = NULL;

static char linebuf[256];

static bool debugging_flag = false;
static bool print_message = true;
static bool go_until_rts = false;
static bool cont_dest_flag = false;
static byte stack_min;
static word cont_dest;

bool debugging(void)
{
    return debugging_flag;
}

void dbg_on(void)
{
    event_fire(EV_UNHOOK);
    debugging_flag = true;
    print_message = true;
}

static void breakpoint_set_(word loc, bool wp)
{
    Breakpoint *bp = xalloc(sizeof *bp);

    bp->loc = loc;
    bp->enabled = true;
    bp->next = NULL;
    bp->is_watchpoint = wp;
    if (wp) {
        bp->val = peek_sneaky(loc);
    }

    // Find tail of chain
    if (bp_head == NULL) {
        bp_head = bp;
    } else {
        Breakpoint *tail;
        for (tail = bp_head; tail->next != NULL; tail = tail->next) {}
        tail->next = bp;
    }

    if (wp) {
        printf("Watchpoint set for $%04X (cur val is $%02X).\n",
               (unsigned int)bp->loc, (unsigned int)bp->val);
    } else {
        printf("Breakpoint set for $%04X.\n", (unsigned int)bp->loc);
    }
}

void breakpoint_set(word loc)
{
    breakpoint_set_(loc, false);
}

void watchpoint_set(word loc)
{
    breakpoint_set_(loc, true);
}

static bool bp_reached(void)
{
    byte op = peek_sneaky(current_pc());
    if (go_until_rts && SP >= stack_min) {
        go_until_rts = false;
        event_fire(EV_UNHOOK); // Early, so the below message is visible
        printf("Returned.\n");
        return true;
    }

    if (cont_dest_flag && current_pc() == cont_dest) {
        cont_dest_flag = false;
        event_fire(EV_UNHOOK); // Early, so the below message is visible
        printf("Arrived at $%04X.\n", (unsigned int)cont_dest);
        return true;
    }

    Breakpoint *bp;
    word pc = current_pc();
    int i;
    for (bp = bp_head, i = 1; bp != NULL; ++i, bp = bp->next) {
        byte val = peek_sneaky(bp->loc);
        if (!bp->enabled) {
            // skip this one
        } else if (bp->is_watchpoint && val != bp->val) {
            event_fire(EV_UNHOOK); // Early, so the below message is visible
            printf("Watchpoint %d fired:\n", i);
            printf("Value changed at $%04X ($%02X -> $%02X).\n",
                   (unsigned int)(bp->loc), (unsigned int)bp->val,
                   (unsigned int)val);
            bp->val = val;
            return true;
        } else if (!bp->is_watchpoint && pc == bp->loc) {
            event_fire(EV_UNHOOK); // Early, so the below message is visible
            printf("Breakpoint %d at $%04X.\n", i, current_pc());
            return true;
        }
    }
    return false;
}

static inline void bp_disable(int num)
{
    Breakpoint *bp;
    int i;
    for (bp = bp_head, i = 1; bp != NULL; ++i, bp = bp->next) {
        if (i == num) {
            bp->enabled = false;
            printf("Breakpoint %d disabled.\n", i);
            return;
        }
    }
    printf("ERR: no such breakpoint #%d.\n", num);
}

static inline void bp_enable(int num)
{
    Breakpoint *bp;
    int i;
    for (bp = bp_head, i = 1; bp != NULL; ++i, bp = bp->next) {
        if (i == num) {
            bp->enabled = true;
            if (bp->is_watchpoint) {
                bp->val = peek_sneaky(bp->loc);
            }
            printf("Breakpoint %d enabled.\n", i);
            return;
        }
    }
    printf("ERR: no such breakpoint #%d.\n", num);
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
    while(++next <= last) {
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

static void mlcmd_write(word mem, const char *str)
{
    unsigned long val;
    char *end;

    for (;;) {
        while (*str == ' ') ++str;
        val = strtoul(str, &end, 16);
        if (str == end) {
            if (*str != '\0') {
                printf("ERR: garbage at end of mon wr: %s\n", str);
            }
            break; // done processing writes
        }

        poke_sneaky(mem, val);
        ++mem;
        str = end;
    }
}

// returns true if command in linebuf was handled.
static bool handle_monitor_like_cmd(bool *loop)
{
    unsigned long first, second = 0;
    const char *str = linebuf;
    char *end;

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
    } else if (STREQCASE(str, "G")) {
        PC = first;
        debugging_flag = *loop = false;
    } else if (*str == '\0') {
        mlcmd_read(first, second);
    } else if (*str == ':') {
        mlcmd_write(first, str+1);
    } else {
        // Garbage at end of line: unrecognized command overall.
        return false;
    }

    return true;
}

void debugger(void)
{
    if (sigint_received >= 2) {
        fputs("Debug entered via ^C^C.\n", stdout);
        sigint_received = 0;
    } else if (bp_reached()) {
        // Handled
    } else if (!debugging_flag) {
        return;
    }
    event_fire(EV_UNHOOK);
    debugging_flag = true;

    go_until_rts = false;
    cont_dest_flag = false;

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
                fputs("(Interrupted by signal; re-prompting...)\n", stdout);
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
        } else if (HAVE("cycles")) {
            printf("cycles: %" PRIuMAX "\n", cycle_count);
        } else if (linebuf[0] == 'c') {
            if (linebuf[1] == '\0') {
                fputs("Continuing...\n", stdout);
                loop = debugging_flag = false;
            } else if (linebuf[1] == ' ' && linebuf[2] != '\0') {
                char *end;
                unsigned long dest = strtoul(&linebuf[2], &end, 16);
                if (*end != '\0') {
                    fputs("ERR: Garbage at end of 'c' command.\n", stdout);
                }
                loop = debugging_flag = false;
                cont_dest = dest;
                cont_dest_flag = true;
                printf("Continuing until $%04X...\n", (unsigned int)dest);
            }
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
        } else if (linebuf[0] == 'w' && linebuf[1] == ' ') {
            char *end;
            unsigned long dest = strtoul(&linebuf[2], &end, 16);
            if (*end != '\0') {
                fputs("ERR: Garbage at end of 'w' command.\n", stdout);
            }
            watchpoint_set(dest);
        } else if (memcmp(linebuf, "disable", 7) == 0) {
            if (linebuf[7] == ' ') {
                char *end;
                unsigned long dest = strtoul(&linebuf[8], &end, 16);
                if (end == &linebuf[8] || *end != '\0') {
                    fputs("ERR: Garbage at end of 'disable' command.\n",
                          stdout);
                    continue;
                }
                bp_disable(dest);
            }
        } else if (memcmp(linebuf, "enable", 6) == 0) {
            if (linebuf[6] == ' ') {
                char *end;
                unsigned long dest = strtoul(&linebuf[7], &end, 16);
                if (end == &linebuf[7] || *end != '\0') {
                    fputs("ERR: Garbage at end of 'enable' command.\n",
                          stdout);
                    continue;
                }
                bp_enable(dest);
            }
        } else if (HAVE("rts")) {
            // Run until stack is two higher than current
            fputs("Continuing until RTS...\n", stdout);
            go_until_rts = true;
            stack_min = LO(SP+2);
            loop = debugging_flag = false;
        } else if (HAVE("tron")) {
            trace_on("Trace started from debugger");
            printf("Trace log started to file \"%s\".\n", cfg.trace_file);
        } else if (HAVE("troff")) {
            trace_off();
            printf("Trace log \"%s\" closed.\n", cfg.trace_file);
        } else if (handle_monitor_like_cmd(&loop)) {
            // Handled.
        } else {
            fputs("Unrecognized command!\n", stdout);
        }
#undef HAVE
    }

    if (!debugging_flag)
        event_fire(EV_REHOOK); // Interface can take over again.
}
