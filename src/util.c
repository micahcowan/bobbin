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
    
    int original_c = c;

    // Mousetext logic moved to correct location after Apple IIe processing

    // Handle control characters, but not inverted characters
    int raw_c = c & 0x7F;
    if (raw_c < 0x20) {
        // Check if this is an inverted character (high bit clear, but represents a printable char)
        // Apple II inverted characters: when a character value has bit 7 clear and bits 6-5 clear,
        // it's an inverted version of the character with bits 6-5 set
        if (c < 0x20) {
            // This is an inverted character in range 0x00-0x1F
            // Add 0x40 to get the base character (0x00 -> 0x40 '@', 0x01 -> 0x41 'A', etc.)
            c = c + 0x40;
        } else if (c >= 0x80 && c < 0xA0) {
            // This is an inverted character in range 0x80-0x9F  
            // The base character is (c & 0x1F) + 0x40
            c = (c & 0x1F) + 0x40;
        } else {
            // True control character - return the low 7 bits
            return raw_c;
        }
        // Don't update raw_c here - let the rest of the function handle the converted character
    }

    if (!machine_is_iie()) {
        c &= 0x3F;
        c ^= 0x20;
        c += 0x20;
    } else {
        // First do high-bit processing for Apple IIe
        if (c >= 0xA0) {
            c -= 0x80;
        } else if (c >= 0x80) {
            c -= 0x40;
        }
        
        // Then handle character ranges based on the transformed value
        if (c >= 0x60) {
            if (swget(ss, ss_altcharset) || swget(ss, ss_eightycol)) {
                // already lowercase - keep as is
            } else {
                // Convert lowercase to uppercase when alternate charset is off
                c -= 0x20;  // Convert lowercase to uppercase
            }
        } else if (c >= 0x40 && c < 0x60) {
            // Characters in the $40-$5F range (@ A-Z [ \ ] ^ _)
            // Check for mousetext: only convert if original character was ALREADY in $40-$5F range
            // This preserves normal text like "DaveX" (from $C0-$DF) as letters
            // while converting true mousetext characters to symbols
            if (machine_has_mousetext() && swget(ss, ss_altcharset)) {
                // Only convert to mousetext if the ORIGINAL memory value was in $40-$5F range
                // AND did not come from normal text ranges like $C0-$DF
                // Normal text like "DaveX" comes from $C0-$DF and should stay as letters
                int orig_masked = original_c & 0x7F;
                bool was_normal_text = (original_c >= 0xC0 && original_c <= 0xDF);  // Normal uppercase
                
                if (orig_masked >= 0x40 && orig_masked < 0x60 && !was_normal_text) {
                    return '@';  // Convert true mousetext characters to @ symbols
                }
            }
            // Otherwise display as normal characters
        } else {
            c = (c ^ 0x20) + 0x20;
        }
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
