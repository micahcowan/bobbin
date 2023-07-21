#include "bobbin-internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

word current_instruction = 0;
unsigned long long cycle_count = 0;

FILE *trfile;

static int traceon = 0;

static void do_rts(void)
{
    byte lo = stack_pop();
    byte hi = stack_pop();
    go_to(WORD(lo, hi)+1);
}

static void bobbin_hooks(void)
{
#if 0
    if (!traceon && (cycle_count + 256) > 96404509)
        trace_on("6502 TESTS");
#endif

    switch (current_instruction) {
        case 0:
            // report_init
            do_rts();
            break;
        case 1:
            fputs("*** REPORT ERROR ***\n", stderr);
            fprintf(stderr, "Cycle count: %llu\n", cycle_count);
            fprintf(stderr, "Failed testcase: %02X\n",
                    peek_sneaky(0x200));
            do_rts();
            PC -= 3;
            current_instruction = PC;
            util_print_state(stderr);
            exit(3);
            break;
        case 2:
            fputs(".-= !!! REPORT SUCCESS !!! =-.\n", stderr);
            exit(0);
            break;
        default:
            ;
    }
}

void trace_instr(void)
{
    current_instruction = PC;

    if (traceon) {
        fputc('\n', trfile);
        util_print_state(trfile);
    }

    if (bobbin_test)
        bobbin_hooks();

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
