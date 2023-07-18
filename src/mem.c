#include "bobbin-internal.h"

/* For the ROM file, and error handling */
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

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

    fread(membuf, 1, 64 * 1024, tf);
}

static void load_rom(void)
{
    /* Load a 12k rom file into place (not provided; get your own!) */
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

    if (rombuf && loc >= 0xD000) {
        return rombuf[loc - 0xD000];
    }
    else if (loc >= 0xC000 && loc < 0xD000) {
        return 0;
    }
    else {
        return membuf[loc];
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
