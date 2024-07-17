//  interfaces/tty.c
//
//  Copyright (c) 2023 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/ioctl.h>

#include <curses.h>

#define DRIVE_INDICATOR_Y   24
#define DRIVE_INDICATOR_X   3
static const word text_size = 0x400;
static byte text_page = 0x4;
static int disk_active = 0;

static WINDOW *msgwin;
static bool saved_flash;
static bool refresh_overlay = false;
static bool refresh_all = false;
static bool altcharset = false;
static const unsigned long overlay_wait = 120; // 2 seconds
static const unsigned long overlay_long_wait = 600; // 10 seconds
static unsigned long overlay_timer = 0;
static int msg_attr;
static int cmd_attr;
static int disk_attr;
static int badterm_attr;
static int cols = 40;
static byte typed_char = '\0';

static void draw_border(void);
static void do_overlay(int offset);
static void repaint_flash(bool flash);
static void refresh_video(bool flash);
static void clear_overlay(void);
static void do_overlay_timer(void);
static void if_tty_start(void);
static void if_tty_peek(Event *e);
static void if_tty_poke(Event *e);
static void if_tty_unhook(void);
static void redraw(bool force, int overlay_offset);
static void breakout(void);
static void if_tty_frame(void);
static void if_tty_step(void);
static void if_tty_display_touched(void);
static bool if_tty_squawk(int level, bool cont, const char *fmt, va_list args);

static void tty_atexit(void)
{
    (void) endwin();
}

static word get_line_base(byte page, byte y)
{
    byte hi = ((y >> 1) & 0x03) | page;
    byte lo = ((y & 0x18) + (((y % 2) == 1)? 0x80 : 0));
    lo |= (lo << 2); // *= 3
    return WORD(lo, hi);
}

static byte get_line_for_addr(word loc)
{
    loc &= 0x03F8; // discard uninteresting high bits
    // lowest bit (0) of the line # is loc & 0x0080
    // bits 1 and 2 are loc & 0x0300
    byte y = (loc & 0x0380) >> 7;
    // The remaining high bytes are determined by ranges
    byte lo = loc & 0x7F;
    y |= lo < 0x28? 0
        : (lo < 0x50? 0x8 : 0x10);
    return y;
}

static void draw_border(void)
{
    int y, x;
    getmaxyx(stdscr, y, x);
    if (x > cols) {
        move(0,cols);
        vline('|', y >= 25? 25: y);
    }
    if (y > 24) {
        move(24,0);
        hline('-', x >= (cols)? (cols): x);
    }
    if (x > cols && y > 24) {
        move(24, cols);
        addch('+');
    }
    // Draw disk activity
    if (disk_active != 0 && x > DRIVE_INDICATOR_X && y > DRIVE_INDICATOR_Y) {
        move(DRIVE_INDICATOR_Y, DRIVE_INDICATOR_X);
        attrset(disk_attr);
        addstr(disk_active == 1? " ONE " : " TWO ");
        attrset(A_NORMAL);
    }
}

static void do_overlay(int offset)
{
    int y, x;
    getyx(msgwin, y, x);
    draw_border();
    if (x == 0 && y == 0) return;
    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);
    int err = copywin(msgwin, stdscr, 0, 0, maxy - 1 - y + (x? 0: 1) - offset, 0, maxy-1, maxx-1, false);
}

static void repaint_flash(bool flash)
{
    saved_flash = flash;
    attrset(A_NORMAL);
    if (flash) attron(A_REVERSE);
    for (int y=0; y != 24; ++y) {
        word base = get_line_base(text_page, y);
        for (int x=0; x != 40; ++x) {
            byte c = peek_sneaky(base + x);
            if (util_isflashing(c)) {
                byte cd = util_todisplay(c);
                mvaddch(y, x, cd);
            }
        }
    }
    attrset(A_NORMAL);
}

