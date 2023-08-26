#include "bobbin-internal.h"

#include <assert.h>
#include <stdlib.h>

struct timedfn {
    void (*fn)(void);
    unsigned int frames_left;
    struct timedfn *next;
};
struct timedfn *tmhead = NULL;
bool timer_access = false;

void frame_timer_cancel(void (*fn)(void))
{
    assert(!timer_access);
    timer_access = true;
    struct timedfn *p;
    struct timedfn **prevnext = &tmhead;
    for (p = tmhead; p != NULL; p = p->next) {
        if (p->fn == fn) {
            *prevnext = p->next;
            free(p);
            break;
        }
        prevnext = &p->next;
    }
    timer_access = false;
    return;
}

static struct timedfn *frame_timer_reset_(unsigned int time, void (*fn)(void))
{
    struct timedfn *p;
    for (p = tmhead; p != NULL; p = p->next) {
        if (p->fn == fn) {
            p->frames_left = time;
            break;
        }
    }
    return p;
}

void frame_timer_reset(unsigned int time, void (*fn)(void))
{
    (void) frame_timer_reset_(time, fn);
}

void frame_timer(unsigned int time, void (*fn)(void))
{
    // Try to reset timer if it exists
    struct timedfn *p = frame_timer_reset_(time, fn);

    // Otherwise, create
    if (p == NULL) {
        struct timedfn *newtm = xalloc(sizeof *newtm);
        newtm->fn = fn;
        newtm->frames_left = time;
        newtm->next = tmhead;
        tmhead = newtm;
    }
}

void frame_timer_countdown(void)
{
    struct timedfn *p;
    struct timedfn **prevnext = &tmhead;
    for (p = tmhead; p != NULL; ) {
        if (--(p->frames_left) == 0) {
            p->fn();
            *prevnext = p->next;
            free(p);
            p = *prevnext;
        } else {
            prevnext = &(p->next);
            p = *prevnext;
        }
    }
}

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
    if (e->type == EV_FRAME) {
        frame_timer_countdown();
    }

    if (type == EV_STEP) {
        // Not allowed to change PC in STEP, PEEK, POKE events...
        assert(PC == current_pc());
    }

    free(e);
}

int event_fire_peek(word loc)
{
    Event *e = xalloc(sizeof *e);
    *e = evinit;
    e->type = EV_PEEK;
    e->loc = loc;
    e->aloc = mem_transform_aux(loc, false);
    word pc = PC; // may not eq current_instruction, if we're in the midst
                  //  of some CPU thing
    iface_fire(e);
    dispatch(e);
    assert(pc == PC);
    int val = e->val;
    free(e);
    return val;
}

bool event_fire_poke(word loc, byte val)
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
    dispatch(e);
    assert(pc == PC);
    bool suppress = e->suppress;
    free(e);
    return suppress;
}

void event_fire_disk_active(int val)
{
    Event *e = xalloc(sizeof *e);
    *e = evinit;
    e->type = EV_DISK_ACTIVE;
    e->val = val;

    iface_fire(e);
    free(e);
}

void event_fire_switch(SoftSwitchFlagPos f)
{
    Event *e = xalloc(sizeof *e);
    *e = evinit;
    e->type = EV_SWITCH;
    e->val = f;

    iface_fire(e);
    free(e);
}
