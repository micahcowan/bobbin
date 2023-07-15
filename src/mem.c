#include "bobbin-internal.h"

static byte membuf[128 * 1024];

byte mem_get_byte(word loc)
{
    // XXX
    return membuf[loc];
}

void mem_put_byte(word loc, byte val)
{
    // XXX
    membuf[loc] = val;
}
