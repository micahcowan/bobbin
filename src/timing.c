//  timing.c
//
//  Copyright (c) 2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE tf for details.

#include "bobbin-internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

struct timing_t {
    bool    first_time;
    int     count;
    struct timespec ts;
    long    overslept;
};

#ifdef BOBBIN_TIMINGS_DEBUG
FILE    *tf;
#define timdbg(...)   fprintf(tf, __VA_ARGS__)
#else
#define timdbg(...)
#endif

struct timing_t  *timing_init(void)
{
    struct timing_t *t = xalloc(sizeof *t);
    t->first_time = true;
    t->count = 0;
    t->overslept = 0;

#ifdef BOBBIN_TIMINGS_DEBUG
    tf = fopen("timings.dbg", "w");
#endif
    return t;
}

void timing_adjust(struct timing_t *t)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (t->count < 100) {
        ++t->count;
    }

    if (t->first_time) {
        t->first_time = false;
        t->ts = now;
        return;
    }

    long elapsed;
    if (now.tv_sec == t->ts.tv_sec) {
        elapsed = now.tv_nsec - t->ts.tv_nsec;
    } else if (now.tv_sec == t->ts.tv_sec + 1) {
        elapsed = ONE_SEC_IN_NS + now.tv_nsec - t->ts.tv_nsec;
    } else {
        elapsed = NS_PER_FRAME; // we've already surpassed,
                                // no further waiting to do.
    }

    timdbg("--------------------\n");
    if (elapsed < NS_PER_FRAME) {
        long desired = NS_PER_FRAME - elapsed;

        if (t->overslept >= desired) {
            timdbg("%10ld elapsed\n", elapsed);
            timdbg("Withdrawing desired time from previous overage.\n");
            timdbg("%10ld desired, from\n", desired);
            timdbg("%10ld oversleep;\n", t->overslept);
            t->overslept -= desired;
            timdbg("%10ld oversleep still remaining.\n", t->overslept);
        }
        else {
            struct timespec sltime = now;
            sltime.tv_sec = 0;
            timdbg("%10ld elapsed\n%10ld desired\n",
                   elapsed, (long)NS_PER_FRAME);
            if (t->overslept > 0) {
                desired -= t->overslept;
                timdbg("%10ld oversleep BALANCE USED\n",
                       t->overslept);
                timdbg("%10ld desired FINAL\n", desired);
                t->overslept = 0;
            }
            sltime.tv_nsec = desired;
            (void) nanosleep(&sltime, NULL);
            struct timespec actual;
            clock_gettime(CLOCK_MONOTONIC, &actual);
            long new_addtl = actual.tv_nsec - now.tv_nsec;
            if (new_addtl > desired) {
                t->overslept = new_addtl - desired;
            }
            timdbg("%10ld actual additional\n%10ld oversleep\n",
                   (long)new_addtl,
                   (long)t->overslept);
            now = actual;
        }
    } else {
        timdbg("Frame took too long naturally, not sleeping any further.\n");
    }

    t->ts = now;
}
