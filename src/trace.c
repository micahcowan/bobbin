#include "bobbin-internal.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

bool handler_registered = false;

uintmax_t cycle_count = 0;

FILE *trfile = NULL;

static int traceon = 0;

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
    if (!handler_registered) {
        handler_registered = true;
        event_reghandler(trace_step);
    }
}

void trace_off(void)
{
    fprintf(trfile, "~~~ TRACING FINISHED ~~~\n");
    traceon = 0;
}

void trace_step(Event *e)
{
    if (e->type != EV_STEP) return;
    if (cfg.trace_start == cfg.trace_end) {
        // Do nothing.
    } else if (instr_count == cfg.trace_start) {
        trace_on("Requested by user");
    } else if (instr_count == cfg.trace_end) {
        trace_off();
    }

    if (traceon) {
        fprintf(trfile, "%79ju\n", instr_count);
        util_print_state(trfile, current_pc(), &theCpu.regs);
    }
}

int tracing(void)
{
    return traceon;
}
