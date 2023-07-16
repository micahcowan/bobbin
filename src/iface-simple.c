#include "bobbin-internal.h"
#include "iface-simple.h"

#include <stdio.h>
#include <stdlib.h>

static int end_of_file = 0;

void iface_simple_instr_init(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    //setvbuf(stdin,  NULL, _IONBF, 0);
}

void vidout(void)
{
    // Output a character when COUT1 is called
    int c = util_toascii(ACC);
    if (c < 0) return;

    if (util_isprint(c)
        || c == '\t') {

        putchar(c);
    }
    else if (c == '\r') {
        putchar('\n');
    }
}

unsigned char linebuf[256];
void getln(void)
{
    char *s;
    word bufloc = 0x200;

    if (end_of_file) {
        putchar('\n');
        exit(0);
    }

    // Something wants to read a line of text, so do that
    s = fgets((char *)linebuf, sizeof linebuf, stdin);
    if (s == NULL) {
        // Delay program exit until next input prompt
        ++end_of_file;
    }
    else {
        // Put it into $200, as GETLN would
        for (; *s && *s != '\n'; ++s) {
            unsigned char cin, cout;
            cin = *s;
            cout = util_fromascii(cin);
            mem_put_byte_nobus(bufloc++, cout);
        }
    }
    mem_put_byte_nobus(bufloc, 0x8D); // RETURN character

    // Set PC to return from GETLN (COUT1)
    PC = 0xFDFF;
    XREG = bufloc - 0x200; // X must point at final RETURN
}

void iface_simple_instr_hook(void)
{
    switch (current_instruction) {
        case 0xFDF0:
            vidout();
            break;
        case 0xFD6F:
            getln();
            break;
    }
}

int iface_simple_getb_hook(word loc)
{
    return -1;
}