static void refresh_video80(void)
{
    attrset(A_NORMAL);
    const byte *membuf = getram();
    bool have_aux = cfg.amt_ram > LOC_AUX_START;
    for (int y=0; y != 24; ++y) {
        word base = get_line_base(0x4, y);
        move(y, 0);
        for (byte x=0; x != 80; ++x) {
            bool even = have_aux && (x % 2 == 0);
            byte mx = x >> 1;
            byte c = membuf[(base | (even? LOC_AUX_START : 0)) + mx];
            byte cd = util_todisplay(c);
            bool cfl = util_isreversed(c, false);
            addch(cd | (cfl? A_REVERSE: 0));
        }
    }

    do_overlay(0);
}

static void refresh_video(bool flash)
{
    if (COLS < cols || LINES < 24) {
        clear();
        attron(badterm_attr);
        printw("Terminal too small. Please resize.\n");
        attrset(A_NORMAL);
        return;
    }
    if (cols == 80) {
        refresh_video80();
        return;
    }
    saved_flash = flash;
    attrset(A_NORMAL);
    for (int y=0; y != 24; ++y) {
        word base = get_line_base(text_page, y);
        move(y, 0);
        for (int x=0; x != 40; ++x) {
            byte c = peek_sneaky(base + x);
            byte cd = util_todisplay(c);
            bool cfl = util_isreversed(c, flash);
            addch(cd | (cfl? A_REVERSE: 0));
        }
    }

    do_overlay(0);
}

static void clear_overlay(void)
{
    attrset(A_NORMAL);
    wattron(msgwin, COLOR_PAIR(0));
    werase(msgwin);
    wmove(msgwin, 0, 0);
    //wrefresh(msgwin);
    wattron(msgwin, msg_attr);
    erase();
    refresh_video(saved_flash);
    draw_border();
    refresh();
}

static void do_overlay_timer(void)
{
    if (!overlay_timer) return;
    if (--overlay_timer == 0) clear_overlay();
}

static byte read_char(void) {
    if ((typed_char & 0x80) != 0)
        return typed_char;

    if (sigint_received == 1) {
        sigint_received = 0;
        typed_char = 0x83; // Ctrl-C
        return typed_char;
    }

    int c = getch();
    if (c == ERR) {
        // No char read; nothing to do.
    } else if (c == KEY_BACKSPACE || c == KEY_LEFT) {
        typed_char = 0x88; // Apple's backspace (Ctrl-H)
    } else if (c == KEY_RIGHT) {
        typed_char = 0x95; // Apple's right-arrow (Ctrl-U)
    } else if (c == KEY_UP) {
        typed_char = 0x8B; // Apple's up-arrow (Ctrl-K)
    } else if (c == KEY_DOWN) {
        typed_char = 0x8A; // Apple's down-arrow (newline, Ctrl-J)
#if 0
    } else if (c == 0x1A) {// We're in raw mode; this is Ctrl-Z
        raise(SIGTSTP);
#endif
    } else if (c >= 0 && (c & 0x7F) == c) {
        typed_char = util_fromascii(c & 0x7F);
    }

    return typed_char;
}

static void if_tty_start(void)
{
    if (!cfg.turbo_was_set) {
        cfg.turbo = false; // default
    }
    if (!isatty(STDIN_FILENO)) {
        util_reopen_stdin_tty(O_RDONLY);
    }
    // Init curses
    //use_tioctl(true);
    initscr();
    start_color();
    raw();
    cbreak(); // <-- re-enables signal processing after raw() disables
    intrflush(stdscr, false);
    noecho();
    nonl();
    curs_set(0);

    // Ensure we quit gracefully
    atexit(tty_atexit);

    keypad(stdscr, true);
    nodelay(stdscr, true);

#ifdef NCURSES_VERSION
    ESCDELAY=17; // Wait 1/60th of a second to see if an escape char
                 // is the start of a terminal control sequence/function key
    // ^ This solution is not portable. But there doesn't seem to be a
    // portable way to reduce/eliminate the wait time for an escape
    // sequence. If we have to choose between zero delay, and half a
    // second, we'd rather have zero and lose function-key support.
#endif

    draw_border();

    // Messaging window test
    msgwin = newpad(LINES, COLS);
    if (msgwin == NULL) {
        DIE(1, "curses: Couldn't allocate messages window.\n");
    }
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_CYAN);
    init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(4, COLOR_WHITE, COLOR_RED);
    msg_attr = has_colors()? COLOR_PAIR(1) : A_BOLD;
    cmd_attr = has_colors()? COLOR_PAIR(2) : A_REVERSE;
    badterm_attr = A_BOLD | (has_colors()? COLOR_PAIR(3) : 0);
    disk_attr = (has_colors()? COLOR_PAIR(4) : A_REVERSE) | A_BOLD;
    wattron(msgwin, msg_attr);
    scrollok(msgwin, true);

    // Draw current video memory (garbage)
    refresh_video(false);
    refresh();
}

