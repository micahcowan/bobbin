//  watch.c
//
//  Copyright (c) 2023 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>

typedef struct WRec {
    struct WRec *next;
    const char *path;
    struct stat sbuf;
} WRec;

WRec *wlist = NULL;

static int inotify_fd = -1;

void setup_watches(void)
{
    if (!cfg.watch) return; // We're not doing watches.
    if (wlist) return; // Don't do setup a second time.

    if (cfg.ram_load_file) {
        add_watch(cfg.ram_load_file);
    }

    if (cfg.runbasicfile) {
        add_watch(cfg.runbasicfile);
    }
}

void add_watch(const char *fname)
{
    WRec *rec = xalloc(sizeof *rec);
    errno = 0;
    int result = stat(fname, &rec->sbuf);
    if (result < 0) {
        int err = errno;
        DIE(1, "Couldn't stat \"%s\" to start watching: %s.\n",
            fname, strerror(err));
    }

    size_t namelen = strlen(fname);
    char *path = xalloc(namelen + 1);
    memcpy(path, fname, namelen + 1);
    rec->path = path;
    INFO("Watching \"%s\" for changes.\n", fname);

    rec->next = wlist;
    wlist = rec;
}

bool check_watches(void)
{
    WRec *rec;
    const char *changed = NULL;
    struct timespec mtime;
    for (rec = wlist; rec != NULL; rec = rec->next) {
        mtime = rec->sbuf.st_mtim;
        errno = 0;
        int result = stat(rec->path, &rec->sbuf);
        if (result < 0) {
            int err = errno;
            if (err == ENOENT) {
                DIE(1, "File \"%s\" disappeared while watching.", rec->path);
            }
            else {
                DIE(1, "Couldn't stat \"%s\" while watching: %s.\n",
                    rec->path, strerror(err));
            }
        }
        if (mtime.tv_sec != rec->sbuf.st_mtim.tv_sec
            || mtime.tv_nsec != rec->sbuf.st_mtim.tv_nsec) {
            // Note the changed file, keep looking for more (so as not
            // to retrigger belatedly on another file, after reboot).
            changed = rec->path;
        }
    }

    if (changed) {
        WARN("Rewrite event for watched file \"%s\". Restarting...\n", changed);
        event_fire(EV_REBOOT);
        return true;
    }
    return false;
}
