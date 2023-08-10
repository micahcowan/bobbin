#include "bobbin-internal.h"

static byte *rombuf;

static void init(void)
{
    rombuf = load_rom("cards/disk2.rom", 256, false);
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
