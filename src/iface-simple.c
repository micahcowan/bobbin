#include "bobbin-internal.h"
#include "iface-simple.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static bool interactive;
static bool getln_seen;
static struct { bool stay_after_pipe; } cfg;
FILE *insrc;

void iface_simple_instr_init(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    cfg.stay_after_pipe = true;
    if (isatty(0)) {
        interactive = 1;
    }
    insrc = stdin;
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
        /* May wish to suppress newline issued at $F168 (from cold
           start), and the one at $D43C. The latter is probably
           a dependable location, but the cold-start one may not be.
           Perhaps just suppress all newlines until the first time
           GETLN is encountered? */
        if (interactive || getln_seen)
            putchar('\n');
    }
}

unsigned char linebuf[256];
void getln(void)
{
    char *s;
    word bufloc = 0x200;

    getln_seen = 1;

    // Something wants to read a line of text, so do that
reread:
    s = fgets((char *)linebuf, sizeof linebuf, insrc);
    if (s == NULL) {
        if (interactive) {
            putchar('\n');
            exit(0);
        }
        else if (cfg.stay_after_pipe) {
            insrc = fopen("/dev/tty", "r");
            if (!insrc) {
                putchar('\n');
                exit(0);
            }
            interactive = true;

            // Odds are strong that we suppressed the prompt
            // for the current line read. As a hack, read
            // the current prompt char and re-issue it.
            byte prompt = mem_get_byte_nobus(0x33);
            putchar(util_toascii(prompt));
            goto reread;
        }
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

static void prompt(void)
{
    // Skip printing the line prompt, IF stdin is not a tty.
    if (!interactive) {
        // It's not a tty. Skip to line fetch.
        getln();
    }
}

void iface_simple_instr_hook(void)
{
    switch (current_instruction) {
        // XXX these should check that firmware is active
        case 0xD43C:
            if (!interactive) go_to(0xD43F);
            break;
        case 0xFDF0:
            vidout();
            break;
        case 0xFD6F:
            getln();
            break;
        case 0xFD67:
        case 0xFD6A:
            prompt();
            break;
    }
}

int iface_simple_getb_hook(word loc)
{
    return -1;
}
