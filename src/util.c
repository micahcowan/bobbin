//  util.c
//
//  Copyright (c) 2023-2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

const char *get_file_ext(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (ext == NULL) {
        return strrchr(path, '\0');
    }
    return ext+1;
}

void *xalloc(size_t sz)
{
    errno = 0;
    void *obj = malloc(sz);
    if (obj == NULL) {
        DIE(1, "malloc: %s\n", strerror(errno));
    }
    return obj;
}

int mmapfile(const char *fname, byte **buf, size_t *sz, int flags)
{
    int err;
    int fd;

    *buf = NULL;

    errno = 0;
    fd = open(fname, flags);
    if (fd < 0) {
        return errno;
    }

    struct stat st;
    errno = 0;
    err = fstat(fd, &st);
    if (err < 0) {
        err = errno;
        goto bail;
    }

    errno = 0;
    int protect = PROT_READ;
    int mflags = MAP_PRIVATE;
    if (flags & O_RDWR || flags & O_WRONLY) {
        protect |= PROT_WRITE;
        mflags = MAP_SHARED;
    }
    *buf = mmap(NULL, st.st_size, protect, mflags, fd, 0);
    if (*buf == NULL) {
        err = errno;
        goto bail;
    }
    close(fd); // safe to close now.

    *sz = st.st_size;
    return 0;
bail:
    close(fd);
    return err;
}

bool util_isflashing(int c)
{
    return !(swget(ss, ss_altcharset) || swget(ss, ss_eightycol))
        && c >= 0x40 && c < 0x80;
}

bool util_isreversed(int c, bool flash)
{
    if (!(swget(ss, ss_altcharset) || swget(ss, ss_eightycol))) {
        return (c < 0x40 || (flash && c < 0x80));
    } else {
        return (c < 0x80 && (!machine_has_mousetext() || c < 0x40 || c >= 0x60));
    }
}

int util_todisplay(int c)
{
    if (c < 0) return c;

    if (!machine_is_iie()) {
        c &= 0x3F;
        c ^= 0x20;
        c += 0x20;
    } else if (c >= 0xA0) {
        c -= 0x80;
    } else if (c >= 0x80) {
        c -= 0x40;
    } else if (c >= 0x60) {
        if (swget(ss, ss_altcharset) || swget(ss, ss_eightycol)) {
            // already lowercase
        } else {
            c -= 0x40;
        }
    } else if (c >= 0x40) {
        if (machine_has_mousetext() && (swget(ss, ss_altcharset)
                                        || swget(ss, ss_eightycol))) {
            // Apple IIe MouseText characters ($40-$5F) from Wikipedia table
            // Using single-width Unicode characters where possible
            static const char mousetext_chars[32] = {
                '@',     // $40 - Apple logo (open) - use @ as fallback
                'A',     // $41 - Apple logo (solid) - use A as fallback  
                'H',     // $42 - Hourglass - use H as fallback
                'T',     // $43 - Timer/clock - use T as fallback
                'Y',     // $44 - Checkmark - use Y as fallback
                'N',     // $45 - X mark - use N as fallback
                '<',     // $46 - Running man left - use < as fallback
                '>',     // $47 - Running man right - use > as fallback
                '#',     // $48 - Solid rectangle
                '[',     // $49 - Left half block
                ']',     // $4A - Right half block
                '-',     // $4B - Upper half block
                '_',     // $4C - Lower half block
                '/',     // $4D - Lower right triangle
                '\\',    // $4E - Lower left triangle  
                '\\',    // $4F - Upper left triangle
                '/',     // $50 - Upper right triangle
                '.',     // $51 - Ellipsis - use . as fallback
                'v',     // $52 - Down arrow
                '^',     // $53 - Up arrow
                '>',     // $54 - Right arrow
                '<',     // $55 - Left arrow
                'R',     // $56 - Return symbol - use R as fallback
                '=',     // $57 - Horizontal line
                '%',     // $58 - Pattern/texture
                '*',     // $59 - Pattern/texture
                '+',     // $5A - Plus/cross
                'o',     // $5B - Circle/bullet
                '|',     // $5C - Vertical bar
                '!',     // $5D - Exclamation
                'X',     // $5E - X mark
                '_'      // $5F - Underscore
            };
            // Return the MouseText character
            return (int)(unsigned char)mousetext_chars[c - 0x40];
        } else {
            // Uppercase. Good as it is
        }
    } else {
        c = (c ^ 0x20) + 0x20;
    }
    if (machine_is_iie() && c == 0x7F) c = '#'; // display DEL as an asterisk
    return c;
}

int util_toascii(int c)
{
    if (c < 0) return c;
    c &= 0x7F;
    if (!machine_is_iie() && c >= 0x60)
        c -= 0x20;
    return c & 0x7F;
}

int util_fromascii(int c)
{
    if (c == '\n') return 0x8D; // RETURN char
    if (c == 0x7f) return 0x88; // BS char
    if (!machine_is_iie() && !cfg.tokenize && c >= 0x60 && c != 0x7f)
        c &= 0x5F; // make uppercase (~ some punctuation changes)
    return c | 0x80;
}

int util_isprint(int c)
{
    if (c < 0) return c;
    return c >= 0x20 && c < 0x7F;
}

void util_print_state(FILE *f, word pc, Registers *reg)
{
    static const char fnams[] = "CZIDBUVN";
    // printf("\n12345678901234567890123456789012345678901234567890\n");

    // Print registers
    fprintf(f, "ACC: %02X  X: %02X  Y: %02X  SP: %02X", // takes 31 chars
           reg->a, reg->x, reg->y, reg->sp);
    fprintf(f, "%8c", ' '); // bring it to 39 (first flag starts on #41)

    // Print status flags
    for (int i=7; i != -1; --i) {
        //if (i == PUNUSED) continue;
        fputc(' ', f);
        char c = fnams[i];
        if (RPTEST(reg->p,i)) {
            fprintf(f, " [%c]", c);
        } else {
            fprintf(f, "  %c ", c);
        }
    }
    fputc('\n', f);

    // Print stack
    byte sp = reg->sp - 3;
    fprintf(f, "STK: $1%02X:", sp);
    for (int i=0; i != 13; ++i) {
        if (!sp) fprintf(f, "  |");
        if (sp == reg->sp)
            fprintf(f, "  (%02X)", peek_sneaky(WORD(sp++,0x1)));
        else
            fprintf(f, "  %02X", peek_sneaky(WORD(sp++,0x1)));
    }
    fputc('\n', f);

    // Print current location and instruction
    (void) print_disasm(f, pc, reg);
}

void util_reopen_stdin_tty(int flags)
{
    // close input, we want to make sure tty is fd 0
    errno = 0;
    close(0);
    int err = errno;
    errno = 0;
    int fd = open("/dev/tty", flags);
    if (fd < 0) {
        DIE(1,"Couldn't open /dev/tty: %s\n",
            strerror(errno));
    } else if (fd != 0) {
        DIE(1, "Couldn't reopen /dev/tty to fd 0: %s\n",
            strerror(err? err : errno));
    }
}
