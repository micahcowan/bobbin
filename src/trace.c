#include "bobbin-internal.h"
#include "iface-simple.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

word current_instruction = 0;

FILE *trfile;

static int traceon = 0;

void trace_instr(void)
{
    current_instruction = PC;

    if (traceon)
        util_print_state(trfile);

    iface_simple_instr_hook();
}

int trace_mem_get_byte(word loc)
{
    return iface_simple_getb_hook(loc);
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
