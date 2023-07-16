#include "bobbin-internal.h"

#include <stdio.h>

int util_toascii(int c)
{
    if (c < 0) return c;
    return c & 0x7F;
}

int util_isprint(int c)
{
    if (c < 0) return c;
    return c > 0x20 && c < 0x7F;
}

void util_print_state(void)
{
    static const char fnams[] = "CZIDB@VN";
    // printf("\n12345678901234567890123456789012345678901234567890\n");

    // Print registers
    printf("ACC: %02X  X: %02X  Y: %02X  SP: %02X", // takes 31 chars
           ACC, XREG, YREG, SP);
    printf("%8c", ' '); // bring it to 39 (first flag starts on #41)

    // Print status flags
    for (int i=7; i != -1; --i) {
        if (i == PUNUSED) continue;
        putchar(' ');
        char c = fnams[i];
        if (PTEST(i)) {
            printf(" [%c]", c);
        } else {
            printf("  %c ", c);
        }
    }
    putchar('\n');

    // Print current location and instruction
    (void) print_disasm(current_instruction, &theCpu.regs);
}
