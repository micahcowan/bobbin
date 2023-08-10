#include "bobbin-internal.h"

#include <stdbool.h>

#ifdef HAVE_SYS_INOTIFY_H
#include <errno.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/inotify.h>
#endif

static int inotify_fd = -1;

void setup_watches(void)
{
#ifdef HAVE_SYS_INOTIFY_H
    if (cfg.watch && cfg.ram_load_file) {
        errno = 0;
        inotify_fd = inotify_init1(O_NONBLOCK);
        if (inotify_fd < 0) {
            DIE(1,"Failed to set up watches: %s\n", strerror(errno));
        }

        errno = 0;
        int err = inotify_add_watch(inotify_fd, cfg.ram_load_file,
                                    IN_CLOSE_WRITE);
        if (err < 0) {
            DIE(1,"Failed to set up watch for \"%s\": %s\n",
                cfg.ram_load_file, strerror(errno));
        }
        INFO("Watching \"%s\" for rewrites.\n", cfg.ram_load_file);
    }
#else  // HAVE_SYS_INOTIFY_H
    if (cfg.watch) {
        DIE(0,"--watch requested, but this build of bobbin is"
            " not configured\n");
        DIE(2,"with inotify support (only available on Linux).\n");
    }
#endif // HAVE_SYS_INOTIFY_H
}

bool check_watches(void)
{
#ifdef HAVE_SYS_INOTIFY_H
    struct inotify_event evt;
    int rb = read(inotify_fd, &evt, sizeof evt);
    if (rb == sizeof evt) {
        INFO("Rewrite event for watched file. Rebooting...\n");
        event_fire(EV_REBOOT);
        return true;
    }
#endif
    return false;
}
