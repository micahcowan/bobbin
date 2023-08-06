#include "bobbin-internal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

bool util_isflashing(int c)
{
    return !(rstsw.altcharset || rstsw.eightycol) && c >= 0x40 && c < 0x80;
}

bool util_isreversed(int c, bool flash)
{
    if (!(rstsw.altcharset || rstsw.eightycol)) {
        return (c < 0x40 || (flash && c < 0x80));
    } else {
        return (c < 0x80 && (c < 0x40 || c >= 0x60));
    }
}

int util_todisplay(int c)
{
    if (c < 0) return c;

    if (!machine_is_iie()) {
        c &= 0x3F;
        c ^= 0x20;
        c += 0x20;
    }
    else if (c >= 0xA0) {
        c -= 0x80;
    } else if (c >= 0x60) {
        if (c >= 0x80 || !(rstsw.altcharset || rstsw.eightycol)) {
            c -= 0x40;
        } else {
            // already lowercase
        }
    } else if (c >= 0x20) {
        if (rstsw.altcharset || rstsw.eightycol) {
            // mousetext. All @@ for now?
        } else {
            // good as it is
        }
    } else {
        c |= 0x40;
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
