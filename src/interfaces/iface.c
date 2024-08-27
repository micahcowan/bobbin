//  interfaces/iface.c
//
//  Copyright (c) 2023-2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <stdio.h>
#include <unistd.h>

extern IfaceDesc simpleInterface;
#ifdef HAVE_LIBCURSES
extern IfaceDesc ttyInterface;
#endif

static IfaceDesc *iii = NULL;

static struct if_t {
    const char *name;
    IfaceDesc *iface;
} ifs[] = {
#ifdef HAVE_LIBCURSES
    {"tty", &ttyInterface},
#endif
    {"simple", &simpleInterface},
};

void iface_fire(Event *e)
{
    if (iii->event)
        iii->event(e);
}

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
    if (cfg.tokenize) {
        // Force interface to "simple".
        cfg.interface = "simple";
        if (isatty(STDOUT_FILENO)) {
            DIE(2,"Can't --tokenize output to a tty!\n");
        }
        if (cfg.remain_after_pipe || cfg.remain_tty) {
            DIE(2,"--tokenize conflicts with --remain.\n");
        }
    } else if (cfg.detokenize || cfg.runbasicfile) {
        // Force interface to "simple".
        cfg.interface = "simple";
    } if (cfg.interface == NULL) {
        // No default interface selected?
        // Pick "simple" if stdin isn't a tty,
        // otherwise, pick "tty" (not yet implemented).
        cfg.interface = (isatty(0)) ? "tty" : "simple";
    }

#ifndef HAVE_LIBCURSES
    if (STREQ(cfg.interface, "tty")) {
        DIE(0,"default interface is \"tty\" when stdin is a tty; but that\n");
        DIE(0,"interface has not been included with this build of bobbin\n");
        DIE(0,"(may have been missing a libncurses-dev or similar package?)\n");
        DIE(2,"Try invoking with --simple, instead.\n");
    }
#endif
    load_interface();
    if (iii == NULL) {
        DIE(2,"unsupported interface \"%s\".\n", cfg.interface);
    }

    Event e = { .type = EV_INIT };
    iface_fire(&e);
}

void interfaces_start(void)
{
    Event e = { .type = EV_START };
    iface_fire(&e);
}

void squawk(int level, bool cont, const char *fmt, ...)
{
    bool suppress = false;
    va_list args;
    va_start(args, fmt);
    if (level == DIE_LEVEL) {
        if (iii) {
            event_fire(EV_UNHOOK);
        }
    } else if (iii != NULL && iii->squawk != NULL) {
        suppress = iii->squawk(level, cont, fmt, args);
    }

    if (!suppress) {
        if (!cont) {
            fprintf(stderr, "%s: ", program_name);
        }
        vfprintf(stderr, fmt, args);
    }
    va_end(args);
}
