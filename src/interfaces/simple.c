#include "bobbin-internal.h"

#include <errno.h>
#include <fcntl.h> // open()
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static int saved_char = -1;
static bool interactive;
static bool output_seen;
static int inputfd = -1;
FILE *inf = NULL;
static struct termios ios;
static struct termios orig_ios;
static byte last_char_read;
static byte last_char_consumed;
static bool eof_found = 0;

static enum {
    IM_APPLE = 0,
    IM_CANON,
    // IM_GETLINE, // future GNU getline() feature??
} input_mode;

enum mon_rom_check_status {
    MON_ROM_NOT_CHECKED,
    MON_ROM_IS_WOZ,
    MON_ROM_NOT_WOZ,
};
bool mon_entered = false;

unsigned char linebuf[256];
unsigned char *lbuf_start = linebuf;
unsigned char *lbuf_end = linebuf;

#define OS_SUPPRESS_NONE    0
#define OS_SUPPRESS_CR      1
#define OS_SUPPRESS_ALL     2
int output_suppressed = OS_SUPPRESS_NONE;

static inline bool is_canon(void)
{
    return (ios.c_lflag & ICANON) != 0;
}

static void transition_tty(void)
{
    lbuf_start = lbuf_end = linebuf;
    // close input, we want to make sure tty is fd 0
    errno = 0;
    close(0);
    int err = errno;
    errno = 0;
    int fd = open("/dev/tty", O_RDONLY);
    if (fd < 0) {
        DIE(1,"Tried to --remain-tty, but couldn't open /dev/tty: %s\n",
            strerror(errno));
    } else if (fd != 0) {
        DIE(1, "Couldn't reopen /dev/tty to fd 0: %s\n",
            strerror(err? err : errno));
    }

    INFO("--remain-tty, switching to tty interface...\n");
    cfg.interface = "tty";
    interfaces_init();
    interfaces_start();
}

static void restore_term(void)
{
    ios = orig_ios;
    int e = tcsetattr(inputfd, TCSANOW, &ios);
    if (e < 0) {
        const char *err = strerror(errno);
        WARN("tcsetattr: %s\n", err);
    }
}

static void set_ios(struct termios *my_ios)
{
    int e = tcsetattr(inputfd, TCSANOW, my_ios);
    if (e < 0) {
        const char *err = strerror(errno);
        WARN("tcsetattr: %s\n", err);
    }
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
    set_ios(&ios);
}

static void set_canon(void)
{
    if (!interactive) return;

    // turn on canonical mode until we hit a newline
    ios.c_lflag |= ICANON | ECHO;
    set_ios(&ios);
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

    // Not a warning... but we really want the user to see this by
    // default. They can shut it up with --quiet
    if (WARN_OK) {
        fprintf(stderr, "\n[Bobbin \"simple\" interactive mode.\n"
                " Ctrl-D at input to exit.\n"
                " Ctrl-C *TWICE* to enter debugger.]\n");
    }
}

// Check for common VT100 left-/right-arrow key bindings
// This won't work everywhere, and may rely on zero delays
// between chars of the escape sequence, which is not
// at all guaranteed.
//
// returns 0 if next isn't a recognized arrow-key escape sequence
// (just a dumb-but-informed guess), and the Apple ][ equivalent key
// if it is.
byte is_arrow_key(void)
{
    byte c = 0;
    if (*lbuf_start == '\x1B' // ESC
        && lbuf_end > lbuf_start + 2
        && (lbuf_start[1] == '[' || lbuf_start[1] == 'O')
        && (lbuf_start[2] == 'C' || lbuf_start[2] == 'D')) {

        if (lbuf_start[2] == 'D') {
            c = 0x88; // backspace / left-arrow
        } else {
            c = 0x95; // right-arrow
        }
    }

    return c;
}

