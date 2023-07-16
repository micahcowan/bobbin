#include "bobbin-internal.h"

// XXX
#include <fcntl.h>
#include <sys/mman.h>
// XXX foo
#include <stdio.h>
#include <stdlib.h>

static byte membuf[128 * 1024];
static unsigned char *rombuf;

void mem_init(void)
{
    for (size_t z=0; z != ((sizeof membuf)/(sizeof membuf[0])); ++z) {
        if (z & 0x2)
            membuf[z] = 0xFF;
    }
    // XXX
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
    // XXX foo
    if ((loc & 0xFFF0) == 0xC000) {
        fprintf(stderr, "KBD checked at %04X. Done.\n", (unsigned int)PC);
        exit(0);
    }
    // XXX
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
    // XXX
    membuf[loc] = val;
}
