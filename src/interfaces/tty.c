#include "bobbin-internal.h"

#include <stdlib.h>

#include <curses.h>

static WINDOW *win;
static bool saved_flash;

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

static void if_tty_step(void) {
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
    if (c == 'q' || c == 'Q')
        exit(0);
}

IfaceDesc ttyInterface = {
    .start = if_tty_start,
    .step = if_tty_step,
    .frame = if_tty_frame,
};
