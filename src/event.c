#include "bobbin-internal.h"

#include <assert.h>
#include <stdlib.h>

static const Event evinit = {
    .suppress = false,
    .val = -1,
};

void events_init(void)
{
}

void event_reghandler(event_handler h)
{
}

void event_unreghandler(event_handler h)
{
}

void event_fire(EventType type)
{
    Event *e = malloc(sizeof *e);
    *e = evinit;
    e->type = type;

    // XXX special handling
    static bool did_delayed_jmp = false;
    switch (type) {
        case EV_REBOOT:
        {
            mem_reboot();
            event_fire(EV_RESET);
            event_fire(EV_DISPLAY_TOUCH);
            did_delayed_jmp = false;
        }
            break;
        case EV_RESET:
        {
            cpu_reset();
            mem_reset();
        }
            break;
        case EV_PRESTEP:
        {
            if (cfg.delay_set && PC == cfg.delay_until && !did_delayed_jmp) {
                did_delayed_jmp = true;
                if (cfg.start_loc_set) {
                    INFO("Jumping PC to $%04X due to --delay-until option.\n",
                         cfg.start_loc_set);
                    PC = cfg.start_loc;
                }
                load_ram_finish();
                event_fire(EV_DISPLAY_TOUCH);
            }
        }
            break;
        case EV_STEP:
        {
            trace_step();
            extern void trap_step(void);
            trap_step();

        }
            break;
        default:
            ;
    }

    iface_fire(e);

    if (type == EV_STEP) {
        // Not allowed to change PC in STEP, PEEK, POKE events...
        assert(PC == current_pc());
    }

    free(e);
}

extern int event_fire_peek(word loc)
{
    Event *e = malloc(sizeof *e);
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
    Event *e = malloc(sizeof *e);
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
