//  trace.c
//
//  Copyright (c) 2023-2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

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

void trace_write(word loc, byte val)
{
    if (traceon) {
        size_t aloc;
        MemAccessType acc;
        bool aux;
        mem_get_true_access(loc, true /* writing */, &aloc, &aux, &acc);
        fprintf(trfile, "w @%05zX $%04X :%02X %s%s\n", aloc, loc, val,
                mem_get_acctype_name(acc), aux? " (AUX)" : "");
    }
}

void trace_read(word loc, byte val)
{
    if (traceon) {
        size_t aloc;
        MemAccessType acc;
        mem_get_true_access(loc, false /* reading */, &aloc, NULL, &acc);
        fprintf(trfile, "r @%05zX $%04X :%02X %s\n", aloc, loc, val, mem_get_acctype_name(acc));
    }
}

void trace_reg(void)
{
    if (!handler_registered) {
        handler_registered = true;
        event_reghandler(trace_step);
    }
}

int tracing(void)
{
    return traceon;
}
