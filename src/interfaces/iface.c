#include "bobbin-internal.h"

extern void iface_simple_instr_init(void); // XXX

void interfaces_init(void)
{
    // No default interface selected?
    // Pick "simple" if stdin isn't a tty;
    // otherwise, pick "tty" (not yet implemented).
    iface_simple_instr_init();
}
