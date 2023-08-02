#include "bobbin-internal.h"

#include <stdlib.h>

#include <curses.h>

static const word text_size = 0x400;
static byte text_page = 0x4;

static WINDOW *win;
static WINDOW *msgwin;
static bool saved_flash;
static bool refresh_overlay = false;
static const unsigned long overlay_wait = 120; // 2 seconds
static unsigned long overlay_timer = 0;
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

static void do_overlay(void)
{
    int y, x;
    getyx(msgwin, y, x);
    if (x == 0 && y == 0) return;
    copywin(msgwin, win, 0, 0, 23 - y + (x? 0: 1), 0, 23, 39, false);
}

static void repaint_flash(bool flash)
{
    saved_flash = flash;
    wattrset(win, A_NORMAL);
    if (flash) wattron(win, A_REVERSE);
    for (int y=0; y != 24; ++y) {
        word base = get_line_base(text_page, y);
        for (int x=0; x != 40; ++x) {
            byte c = peek_sneaky(base + x);
            if (util_isflashing(c)) {
                byte cd = util_todisplay(c);
                mvwaddch(win, y, x, cd);
            }
        }
    }
    wattrset(win, A_NORMAL);
}

static void refresh_video(bool flash)
{
    saved_flash = flash;
    wattrset(win, A_NORMAL);
    for (int y=0; y != 24; ++y) {
        word base = get_line_base(text_page, y);
        wmove(win, y, 0);
        for (int x=0; x != 40; ++x) {
            byte c = peek_sneaky(base + x);
            byte cd = util_todisplay(c);
            bool cfl = util_isreversed(c, flash);
            waddch(win, cd | (cfl? A_REVERSE: 0));
        }
    }

    do_overlay();
}

static void clear_overlay(void)
{
    wattron(msgwin, COLOR_PAIR(0));
    wclear(msgwin);
    wmove(msgwin, 0, 0);
    wrefresh(msgwin);
    wattron(msgwin, COLOR_PAIR(1));
    refresh_video(saved_flash);
}

static void do_overlay_timer(void)
{
    if (!overlay_timer) return;
    if (--overlay_timer == 0) clear_overlay();
}

static void draw_border(void)
{
    move(0,40);
    vline('|', 25);
    move(24,0);
    hline('-',41);
    refresh();
}

static void if_tty_start(void)
{
    if (!cfg.turbo_was_set) {
        cfg.turbo = false; // default
    }
    // Init curses
    initscr();
    start_color();
    raw(); //cbreak();
    noecho();
    nonl();
    curs_set(0);

    // Ensure we quit gracefully
    atexit(tty_atexit);

    // Create a window at the top-left of the screen
    win = subwin(stdscr, 24, 40, 0, 0);
    keypad(win, true);
    nodelay(win, 1);

    draw_border();

    // Messaging window test
    msgwin = newwin(24, 40, 0, 0);
    init_pair(1, COLOR_BLACK, COLOR_CYAN);
    wattron(msgwin, COLOR_PAIR(1));
    wprintw(msgwin, "Here is a line of text.\n");
    wprintw(msgwin, "Here is another!\n");
    wprintw(msgwin, "...and yet another.\n");
    wrefresh(msgwin);
    overlay_timer = overlay_wait;

    // Draw current video memory (garbage)
    refresh_video(false);
}

static void if_tty_peek_or_poke(word loc)
{
    word prev = text_page;
    switch (loc) {
        case 0xC054:
            text_page = 0x04;
            if (prev != text_page)
                refresh_video(saved_flash);
            break;
        case 0xC055:
            text_page = 0x08;
            if (prev != text_page)
                refresh_video(saved_flash);
            break;
    }
}

static int if_tty_peek(word loc)
{
    word a = loc & 0xFFF0;

    if (a == SS_KBD) {
        return typed_char;
    } else if (a == SS_KBDSTROBE) {
        sigint_received = 0;
        typed_char &= 0x7F; // Clear high-bit (key avail)
    } else {
        if_tty_peek_or_poke(loc);
    }

    return -1;
}

static bool if_tty_poke(word loc, byte val)
{
    byte x = loc & 0x7F;
    word pg = WORD(0, text_page);
    if (loc >= pg  && loc < (pg + text_size) && x < 120) {
        x %= 40;
        byte y = get_line_for_addr(loc);

        int c = util_todisplay(val);
        if (util_isreversed(val, saved_flash)) c |= A_REVERSE;
        mvwaddch(win, y, x, c);
        refresh_overlay = true;
    } else {
        if_tty_peek_or_poke(loc);
    }

    return false;
}

static void if_tty_frame(bool flash)
{
    do_overlay_timer();
    if (flash != saved_flash) {
        repaint_flash(flash);
        refresh_overlay = true;
    }
    if (refresh_overlay) {
        refresh_overlay = false;
        do_overlay();
    }
    wrefresh(win);

    // NOTE: does auto-refresh of screen
    // put a regular wrefresh in here if getch() goes away
    int c = wgetch(win);
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
    } else if (c == 0x1A) {// We're in raw mode; this is Ctrl-Z
        raise(SIGTSTP);
    } else if (c == 0x0C) {
        // Ctrl-L = refresh screen
        clear();
        refresh();
        wclear(win);
        refresh_video(flash);
        wrefresh(win);
        draw_border(); // does refresh();
    } else if (c >= 0 && (c & 0x7F) == c) {
        typed_char = util_fromascii(c & 0x7F);
    }
}

static void if_tty_step(void)
{
    // XXX more checking s/b done here to make sure we're where we think
    // we are.
    if (current_pc() == 0xFBD9) {
        beep();
    }
}

static void if_tty_display_touched(void)
{
    if (win) {
        refresh();
        touchwin(win);
    }
}

IfaceDesc ttyInterface = {
    .start = if_tty_start,
    .step  = if_tty_step,
    .poke  = if_tty_poke,
    .peek  = if_tty_peek,
    .frame = if_tty_frame,
    .display_touched = if_tty_display_touched,
};
