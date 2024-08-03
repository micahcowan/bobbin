//  watch.c
//
//  Copyright (c) 2023 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>

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

    // Add watches for all the --load files
    struct dlypc_file_iter *iter = dlypc_file_iter_new();
    const char *fname;
    while ((fname = dlypc_file_iter_getnext(iter)) != NULL)
    {
        add_watch(fname);
    }
    dlypc_file_iter_destroy(iter);

    (void) alarm(1);
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
    if (!sigalrm_received) return false;

    WRec *rec;
    const char *changed = NULL;
    struct timespec mtime;
#if defined(__APPLE__) || defined(__NetBSD__)
// Darwin and NetBSD don't follow the standard naming from POSIX.1-2008
#  define st_mtim st_mtimespec
#endif
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
    }
    sigalrm_received = 0;
    (void) alarm(1);
    if (changed) {
        return true;
    } else {
        return false;
    }
}