int read_char(void)
{
    int c = -1;

recheck:
    c = -1;
    if (sigint_received) {
        c = 0x83; // Ctrl-C in Apple ][
        if (interactive) {
            if (last_char_consumed == 0x03 || sigint_received > 1) {
                // Take this to mean two consecutive Ctrl-C's, with no
                // intervening user input. This should trigger the debugger.
                dbg_on();
                sigint_received = 0;
                goto recheck; // go again, in case buffered chars
            } else {
                // Everything's fine
            }
        } else if (cfg.remain_after_pipe) {
            // Flush remaining input and switch to interactive.
            lbuf_start = lbuf_end = linebuf;
            set_interactive();
        } else if (cfg.remain_tty) {
            transition_tty();
        } else {
            eof_found = true;
        }
    } else if (lbuf_start < lbuf_end) {
        // We have chars left from a buffered read, grab the next
        //  from that.

        byte a = is_arrow_key();
        if (a != 0) {
            c = a;
        } else {
            c = util_fromascii(*lbuf_start);

            if (c == 0x8D) // CR
                set_noncanon(); // may have just finished a GETLN
        }
    } else if (debugging()) {
        // Don't try to read any characters
    } else {
        errno = 0;
        ssize_t nbytes = read(inputfd, &linebuf, sizeof linebuf);
        if (nbytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            const char *err = strerror(errno);
            DIE(2,"read input failed: %s\n", err);
        } else if (nbytes <= 0) {
            // If < 0, it was just EAGAIN or EWOULDBLOCK,
            // not a "real" error
            if (interactive) {
                if (nbytes == 0 && is_canon()) {
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
            } else if (cfg.remain_tty) {
                transition_tty();
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
        byte a = is_arrow_key();
        if (a != 0) {
            lbuf_start += 3;
        } else {
            ++lbuf_start;
        }
    }
    else {
        // nothing - no keypress was ready
    }
    last_char_consumed = last_char_read;
}

static void iface_simple_init(void)
{
    // Handle input mode
    const char *s = cfg.simple_input_mode;
    if (STREQ(s, "apple")) {
        input_mode = IM_APPLE;
    } else if (STREQ(s, "canonical") || STREQ(s, "fgets")) {
        input_mode = IM_CANON;
    } else {
        DIE(2,"Unrecognized --simple-input value \"%s\".\n", s);
    }
}

static void iface_simple_start(void)
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
        || c == '\t' || c == '\b') {

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

static bool check_is_woz_rom(void)
{
    static enum mon_rom_check_status status = MON_ROM_NOT_CHECKED;
    
    if (status == MON_ROM_NOT_CHECKED) {
        status = mem_match(INT_SETPROMPT, 5, 0x85, 0x33, 0x4C, 0xED, 0xFD) ?
                 MON_ROM_IS_WOZ : MON_ROM_NOT_WOZ;
    }
    return status == MON_ROM_IS_WOZ;
}

static void prompt_wozbasic(void)
{
    // Skip printing the line prompt, IF stdin is not a tty.
    if (check_is_woz_rom() && !interactive) {    //  ^ make sure we're in INT basic
        // It's not a tty. Skip to line fetch.
        suppress_output();
    }
}

static void iface_simple_prestep(void)
{
    if (current_pc() == MON_MONZ) {
        if (!mon_entered) {
            mon_entered = true;
            if (check_is_woz_rom()) {
                // Special kludge: skip monitor at startup,
                // go straight to Woz basic.
                go_to(INT_BASIC);
            }
        }
    }
}

static void iface_simple_step(void)
{
    switch (current_pc()) {
        // XXX these should check that firmware is active
        case MON_COUT1:
            vidout();
            break;
        case MON_NXTCHR: // common part of GETLN used by
                     //  both AppleSoft and Woz basics
            if (!interactive) {
                // Don't want to echo the input when it's piped in.
                suppress_output();
            }
            else if (input_mode == IM_CANON) {
                // Use the terminal's "canonical mode" input handling,
                //  instead of the Apple ]['s built-in handling
                suppress_output();
                set_canon();
            }
            break;
        case MON_GETLNZ:
        case MON_GETLN:
            prompt();
            break;
        case INT_SETPROMPT:
            prompt_wozbasic();
            break;
    }
}

static int iface_simple_peek(word loc)
{
    word a = loc & 0xFFF0;

    if (a == SS_KBD) {
        return read_char();
    } else if (a == SS_KBDSTROBE) {
        consume_char();
    }

    return -1;
}

static void iface_simple_enter_dbg(FILE **in, FILE **out)
{
    if (!interactive) {
        set_interactive();
    }
    set_canon();

    // Set input blocking.
    int flags = fcntl(inputfd, F_GETFL);
    (void) fcntl(inputfd, F_SETFL, flags & ~O_NONBLOCK);
    if (inf != NULL) {
        // Nothing to do, already have the FILE*.
    } else if (inputfd == 0) {
        inf = stdin;
    } else {
        errno = 0;
        inf = fdopen(inputfd, "r");
        if (inf == NULL)
            DIE(2,"fdopen (/dev/tty): %s\n", strerror(errno));
    }

    *in = inf;
    *out = stdout;
}

static void iface_simple_exit_dbg(void)
{
    set_noncanon();
    int flags = fcntl(inputfd, F_GETFL);
    // Set non-blocking.
    (void) fcntl(inputfd, F_SETFL, flags | O_NONBLOCK);
}

IfaceDesc simpleInterface = {
    .init = iface_simple_init,
    .start= iface_simple_start,
    .prestep = iface_simple_prestep,
    .step = iface_simple_step,
    .peek = iface_simple_peek,
    .enter_dbg = iface_simple_enter_dbg,
    .exit_dbg = iface_simple_exit_dbg,
};
