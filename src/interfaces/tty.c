#include "bobbin-internal.h"

#include <stdarg.h>
#include <stdlib.h>

#include <curses.h>

static const word text_size = 0x400;
static byte text_page = 0x4;

static WINDOW *msgwin;
static bool saved_flash;
static bool refresh_overlay = false;
static const unsigned long overlay_wait = 120; // 2 seconds
static const unsigned long overlay_long_wait = 600; // 10 seconds
static unsigned long overlay_timer = 0;
static int msg_attr;
static int cmd_attr;
static byte typed_char = '\0';

static void draw_border(void);
static void do_overlay(int offset);
static void repaint_flash(bool flash);
static void refresh_video(bool flash);
static void clear_overlay(void);
static void do_overlay_timer(void);
static void if_tty_start(void);
static void if_tty_peek_or_poke(word loc);
static int if_tty_peek(word loc);
static bool if_tty_poke(word loc, byte val);
static void if_tty_unhook(void);
static void redraw(bool force, int overlay_offset);
static void breakout(void);
static void if_tty_frame(bool flash);
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
    if (x > 40) {
        move(0,40);
        vline('|', y >= 25? 25: y);
    }
    if (y > 24) {
        move(24,0);
        hline('-', x >= 41? 41: x);
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

static void refresh_video(bool flash)
{
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

static void if_tty_start(void)
{
    if (!cfg.turbo_was_set) {
        cfg.turbo = false; // default
    }
    // Init curses
    //use_tioctl(true);
    initscr();
    start_color();
    raw(); //cbreak();
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
    msg_attr = has_colors()? COLOR_PAIR(1) : A_BOLD;
    cmd_attr = has_colors()? COLOR_PAIR(2) : A_REVERSE;
    wattron(msgwin, msg_attr);
    scrollok(msgwin, true);

    // Draw current video memory (garbage)
    refresh_video(false);
    refresh();
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
        byte saved_c = typed_char;
        if (!machine_is_iie()) // Must be a write, for ]]e and up
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
        mvaddch(y, x, c);
        refresh_overlay = true;
    } else if ((loc & 0xFFF0) == 0xC010) {
        typed_char &= 0x7F;
    } else {
        if_tty_peek_or_poke(loc);
    }

    return false;
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
    typed_char = 0;

    redraw(true, 2);

    attron(cmd_attr);
    mvprintw(LINES-2, 0, "CMD ENTRY. q = quit. h = help.");
    mvaddch(LINES-1, 0, ':');
    hline(' ', COLS-1);
    refresh();
    //noraw();
    timeout(-1);
    echo();

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

static void if_tty_frame(bool flash)
{
    do_overlay_timer();
    if (flash != saved_flash) {
        repaint_flash(flash);
        refresh_overlay = true;
    }
    if (refresh_overlay) {
        refresh_overlay = false;
        do_overlay(0);
        refresh();
    }

    // NOTE: does auto-refresh of screen
    // put a regular wrefresh in here if getch() goes away
    int c = getch();
    if (sigint_received >= 2
        || ((sigint_received != 0 || c == '\x03') && ((typed_char & 0x7F) == '\x03'))) {
        breakout();
        sigint_received = 0;
    } else if (sigint_received > 0) {
        typed_char = 0x83; // Ctrl-C
    } else if (c == ERR) {
        // No char read; nothing to do.
    } else if (c == KEY_BACKSPACE || c == KEY_LEFT) {
        typed_char = 0x88; // Apple's backspace (Ctrl-H)
    } else if (c == KEY_RIGHT) {
        typed_char = 0x95; // Apple's right-arrow (Ctrl-U)
    } else if (c == 0x1A) {// We're in raw mode; this is Ctrl-Z
        raise(SIGTSTP);
    } else if (c == 0x0C) {
        // Ctrl-L = refresh screen
        redraw(true, 0);
    } else if (c >= 0 && (c & 0x7F) == c) {
        typed_char = util_fromascii(c & 0x7F);
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

static bool if_tty_squawk(int level, bool cont, const char *fmt, va_list args)
{
    if (!msgwin) return false;
    vw_printw(msgwin, fmt, args);
    overlay_timer = overlay_wait;
    refresh_overlay = true;
    return true; // suppress normal (stderr) squawking
}

IfaceDesc ttyInterface = {
    .start = if_tty_start,
    .step  = if_tty_step,
    .poke  = if_tty_poke,
    .peek  = if_tty_peek,
    .frame = if_tty_frame,
    .unhook= if_tty_unhook,
    .squawk= if_tty_squawk,
    .display_touched = if_tty_display_touched,
};
