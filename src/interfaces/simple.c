#include "bobbin-internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int saved_char = -1;
static bool interactive;
static bool output_seen;
FILE *insrc;

static void iface_simple_init(void)
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

        output_seen = true;
        putchar(c);
    }
    else if (c == '\r') {
        /* May wish to suppress newline issued at $F168 (from cold
           start), and the one at $D43C. The latter is probably
           a dependable location, but the cold-start one may not be.
           Perhaps just suppress all newlines until the first time a
           non-newline is encountered? */
        if (interactive || output_seen)
            putchar('\n');
    }
}

unsigned char linebuf[256];
void getln(void)
{
    char *s;
    word bufloc = 0x200;

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
            byte prompt = peek_sneaky(0x33);
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
            poke_sneaky(bufloc++, cout);
        }
    }
    poke_sneaky(bufloc, 0x8D); // RETURN character

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

static void iface_simple_step(void)
{
    switch (current_instruction) {
        // XXX these should check that firmware is active
#if 0
        case 0xD43C:
            if (!interactive) go_to(0xD43F); // skip CR before prompt
            break;
#endif
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

static int iface_simple_peek(word loc)
{
    word a = loc & 0xFFF0;

    if (!sigint_received) {
        return -1;
    } else if (a == 0xC000) {
        return 0x83; // Ctrl-C on Apple ][
    } else if (a == 0xC010) {
        sigint_received = 0;
        return 0x00; // s/b bus noise
    }

    return -1;
}

static int iface_simple_poke(word loc, byte val)
{
    return -1;
}

IfaceDesc simpleInterface = {
    iface_simple_init,
    iface_simple_step,
    iface_simple_peek,
    iface_simple_poke
};
