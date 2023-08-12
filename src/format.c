#include "bobbin-internal.h"

#include <fcntl.h>
#include <unistd.h>

extern DiskFormatDesc nib_insert(const char*, byte *, size_t);

DiskFormatDesc disk_insert(const char *path)
{
    byte *buf;
    size_t sz;
    int err = mmapfile(cfg.disk, &buf, &sz, O_RDWR);
    if (buf == NULL) {
        DIE(1,"Couldn't load/mmap disk %s: %s\n",
            path, strerror(err));
    }
    return nib_insert(path, buf, sz);
}
