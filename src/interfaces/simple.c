#include "bobbin-internal.h"

#include <errno.h>
#include <fcntl.h> // open()
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int saved_char = -1;
static bool interactive;
static bool output_seen;
int inputfd = -1;
struct termios ios;
struct termios orig_ios;
bool canon = 1;
byte last_char_read;
bool eof_found = 0;

unsigned char linebuf[256];
unsigned char *lbuf_start = linebuf;
unsigned char *lbuf_end = linebuf;

#define OS_SUPPRESS_NONE    0
#define OS_SUPPRESS_CR      1
#define OS_SUPPRESS_ALL     2
int output_suppressed = OS_SUPPRESS_NONE;

static void restore_term(void)
{
    ios = orig_ios;
    int e = tcsetattr(inputfd, TCSANOW, &ios);
    if (e < 0) {
        const char *err = strerror(errno);
        WARN("tcsetattr: %s", err);
    }
    canon = 1;
}

static void set_noncanon(void)
{
    if (!interactive) return;

    // restore non-canonical mode (char-by-char input)
    ios.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
    ios.c_iflag |= IXANY; // any key can recover from Ctrl-S
                          //  - this is what an Apple does
    ios.c_cc[VMIN] = 0;
    ios.c_cc[VTIME] = 0;
    int e = tcsetattr(inputfd, TCSANOW, &ios);
    if (e < 0) {
        const char *err = strerror(errno);
        WARN("tcsetattr: %s", err);
    }
    canon = 0;
}

static void set_canon(void)
{
    if (!interactive) return;

    // turn on canonical mode until we hit a newline
    ios.c_lflag |= ICANON | ECHO;
    errno = 0;
    int e = tcsetattr(inputfd, TCSANOW, &ios);
    if (e < 0) {
        const char *err = strerror(errno);
        WARN("tcsetattr: %s", err);
    }
    canon = 1;
}


static void set_interactive(void)
{
    // Called either at the very beginning, when stdin is a terminal
    //  (and "simple" interface selected), or else when switching to
    //  terminal input after redirected input (from a pipe or file)
    //  is exhausted (and --remain-after-pipe is set)
    interactive = true;
    errno = 0;
    const char *err;
    if (1 || !isatty(inputfd)) {
        inputfd = open("/dev/tty", O_RDONLY | O_NONBLOCK);
        if (inputfd < 0) {
            err = strerror(errno);
            DIE(1,"couldn't open /dev/tty: %s\n", err);
        }
    }

    errno = 0;
    int e = tcgetattr(inputfd, &ios);
    if (e < 0) {
        err = strerror(errno);
        DIE(1,"tcgetattr: %s\n", err);
    }
    orig_ios = ios;

    atexit(restore_term);

    set_noncanon();

    printf("\n[Bobbin \"simple\" interactive mode.\n"
           " Ctrl-D at input to exit.]\n");
}

int read_char(void)
{
    int c = -1;

    if (sigint_received) {
        c = 0x83; // Ctrl-C in Apple ][
        if (interactive) {
            // Everything's fine
        } else if (cfg.remain_after_pipe) {
            // Flush remaining input and switch to interactive.
            lbuf_start = lbuf_end = linebuf;
            set_interactive();
        } else {
            eof_found = true;
        }
    } else if (lbuf_start < lbuf_end) {
        // We have chars left from a buffered read, grab the next
        //  from that.
        if (*lbuf_start == '\n')
            set_noncanon(); // may have just finished a GETLN
        c = util_fromascii(*lbuf_start);
    } else {
reread:
        errno = 0;
        ssize_t nbytes = read(inputfd, &linebuf, sizeof linebuf);
        if (nbytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            const char *err = strerror(errno);
            DIE(2,"read input failed: %s\n", err);
        } else if (nbytes <= 0) {
            // If < 0, it was just EAGAIN or EWOULDBLOCK,
            // not a "real" error
            if (interactive) {
                if (nbytes == 0 && canon) {
                    // 0 chars read in canonical mode.
                    // According to SUSv4, in non-canonical mode, a
                    // non-blocking terminal read may return 0 bytes instead
                    // of setting errno to EAGAIN and returning -1.
                    // But canonical mode must do -1/EAGAIN.
                    //
                    // I suspect a good Unix will always do -1/EAGAIN,
                    // but to be safe we will only treat a 0-read as EOF
                    // if we were in canonical mode. We'll look for an
                    // explicit Ctrl-D on non-canonical input, to handle
                    // that.
                    eof_found = true; // defer until "consumed"
                    c = 0x8D; // fake "ready-to-read" chr: ensure consumption
                } else {
                    // No input ready at terminal, just return the last
                    // char read, but with byte unset to indicate invalid
                    c = last_char_read;
                }
            } else if (cfg.remain_after_pipe) {
                set_interactive();
            } else {
                // End of redirected input and not remaining after.
                eof_found = true;
                c = 0x8D; // fake "ready-to-read" chr: ensure consumption
            }
        } else {
            lbuf_start = linebuf;
            lbuf_end = linebuf + nbytes;
            if (*lbuf_start == '\n')
                set_noncanon(); // may have just finished an (empty?) GETLN
            if (interactive && nbytes == 1 && *lbuf_start == 0x04) {
                // Ctrl-D read from terminal. Treat as EOF.
                eof_found = true;
                c = 0x8D; // fake "ready-to-read" chr: ensure consumption
            }
            c = util_fromascii(*lbuf_start);
        }
    }
    if (c >= 0) last_char_read = c & 0x7f;

    return c;
}

