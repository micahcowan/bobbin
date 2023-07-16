#include "bobbin-internal.h"

/* For the ROM file, and error handling */
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

// Enough of a RAM buffer to provide 128k
static byte membuf[128 * 1024];

// Pointer to firmware, mapped into the Apple starting at $D000
static unsigned char *rombuf;

void mem_init(void)
{
    /* Immitate the on-boot memory pattern. */
    for (size_t z=0; z != ((sizeof membuf)/(sizeof membuf[0])); ++z) {
        if (z & 0x2)
            membuf[z] = 0xFF;
    }

    /* Load a 12l rom file into place (not provided; get your own!) */
    int fd = open("apple2.rom", O_RDONLY);
    if (fd < 0) {
        perror("Couldn't open ROM file");
        exit(2);
    }

    rombuf = mmap(NULL, 12 * 1024, PROT_READ, MAP_PRIVATE, fd, 0);
    if (rombuf == NULL) {
        perror("Couldn't map in ROM file");
        exit(1);
    }
}

byte mem_get_byte(word loc)
{
    int t = trace_mem_get_byte(loc);
    if (t >= 0) {
        return (byte) t;
    }

    return mem_get_byte_nobus(loc);
}

byte mem_get_byte_nobus(word loc)
{
    int t = -1; // trace_mem_get_byte_nobus(loc);
    if (t >= 0) {
        return (byte) t;
    }
    if (loc < 0xC000) {
        return membuf[loc];
    }
    else if (loc < 0xD000) {
        return 0;
    }
    else {
        return rombuf[loc - 0xD000];
    }
}

void mem_put_byte(word loc, byte val)
{
    mem_put_byte_nobus(loc, val);
}

void mem_put_byte_nobus(word loc, byte val)
{
    // XXX
    membuf[loc] = val;
}
