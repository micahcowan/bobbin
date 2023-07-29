#include "bobbin-internal.h"

#include <stdlib.h>

#include <curses.h>

static WINDOW *win;
static bool saved_flash;
static byte typed_char = '\0';

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

static void repaint_flash(bool flash)
{
    saved_flash = flash;
    wattrset(win, A_NORMAL);
    if (flash) wattron(win, A_REVERSE);
    for (int y=0; y != 24; ++y) {
        word base = get_line_base(4,y);
        for (int x=0; x != 40; ++x) {
            byte c = peek_sneaky(base | x);
            if (util_isflashing(c)) {
                byte c = util_todisplay(peek_sneaky(base | x));
                mvwaddch(win, y, x, c);
            }
        }
    }
    wattrset(win, A_NORMAL);
}

static void refresh_video(bool flash)
{
    saved_flash = flash;
    wattrset(win, A_NORMAL);
    bool last_was_flash = false;
    for (int y=0; y != 24; ++y) {
        word base = get_line_base(4,y);
        wmove(win, y, 0);
        for (int x=0; x != 40; ++x) {
            byte c = peek_sneaky(base | x);
            byte cd = util_todisplay(c);
            bool cfl = util_isreversed(c, flash);
            if (cfl != last_was_flash) {
                last_was_flash = cfl;
                if (cfl) {
                    wattron(win, A_REVERSE);
                } else {
                    wattroff(win, A_REVERSE);
                }
            }
            waddch(win, cd);
        }
    }
    wattrset(win, A_NORMAL);
}

static void if_tty_start(void)
{
    if (!cfg.turbo_was_set) {
        cfg.turbo = false; // default
    }
    // Init curses
    initscr();
    cbreak();
    noecho();
    nonl();
    curs_set(0);

    // Ensure we quit gracefully
    atexit(tty_atexit);

    // Create a window at the top-left of the screen
    win = subwin(stdscr, 24, 40, 0, 0);
    keypad(win, true);
    nodelay(win, 1);
    nodelay(stdscr, 1);

    // Draw current video memory (garbage)
    refresh_video(false);
}

static int if_tty_peek(word loc)
{
    word a = loc & 0xFFF0;

    if (a == SS_KBD) {
        return typed_char;
    } else if (a == SS_KBDSTROBE) {
        sigint_received = 0;
        typed_char &= 0x7F; // Clear high-bit (key avail)
    }

    return -1;
}

static bool if_tty_poke(word loc, byte val)
{
    byte x = loc & 0x7F;
    if (loc >= 0x400 && loc < 0x800 && x < 120) {
        x %= 40;
        byte y = get_line_for_addr(loc);

        int c = util_todisplay(val);
        bool f = util_isflashing(val) && saved_flash;
        if (f) wattron(win, A_REVERSE);
        mvwaddch(win, y, x, c);
        if (f) wattroff(win, A_REVERSE);
    }
    return false;
}

static void if_tty_frame(bool flash)
{
    if (flash != saved_flash) {
        repaint_flash(flash);
    }
    wrefresh(win);

    // NOTE: does auto-refresh of screen
    // put a regular wrefresh in here if getch() goes away
    int c = getch();
    if (sigint_received >= 2
        || ((sigint_received != 0 || c == '\x03') && ((typed_char & 0x7F) == '\x03'))) {
        exit(0);
    }
    if (sigint_received > 0) {
        typed_char = 0x83; // Ctrl-C
    }
    if (c == ERR) {
        // No char read; nothing to do.
    } else if (c == KEY_BACKSPACE || c == KEY_LEFT) {
        typed_char = 0x88; // Apple's backspace (Ctrl-H)
    } else if (c == KEY_RIGHT) {
        typed_char = 0x95; // Apple's right-arrow (Ctrl-U)
    } else if (c >= 0 && (c & 0x7F) == c) {
        typed_char = (0x80 | (c & 0x7F));
    }
}

IfaceDesc ttyInterface = {
    .start = if_tty_start,
    .poke  = if_tty_poke,
    .peek  = if_tty_peek,
    .frame = if_tty_frame,
};
