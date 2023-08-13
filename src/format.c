#include "bobbin-internal.h"

#include <fcntl.h>
#include <unistd.h>

static const size_t nib_disksz = 232960;
static const size_t dsk_disksz = 143360;

extern DiskFormatDesc nib_insert(const char*, byte *, size_t);
extern DiskFormatDesc dsk_insert(const char *, byte *, size_t);
extern DiskFormatDesc empty_disk_desc;

DiskFormatDesc disk_insert(const char *path)
{
    if (path == NULL) {
        return empty_disk_desc;
    }
    byte *buf;
    size_t sz;
    int err = mmapfile(cfg.disk, &buf, &sz, O_RDWR);
    if (buf == NULL) {
        DIE(1,"Couldn't load/mmap disk %s: %s\n",
            path, strerror(err));
    }
    if (sz == nib_disksz) {
        return nib_insert(path, buf, sz);
    } else if (sz == dsk_disksz) {
        return dsk_insert(path, buf, sz);
    } else {
        DIE(2,"Unrecognized disk format for %s.\n", path);
    }
}
