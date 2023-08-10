#include "bobbin-internal.h"

static byte rombuf[256];

static void init(void)
{
    // XXX load ROM
}

byte handler(word loc, int val, int ploc, int psw)
{
    if (ploc != -1) {
        return rombuf[ploc];
    }

    // Soft-switch handling
    return 0; // XXX
}

PeriphDesc disk2card = {
    init,
    handler,
};
