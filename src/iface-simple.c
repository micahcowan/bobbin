#include "bobbin-internal.h"
#include "iface-simple.h"

#include <stdio.h>
#include <stdlib.h>

void iface_simple_instr_init(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
}

void iface_simple_instr_hook(void)
{
    if (PC != 0xFDF0) return;

    // Output a character when COUT1 is called
    printf("%02X\n", (unsigned int)ACC);
    int c = util_toascii(ACC);
    printf("%d\n", c);
    if (c < 0) return;

    if (util_isprint(c)
        || c == '\t') {

        putchar(c);
    }
    else if (c == '\r') {
        putchar('\n');
    }
}

int iface_simple_getb_hook(word loc)
{
    if (current_instruction == 0xFB7C) return -1;
    if ((loc & 0xFFF0) == 0xC000) {
        printf("KEYIN at $%04X. Done.\n",
               (unsigned int)current_instruction);
        exit(0);
    }
    return -1;
}
