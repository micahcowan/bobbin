#include "bobbin-internal.h"

#include <assert.h>

static PeriphDesc *slot[8];

extern PeriphDesc disk2card;

PeriphDesc *get_sw_slot(word loc)
{
    assert(loc >= 0xC090 && loc < 0xC100);
    return slot[(loc & 0x0070) >> 4];
}

PeriphDesc *get_rom_slot(word loc)
{
    assert(loc >= 0xC100 && loc < 0xC800);
    return slot[(loc & 0x0700) >> 8];
}

byte periph_sw_peek(word loc)
{
    PeriphDesc *p = get_sw_slot(loc);
    if (p) {
        return p->handler(loc, -1, -1, loc & 0x000F);
    } else {
        return 0; // s/b floating bus
    }
}

void periph_sw_poke(word loc, byte val)
{
    PeriphDesc *p = get_sw_slot(loc);
    if (p) {
        (void) p->handler(loc, val, -1, loc & 0x000F);
    }
}

byte periph_rom_peek(word loc)
{
    PeriphDesc *p = get_rom_slot(loc);
    if (p) {
        return p->handler(loc, -1, loc & 0x00FF, -1);
    } else {
        return 0; // s/b floating bus
    }
}

void periph_rom_poke(word loc, byte val)
{
    // XXX
}

void periph_init(void)
{
    if (cfg.disk) {
        slot[6] = &disk2card;
    }
    
    const int slots_end = (sizeof slot)/(sizeof slot[0]);
    for (int i=0; i != slots_end; ++i) {
        if (slot[i] != NULL && slot[i]->init != NULL) {
            slot[i]->init();
        }
    }
}
