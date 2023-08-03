#include "bobbin-internal.h"

static const char cmd_help[] = "\
h, help\n\
    print this message.\n\
q, quit\n\
    exit bobbin.\n\
r, w\n\
    send reset (warm).\n\
rr\n\
    send COLD reset.\n\
m\n\
    invoke the Apple ][ monitor.\n\
";

bool command_do(const char *line, printer pr)
{
    bool handled = true;
#define HAVE(s) (STREQ(line,(s)))
    if (HAVE("m")) {
        // Swap ourselves out for the built-in Apple II system
        // monitor!
        pr("Switching to monitor.\n");
        // Behave as if it were a BRK.
        // Push stuff to stack...
        stack_push_sneaky(HI(PC));
        stack_push_sneaky(LO(PC));
        stack_push_sneaky(PFLAGS | PMASK(PUNUSED) | PMASK(PBRK));
        go_to(WORD(peek_sneaky(VEC_BRK),peek_sneaky(VEC_BRK+1)));
        // ^ Note: some autostart ROMs have
        // OLDRST instead of BREAK, in VEC_BRK, with the result that
        // PC and the other registers will NOT be printed on entry
        // into the system monitor.
    } else if (HAVE("r") || HAVE("w")) {
        pr("Sending reset.\n");
        cpu_reset();
    } else if (HAVE("rr")) {
        pr("Sending COLD reset.\n");
        cpu_reset();
        // Hard reset. Invalidate the user reset vector directly
        // (rather than doing open-apple emulation or something)
        // Just copy the high byte of the vector into the
        // "powered up" check; then it can't possibly be the
        // correctly XOR-ed version.
        byte b = peek_sneaky(LOC_SOFTEV+1);
        poke_sneaky(LOC_PWREDUP, b);
    } else if (HAVE("^C")) {
        // XXX in future this will be replaced by a "send" command
        // that can do other things besides just ^C. ^? or ^D for
        // instance
        // Send an interrupt back through to the emulation, and
        // continue.
        sigint_received = 1;
    } else if (HAVE("q") || HAVE("quit")) {
        iface_unhook();
        printf("Exiting.\n"); // Don't use pr
        exit(0);
    } else if (HAVE("h") || HAVE("help")) {
        pr("%s", cmd_help);
    } else {
        handled = false;
    }
#undef HAVE
    return handled;
}
