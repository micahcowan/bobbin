#include "bobbin-internal.h"

#include <unistd.h>

extern IfaceDesc simpleInterface;

static IfaceDesc *iii = NULL;

static struct if_t {
    const char *name;
    IfaceDesc *iface;
} ifs[] = {
    {"tty", NULL},
    {"simple", &simpleInterface},
};

static
void load_interface(void)
{
    struct if_t *visit = &ifs[0];
    struct if_t *end = visit + (sizeof ifs)/(sizeof ifs[0]);

    iii = NULL;
    for (; visit != end; ++visit) {
        if (STREQ(visit->name, cfg.interface)) {
            iii = visit->iface;
            return;
        }
    }
}

void interfaces_init(void)
{
    if (cfg.interface == NULL) {
        // No default interface selected?
        // Pick "simple" if stdin isn't a tty;
        // otherwise, pick "tty" (not yet implemented).
        cfg.interface = isatty(0)? "tty" : "simple";
    }

    if (STREQ(cfg.interface, "tty")) {
        WARN("default interface is \"tty\" when stdin is a tty; but that\n");
        WARN("interface is not yet implemented in this development version.\n");
        DIE(2,"Try invoking with --iface simple.\n");
    }
    load_interface();
    if (iii == NULL) {
        DIE(2,"unsupported interface \"%s\".\n", cfg.interface);
    }
    iii->init();
}

void iface_step(void)
{
    iii->step();
}

int iface_peek(word loc)
{
    return iii->peek(loc);
}

int iface_poke(word loc, byte val)
{
    return iii->poke(loc, val);
}
