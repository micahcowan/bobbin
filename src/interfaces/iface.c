#include "bobbin-internal.h"

extern IfaceDesc simpleInterface;

void interfaces_init(void)
{
    // No default interface selected?
    // Pick "simple" if stdin isn't a tty;
    // otherwise, pick "tty" (not yet implemented).
    simpleInterface.init();
}

void iface_step(void)
{
    simpleInterface.step();
}

int iface_peek(word loc)
{
    return simpleInterface.peek(loc);
}

int iface_poke(word loc, byte val)
{
    return simpleInterface.poke(loc, val);
}
