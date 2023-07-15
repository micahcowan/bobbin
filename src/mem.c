#include "bobbin-internal.h"

static byte membuf[128 * 1024];

byte mem_get_byte(word loc)
{
    return membuf[loc];
}