static void if_tty_switch(void)
{
    word prev_page = text_page;
    if (swget(ss, ss_page2) && !swget(ss, ss_eightycol)) {
        text_page = 0x8;
    } else {
        text_page = 0x4;
    }

    int prevcols = cols;
    if (swget(ss, ss_eightycol)) {
        cols = 80;
        text_page = 0x4;
    } else {
        cols = 40;
        text_page = swget(ss, ss_page2) ? 0x8 : 0x4;
    }

    bool oldcharset = altcharset;
    if (swget(ss, ss_altcharset) != altcharset) {
        altcharset = swget(ss, ss_altcharset);
    }

    refresh_all = refresh_all || (prev_page != text_page || prevcols != cols
        || oldcharset != altcharset);
}

static void if_tty_peek(Event *e)
{
    word a = e->loc & 0xFFF0;

    if (a == SS_KBD) {
        e->val = read_char();
    } else if (a == SS_KBDSTROBE) {
        if (!machine_is_iie()) { // Must be a write, for ]]e and up
            typed_char &= 0x7F; // Clear high-bit (key avail)
            if (sigint_received == 1) sigint_received = 0;
        }
    }
}

static void if_tty_poke(Event *e)
{
    word loc = e->loc;
    byte val = e->val;
    byte x = loc & 0x7F;
    word pg = WORD(0, text_page);
    if ((loc >= pg  && loc < (pg + text_size) && x < 120)
       && !(COLS < cols || LINES < 24)) {
        x %= 40;
        byte y = get_line_for_addr(loc);

#if 0
        int d = util_toascii(val);
        fprintf(stderr, "-----\nWrite '%c' in page, aux is %s.\n", d,
                (e->aloc & LOC_AUX_START) == 0? "false" : "true");
        extern void prsw(void);
        prsw();
#endif
        if (cols == 80) {
            x *= 2;
            if ((e->aloc & LOC_AUX_START) == 0) {
                if (cfg.amt_ram != 0x20000) {
                    // This write is going to aux mem, but
                    //  we don't HAVE aux mem. Ignore it.
                    return;
                }
                x += 1; // main
            }
        } else if ((e->aloc & LOC_AUX_START) != 0) {
            return; // Don't process; it's going somewhere
                    // we aren't displaying.
        }
        int c = util_todisplay(val);
        bool flash = (cols == 80)
            || swget(ss, ss_altcharset)? false : saved_flash;
        if (util_isreversed(val, flash)) c |= A_REVERSE;
        mvaddch(y, x, c);
        if (cols == 80 && cfg.amt_ram <= LOC_AUX_START) {
            // We're in 80-column mode, but don't have an 80-column
            // card! ...Mirror the character to both cells.
            mvaddch(y, x-1, c);
        }
        refresh_overlay = true;
    } else if ((loc & 0xFFF0) == 0xC010) {
        typed_char &= 0x7F;
        if (sigint_received == 1) sigint_received = 0;
    }
}

static void if_tty_unhook(void)
{
    (void) endwin();
}

static void redraw(bool force, int overlay_offset)
{
    clear();
    if (force) refresh();
    refresh_video(saved_flash);
    do_overlay(overlay_offset);
    refresh();
}

