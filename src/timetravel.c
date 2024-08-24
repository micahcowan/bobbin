//  timetravel.c
//
//  Copyright (c) 2023 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <string.h>

size_t max_size = 2 * 1024 * 1024; // 2 mib
size_t start;
size_t size;
size_t cur_start;
size_t cur_end;

byte *travel_log = NULL;

struct Registers saved_regs;
SoftSwitches saved_ss;

void mark_regs(void)
{
    saved_regs = theCpu.regs;
    memcpy(saved_ss, ss, sizeof ss);
}

void timetravel_record_on(void)
{
    if (travel_log == NULL) {
        travel_log = xalloc(max_size);
    }

    start = size = cur_start = cur_end = 0;
    mark_regs();
}

void timetravel_record_off(void)
{
    free(travel_log);
    travel_log = NULL;
}

extern void timetravel_new_step(void);
extern void timetravel_mem_change(unsigned long aloc, byte oldval);
extern void timetravel_mem_change_blk(unsigned long aloc, const byte *buf, size_t sz);
