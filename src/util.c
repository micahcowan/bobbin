#include "bobbin-internal.h"

#include <stdio.h>

int util_toascii(int c)
{
    if (c < 0) return c;
    return c & 0x7F;
}

int util_fromascii(int c)
{
    if (c == '\n') return 0x8D; // RETURN char
    if (c >= 0x60)
        c &= 0x5F; // make uppercase (~ some punctuation changes)
    return c | 0x80;
}

int util_isprint(int c)
{
    if (c < 0) return c;
    return c >= 0x20 && c < 0x7F;
}

void util_print_state(FILE *f)
{
    static const char fnams[] = "CZIDB@VN";
    // printf("\n12345678901234567890123456789012345678901234567890\n");

    // Print registers
    fprintf(f, "ACC: %02X  X: %02X  Y: %02X  SP: %02X", // takes 31 chars
           ACC, XREG, YREG, SP);
    fprintf(f, "%8c", ' '); // bring it to 39 (first flag starts on #41)

    // Print status flags
    for (int i=7; i != -1; --i) {
        if (i == PUNUSED) continue;
        fputc(' ', f);
        char c = fnams[i];
        if (PTEST(i)) {
            fprintf(f, " [%c]", c);
        } else {
            fprintf(f, "  %c ", c);
        }
    }
    fputc('\n', f);

    // Print stack
    fprintf(f, "STK: $1%02X:  (%02X)", SP, mem_get_byte_nobus(STACK));
    byte sp = SP+1;
    for (int i=0; i != 13; ++i) {
        if (!sp) fprintf(f, "  |");
        fprintf(f, "  %02X", mem_get_byte_nobus(WORD(sp++,0x1)));
    }
    fputc('\n', f);

    // Print current location and instruction
    (void) print_disasm(f, current_instruction, &theCpu.regs);
}
