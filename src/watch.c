//  watch.c
//
//  Copyright (c) 2023 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <stdbool.h>

#ifdef HAVE_SYS_INOTIFY_H
#include <errno.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/inotify.h>
#endif

typedef struct WRec {
    struct WRec *next;
    int fd;
    const char *path;
} WRec;

WRec *wlist = NULL;

static int inotify_fd = -1;

void setup_watches(void)
{
#ifdef HAVE_SYS_INOTIFY_H
    if (cfg.watch && (cfg.ram_load_file || cfg.runbasicfile)) {
        errno = 0;
        inotify_fd = inotify_init1(O_NONBLOCK);
        if (inotify_fd < 0) {
            DIE(1,"Failed to set up watches: %s\n", strerror(errno));
        }

        if (cfg.ram_load_file) {
            add_watch(cfg.ram_load_file);
        }

        if (cfg.runbasicfile) {
            add_watch(cfg.runbasicfile);
        }
    }
#else  // HAVE_SYS_INOTIFY_H
    if (cfg.watch) {
        DIE(0,"--watch requested, but this build of bobbin is"
            " not configured\n");
        DIE(2,"with inotify support (only available on Linux).\n");
    }
#endif // HAVE_SYS_INOTIFY_H
}

void add_watch(const char *fname)
{
    errno = 0;
    int fd  = inotify_add_watch(inotify_fd, fname,
                                IN_CLOSE_WRITE | IN_DELETE_SELF);
    if (fd < 0) {
        DIE(1,"Failed to set up watch for \"%s\": %s\n",
            fname, strerror(errno));
    }
    INFO("Watching \"%s\" for rewrites.\n", fname);

    // Record this watch, as we may have to reinstate it.
    WRec *rec = xalloc(sizeof *rec);
    rec->next = wlist;
    rec->fd = fd;
    size_t namelen = strlen(fname);
    char *path = xalloc(namelen + 1);
    memcpy(path, fname, namelen + 1);
    rec->path = path;
    wlist = rec;
}

void reinstate_watch(int fd)
{
    // Trawl through the list and find our expired watch
    WRec *rec;
    for (rec = wlist; rec != NULL && rec->fd != fd; rec = rec->next) {
    }

    if (rec == NULL) {
        return;
        DIE(3,"INTERNAL: Couldn't locate watch fd %d to reinstate.\n", fd);
    }

    // Close the watch just in case.
    (void) inotify_rm_watch(inotify_fd, rec->fd);
    sleep(1);
    errno = 0;
    int newfd = inotify_add_watch(inotify_fd, rec->path,
                                  IN_CLOSE_WRITE | IN_DELETE_SELF);
    if (newfd <= 0) {
        DIE(1,"Failed to reinstate watch for \"%s\": %s\n",
            rec->path, strerror(errno));
    }
    INFO("Reinstating \"%s\" for rewrite watches.\n", rec->path);
    rec->fd = newfd;
}

bool check_watches(void)
{
#ifdef HAVE_SYS_INOTIFY_H
    struct inotify_event evt;
    int rb = read(inotify_fd, &evt, sizeof evt);
    if (rb == sizeof evt) {
        INFO("Rewrite event for watched file. Rebooting...\n");
        event_fire(EV_REBOOT);
        if (evt.mask & IN_IGNORED) {
            reinstate_watch(evt.wd);
        }
        return true;
    }
#endif
    return false;
}
