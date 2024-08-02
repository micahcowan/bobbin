//  delay-pc.c
//
//  Copyright (c) 2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

static void delay_step(Event *e)
{
    // XXX check if we've reached the next "delay until" PC value we're
    //  awaiting.
}

void dlypc_init(void) {
    event_reghandler(delay_step);
}

void dlypc_delay_until_s(const char *loc_s) {
    // XXX
}
void dlypc_load(const char *fname) {
    // XXX
}

void dlypc_load_basic(const char *fname) {
    // XXX
}

void dlypc_load_at_s(const char *loc_s) {
    // XXX
}

void dlypc_jump_to_s(const char *loc_s) {
    // XXX
}

void dlypc_delay_until(word loc) {
    // XXX
}

void dlypc_load_at(word loc) {
    // XXX
}

void dlypc_jump_to(word loc) {
    // XXX
}

void dlypc_restart(void) {
    // XXX
}

void dlypc_reboot(void) {
    // XXX
}


// iterator abstraction for traversing the files to be loaded
struct dlypc_file_iter;

struct dlypc_file_iter *dlypc_file_iter_new(void) {
    // XXX
}

const char *dlypc_file_iter_getnext(struct dlypc_file_iter *iter) {
    // XXX
}

void dlypc_file_iter_destroy(struct dlypc_file_iter *iter) {
    // XXX
}