void consume_char(void)
{
    if (eof_found) {
        // Exit gracefully.
        putchar('\n');
        exit(0);
    }
    if (sigint_received) {
        sigint_received = 0;
    } else if (lbuf_start < lbuf_end) {
        if (output_suppressed == OS_SUPPRESS_ALL
            && (*lbuf_start == '\n' || *lbuf_start == '\r')) {
            output_suppressed = OS_SUPPRESS_CR;
        }
        ++lbuf_start;
    }
    // else nothing - no keypress was ready
}

static void iface_simple_init(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    inputfd = 0;
    if (isatty(0)) {
        set_interactive();
    }
}

void vidout(void)
{
    // Output a character when COUT1 is called
    int suppress = output_suppressed;
    if (suppress == OS_SUPPRESS_CR) {
        // Regardless of what we do with this character
        // (emit, don't emit), we must stop suppressing.
        output_suppressed = OS_SUPPRESS_NONE;
    }
    int c = util_toascii(ACC);
    if (c < 0) return;

    if (suppress == OS_SUPPRESS_ALL)
        return;

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
        if (suppress == OS_SUPPRESS_CR) {
            // Don't emit
        }
        else if (interactive || output_seen) {
            putchar('\n');
        }
    }
}

static void suppress_output(void)
{
    // Suppress output until current emulated routine returns.
    // Can't do that by waiting for PC to hit known RTS locations
    // for the return: both DOS and ProDOS circumvent GETLN's
    // normal return, and just *reset the stack*!!
    //
    // We'll suppress output until we read (and consume!) a carriage
    // return. Then we'll suppress one more output (if it matches
    // a carriage return), and stop suppressing.
    //
    // We could instead probably just suppress until the stack pointer
    // has increased beyond a saved value...
    output_suppressed = OS_SUPPRESS_ALL;
}

static void prompt(void)
{
    // Skip printing the line prompt, IF stdin is not a tty.
    if (!interactive) {
        // It's not a tty. Skip to line fetch.
        suppress_output();
    }
}

static void prompt_wozbasic(void)
{
    // Skip printing the line prompt, IF stdin is not a tty.
    if (mem_match(current_instruction, 5, 0x85, 0x33, 0x4C, 0xED, 0xFD)
            && !interactive) {    //  ^ make sure we're in INT basic
        // It's not a tty. Skip to line fetch.
        suppress_output();
    }
}

static void iface_simple_step(void)
{
    switch (current_instruction) {
        // XXX these should check that firmware is active
        case 0xFDF0:
            vidout();
            break;
        //case 0xFD6F:
        case 0xFD75: // common part of GETLN used by
                     //  both AppleSoft and Woz basics
            suppress_output();
            set_canon();
            break;
        case 0xFD67:
        case 0xFD6A:
            prompt();
            break;
        case 0xE006:
            prompt_wozbasic();
            break;
    }
}

static int iface_simple_peek(word loc)
{
    word a = loc & 0xFFF0;

    if (a == 0xC000) {
        return read_char();
    } else if (a == 0xC010) {
        consume_char();
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
