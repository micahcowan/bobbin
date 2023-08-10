#include "bobbin-internal.h"

#include <assert.h>
#include <stdlib.h>

struct handler {
    event_handler fn;
    struct handler *next;
};

static struct handler *head = NULL;

static const Event evinit = {
    .suppress = false,
    .val = -1,
};

void events_init(void)
{
    extern void hooks_init(void);
    hooks_init();
}

void event_reghandler(event_handler fn)
{
    struct handler *h = xalloc(sizeof *h);
    h->fn = fn;
    h->next = head;
    head = h;
}

void event_unreghandler(event_handler h)
{
    // XXX Currently unimplemented
}

void dispatch(Event *e)
{
    struct handler *h;
    if (e->type == EV_PRESTEP) {
        word pc;
        const unsigned int max_count = 100;
        unsigned int count = 0;
        do {
            if (count++ >= max_count) {
                DIE(1,"PC changed during prestep %u times!\n", max_count);
            }
            pc = PC;
            for (h = head; pc == PC && h != NULL; h = h->next) {
                h->fn(e);
            }
        } while (pc != PC);
    } else {
        for (h = head; h != NULL; h = h->next) {
            h->fn(e);
        }
    }
}

static bool for_iface_only(EventType t)
{
    return (t == EV_UNHOOK || t == EV_REHOOK || t == EV_DISPLAY_TOUCH);
}

void event_fire(EventType type)
{
    Event *e = xalloc(sizeof *e);
    *e = evinit;
    e->type = type;

    // special handling
    switch (type) {
        case EV_REBOOT:
        {
            mem_reboot();
            event_fire(EV_RESET);
            event_fire(EV_DISPLAY_TOUCH);
        }
            break;
        case EV_RESET:
        {
            cpu_reset();
            mem_reset();
        }
            break;
        default:
            ;
    }

    iface_fire(e);

    if (!(e->type == EV_FRAME || e->type == EV_CYCLE // dispatched specially
          || for_iface_only(e->type))) {
        dispatch(e);
    }

    if (type == EV_STEP) {
        // Not allowed to change PC in STEP, PEEK, POKE events...
        assert(PC == current_pc());
    }

    free(e);
}

extern int event_fire_peek(word loc)
{
    Event *e = xalloc(sizeof *e);
    *e = evinit;
    e->type = EV_PEEK;
    e->loc = loc;
    e->aloc = mem_transform_aux(loc, false);
    word pc = PC; // may not eq current_instruction, if we're in the midst
                  //  of some CPU thing
    iface_fire(e);
    assert(pc == PC);
    int val = e->val;
    free(e);
    return val;
}

extern bool event_fire_poke(word loc, byte val)
{
    Event *e = xalloc(sizeof *e);
    *e = evinit;
    e->type = EV_POKE;
    e->loc = loc;
    e->aloc = mem_transform_aux(loc, true);
    e->val = val;
    word pc = PC; // may not eq current_instruction, if we're in the midst
                  //  of some CPU thing
    iface_fire(e);
    assert(pc == PC);
    bool suppress = e->suppress;
    free(e);
    return suppress;
}
