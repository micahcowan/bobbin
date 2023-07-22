#include "bobbin-internal.h"

/* For the ROM file, and error handling */
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char *bobbin_test;

// Enough of a RAM buffer to provide 128k
static byte membuf[128 * 1024];

// Pointer to firmware, mapped into the Apple starting at $D000
static unsigned char *rombuf;

static void load_test_code(void)
{
    FILE *tf = fopen(bobbin_test, "r");
    if (!tf) {
        perror("Couldn't open BOBBIN_TEST file");
        exit(2);
    }

    (void) fread(membuf, 1, 64 * 1024, tf);
}

static const char * const rom_dirs[] = {
    "BOBBIN_ROMDIR", // not a dirname, an env var name
    PREFIX "/share/bobbin/roms",
    "./roms",
};
static const char * const *romdirp = rom_dirs;
static const char * const * const romdend = rom_dirs + (sizeof rom_dirs)/(sizeof rom_dirs[0]);
static const char *get_try_rom_path(const char *fname) {
    static char buf[256];
    const char *env;
    const char *dir;

    if (romdirp == &rom_dirs[0] && (env = getenv(*romdirp++)) != NULL) {
        dir = env;
    } else {
realenv:
        if (romdirp != romdend) {
            dir = *romdirp++;
        }
        else {
            return NULL;
        }
    }
    VERBOSE("Looking for ROM named \"%s\" in %s...\n", fname, dir);
    (void) snprintf(buf, sizeof buf, "%s/%s", dir, fname);
    return buf;
}

static void load_rom(void)
{
    /* Load a 12k rom file into place (not provided; get your own!) */
    const char *rompath;
    const char *last_tried_path;
    int err = 0;
    int fd = -1;
    while (fd < 0
            && (rompath = get_try_rom_path(default_romfname)) != NULL) {
        errno = 0;
        last_tried_path = rompath;
        fd = open(rompath, O_RDONLY);
        err = errno;
    }
    if (fd < 0) {
        DIE(1, "Couldn't open ROM file \"%s\": %s\n", last_tried_path, strerror(err));
    } else {
        INFO("FOUND ROM file \"%s\".\n", rompath);
    }

    struct stat st;
    errno = 0;
    err = fstat(fd, &st);
    if (err < 0) {
        err = errno;
        DIE(1, "Couldn't stat ROM file \"%s\": %s\n", last_tried_path, strerror(err));
    }
    if (st.st_size != 12 * 1024) {
        // XXX expected size will need to be configurable
        DIE(0, "ROM file \"%s\" has unexpected file size %zu\n",
            last_tried_path, (size_t)st.st_size);
        DIE(1, "  (expected %zu).\n",
            (size_t)(12 * 1024));
    }

    rombuf = mmap(NULL, 12 * 1024, PROT_READ, MAP_PRIVATE, fd, 0);
    if (rombuf == NULL) {
        perror("Couldn't map in ROM file");
        exit(1);
    }
    close(fd); // safe to close now.

    validate_rom(rombuf, 12 * 1024);
}

void mem_init(void)
{
    /* Immitate the on-boot memory pattern. */
    for (size_t z=0; z != ((sizeof membuf)/(sizeof membuf[0])); ++z) {
        if (!(z & 0x2))
            membuf[z] = 0xFF;
    }

    bobbin_test = getenv("BOBBIN_TEST");
    if (bobbin_test) {
        load_test_code();
    }
    else {
        load_rom();
    }
}

byte peek(word loc)
{
    int t = trace_peek(loc);
    if (t >= 0) {
        return (byte) t;
    }

    return peek_sneaky(loc);
}

byte peek_sneaky(word loc)
{
    int t = -1; // trace_peek_sneaky(loc);
    if (t >= 0) {
        return (byte) t;
    }

    if (rombuf && loc >= 0xD000) {
        return rombuf[loc - 0xD000];
    }
    else if (loc >= cfg.amt_ram || (loc >= 0xC000 && loc < 0xD000)) {
        return 0;
    }
    else {
        return membuf[loc];
    }
}

void poke(word loc, byte val)
{
    poke_sneaky(loc, val);
}

void poke_sneaky(word loc, byte val)
{
    // XXX
    membuf[loc] = val;
}

bool mem_match(word loc, unsigned int nargs, ...)
{
    bool status = true;
    va_list args;
    va_start(args, nargs);

    for (; nargs != 0; ++loc, --nargs) {
        if (peek_sneaky(loc) != va_arg(args, int)) {
            status = false;
        }
    }

    va_end(args);
    return status;
}
