#include "bobbin-internal.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

word current_instruction = 0;
uintmax_t cycle_count = 0;

FILE *trfile = NULL;

static int traceon = 0;

static void do_rts(void)
{
    byte lo = stack_pop();
    byte hi = stack_pop();
    go_to(WORD(lo, hi)+1);
}

static void trap_hooks(void)
{
    if (cfg.trap_failure_on && current_instruction == cfg.trap_failure) {
        fputs("*** ERROR TRAP REACHED ***\n", stderr);
        fprintf(stderr, "Instr #: %ju\n", instr_count);
        fprintf(stderr, "Failed testcase: %02X\n",
                peek_sneaky(0x200));
        do_rts();
        PC -= 3;
        current_instruction = PC;
        util_print_state(stderr);
        exit(3);
    } else if (cfg.trap_success_on && current_instruction == cfg.trap_success) {
        fputs(".-= !!! REPORT SUCCESS !!! =-.\n", stderr);
        exit(0);
    }
}

void trace_instr(void)
{
    current_instruction = PC; // global

    if (cfg.trace_start == cfg.trace_end) {
        // Do nothing.
    } else if (instr_count == cfg.trace_start) {
        trace_on("Requested by user");
    } else if (instr_count == cfg.trace_end) {
        trace_off();
    }

    if (traceon) {
        fprintf(trfile, "%79ju\n", instr_count);
        util_print_state(trfile);
    }

    trap_hooks();

    iface_step();
}

int trace_peek_sneaky(word loc)
{
    return -1;
}

int trace_peek(word loc)
{
    return iface_peek(loc);
}

void trace_on(char *format, ...)
{
    va_list args;

    if (trfile == NULL) {
        trfile = fopen(cfg.trace_file, "w");
        if (trfile == NULL) {
            perror("Couldn't open trace file");
            exit(2);
        }
        setvbuf(trfile, NULL, _IOLBF, 0);
    }

    fprintf(trfile, "\n\n~~~ TRACING STARTED: ");
    va_start(args, format);
    vfprintf(trfile, format, args);
    va_end(args);
    fprintf(trfile, " ~~~\n");
    traceon = 1;
}

void trace_off(void)
{
    fprintf(trfile, "~~~ TRACING FINISHED ~~~\n");
    traceon = 0;
}

int tracing(void)
{
    return traceon;
}