int squawk_print(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);

    if_tty_squawk(-1, false, fmt, arg);
    va_end(arg);
    return 0;
}

static void breakout(void)
{
    char buf[1024];

    sigint_received = 0;
    typed_char = 'A'; // something besides Ctrl-C, NOT ready for consumption.

    redraw(true, 2);

    attron(cmd_attr);
    mvprintw(LINES-2, 0, "CMD ENTRY. q = quit. h = help.");
    mvaddch(LINES-1, 0, ':');
    hline(' ', COLS-1);
    refresh();
    //noraw();
    timeout(-1);
    echo();

    flushinp(); // throw out any type-ahead/pasted input
    int err = getnstr(buf, sizeof buf);
    bool handled = command_do(buf, squawk_print);
    if (!handled && buf[0] != '\0') {
        squawk_print("Unrecognized command: %s\n", buf);
    }

    noecho();
    nodelay(stdscr, true);
    attroff(msg_attr);

    redraw(false, 0);
    overlay_timer = overlay_long_wait;
}

static void handle_winch(void)
{
    struct winsize ws;
    errno = 0;
    int status = ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    if (status < 0) {
        WARN("winch: ioctl_tty: %s\n", strerror(errno));
    }
    resizeterm(ws.ws_row, ws.ws_col);
}

static void if_tty_frame(void)
{
    bool flash = text_flash;
    do_overlay_timer();
    if (cols == 80 || swget(ss, ss_altcharset)) flash = false;
    if (flash != saved_flash) {
        repaint_flash(flash);
        refresh_overlay = true;
    }
    if (sigwinch_received) {
        sigwinch_received = false;
        handle_winch();
        refresh_all = true;
    }

    if (refresh_all) {
        refresh_all = false;
        redraw(true, 0);
    } else if (refresh_overlay) {
        refresh_overlay = false;
        do_overlay(0);
        refresh();
    }
}

static void if_tty_step(void)
{
    // XXX more checking s/b done here to make sure we're where we think
    // we are.
    if (current_pc() == 0xFBD9 && cfg.bell) {
        beep();
    }
}

static void if_tty_display_touched(void)
{
    if (stdscr) {
        refresh();
        touchwin(stdscr);
    }
}

static void if_tty_disk_active(int val)
{
    disk_active = val;
    draw_border();
}

static bool if_tty_squawk(int level, bool cont, const char *fmt, va_list args)
{
    if (!msgwin) return false;
    vw_printw(msgwin, fmt, args);
    overlay_timer = overlay_wait;
    refresh_overlay = true;
    return true; // suppress normal (stderr) squawking
}

static void if_tty_event(Event *e)
{
    switch (e->type) {
        case EV_START:
            if_tty_start();
            break;
        case EV_PRESTEP:
            if (sigint_received >= 2
                || (sigint_received == 1 && (typed_char & 0x7F) == 0x03)) {

                breakout();
                sigint_received = 0;
            }
            break;
        case EV_STEP:
            if_tty_step();
            break;
        case EV_POKE:
            if_tty_poke(e);
            break;
        case EV_PEEK:
            if_tty_peek(e);
            break;
        case EV_REBOOT:
        case EV_RESET:
        case EV_SWITCH:
            if_tty_switch();
            break;
        case EV_FRAME:
            if_tty_frame();
            break;
        case EV_UNHOOK:
            if_tty_unhook();
            break;
        case EV_DISPLAY_TOUCH:
            if_tty_display_touched();
            break;
        case EV_DISK_ACTIVE:
            if_tty_disk_active(e->val);

            // If --turbo or --no-turbo weren't explicitly set,
            //  set turbo on during disk stuff.
            // XXX should be an option, probably
            if (cfg.turbo_was_set) {
                // nevermind
            } else if (e->val == 0) {
                cfg.turbo = false;
            } else {
                cfg.turbo = true;
            }
            break;
        default:
            ; // Nothing
    }
}

IfaceDesc ttyInterface = {
    .event = if_tty_event,
    .squawk= if_tty_squawk,
};
