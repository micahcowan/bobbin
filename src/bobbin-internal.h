#ifndef BOBBIN_INTERNAL_H
#define BOBBIN_INTERNAL_H

#include "ac-config.h"
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE   700

#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <strings.h>

#include "apple2.h"

typedef uint16_t    word;
typedef uint8_t     byte;

#define STREQ(a, b) (!strcmp(a, b))
#define STREQCASE(a, b) (!strcasecmp(a, b))

#define BYTE(b)         ((b) & 0xFF)
#define WORD(lo, hi)    (0xFFFF & ((BYTE(hi) << 8) | BYTE(lo)))
#define HI(w)           ((0xFF00 & (w)) >> 8)
#define LO(w)           (0x00FF & (w))

extern void bobbin_run(void);
extern word current_pc(void);

/********** LOGGING **********/

#define DIE_LEVEL      0
#define WARN_LEVEL     1
#define INFO_LEVEL     2
#define VERBOSE_LEVEL  3

#define DEFAULT_LEVEL   WARN_LEVEL

#define SQUAWK_OK(level)    ((level) <= cfg.squawk_level)
#define WARN_OK         SQUAWK_OK(WARN_LEVEL)
#define INFO_OK         SQUAWK_OK(INFO_LEVEL)
#define VERBOSE_OK      SQUAWK_OK(VERBOSE_LEVEL)

#define SQUAWK_CONT(level, ...) \
    do { \
        if (SQUAWK_OK(level)) { \
            squawk(level, 1, __VA_ARGS__); \
        } \
    } while (0)
#define SQUAWK(level, ...) \
    do { \
        if (SQUAWK_OK(level)) { \
            squawk(level, 0, __VA_ARGS__); \
        } \
    } while (0)

#define WARN(...)           SQUAWK(WARN_LEVEL, __VA_ARGS__)
#define WARN_CONT(...)      SQUAWK_CONT(WARN_LEVEL, __VA_ARGS__)
#define INFO(...)           SQUAWK(INFO_LEVEL, __VA_ARGS__)
#define INFO_CONT(...)      SQUAWK_CONT(INFO_LEVEL, __VA_ARGS__)
#define VERBOSE(...)        SQUAWK(VERBOSE_LEVEL, __VA_ARGS__)
#define VERBOSE_CONT(...)   SQUAWK_CONT(VERBOSE_LEVEL, __VA_ARGS__)

#define DIE_FINAL(st) do { \
        SQUAWK(DIE_LEVEL, "Exiting (%d).\n", (int)st); \
        exit(st); \
    } while (0)
#define DIE_CONT(st, ...) do { \
        SQUAWK_CONT(DIE_LEVEL, __VA_ARGS__); \
        if (st) DIE_FINAL(st); \
    } while(0)
#define DIE(st, ...) do { \
        SQUAWK(DIE_LEVEL, __VA_ARGS__); \
        if (st) DIE_FINAL(st); \
    } while(0)

/********** CONFIG **********/

extern const char *program_name; // main.c

typedef struct Config Config;
struct Config {
    int squawk_level;
    const char *    inputfile;
    const char *    outputfile;
    const char *    interface;

    // machine/emulation config optioons
    const char *    machine;
    const char *    disk;
    const char *    disk2;
    bool            machine_set;
    size_t          amt_ram;
    bool            load_rom;
    const char *    ram_load_file;
    unsigned long   ram_load_loc;
    const char *    rom_load_file;
    bool            basic_fixup;
    bool            turbo;
    bool            turbo_was_set;
    bool            lang_card;
    bool            lang_card_set;
    word            start_loc;
    bool            start_loc_set;
    word            delay_until;
    bool            delay_set;
    bool            bell;

    // "simple" interface config:
    bool            remain_after_pipe;
    bool            remain_tty;
    const char *    simple_input_mode;

    // trace stuff
    bool            die_on_brk;
    const char *    trace_file;
    uintmax_t       trace_start;
    uintmax_t       trace_end;
    bool            trap_success_on;
    word            trap_success;
    bool            trap_failure_on;
    word            trap_failure;
    bool            trap_print_on;
    word            trap_print;

    // special options
    bool            watch;
    bool            tokenize;
    bool            detokenize;
};
extern Config cfg;

/********** CPU **********/

typedef struct Registers Registers;
struct Registers {
    word    pc;
    byte    sp;
    byte    p; /* status flags */
    byte    a; /* accumulator */
    byte    x;
    byte    y;
};

typedef struct Cpu Cpu;
struct Cpu {
    Registers regs;
};

extern Cpu theCpu;

#define PC      (theCpu.regs.pc)
#define SP      (theCpu.regs.sp)
#define STACK   WORD(SP, 0x01)
#define PFLAGS  (theCpu.regs.p)
#define ACC     (theCpu.regs.a)
#define XREG    (theCpu.regs.x)
#define YREG    (theCpu.regs.y)

#define PC_ADV  ++PC

#define PCARRY  0
#define PZERO   1
#define PINT    2
#define PDEC    3
#define PBRK    4
#define PUNUSED 5
#define POVERFL 6
#define PNEG    7

#define PMASK(flag)     (1 << flag)
#define PGET(flag)      (!!(PFLAGS & PMASK(flag)))
#define PPUT(flag, val) ((void)(PFLAGS = ((PFLAGS & ~PMASK(flag)) \
                                   | (!!(val) << flag))))
#define PTEST(flag)     (!!(PFLAGS & PMASK(flag)))

#define RPGET(p, flag)      (!!((p) & PMASK(flag)))
#define RPPUT(p, flag, val) ((void)((p) = (((p) & ~PMASK(flag)) \
                                   | (!!(val) << flag))))
#define RPTEST(p, flag)     (!!((p) & PMASK(flag)))

extern void cpu_reset(void);
extern void cpu_step(void);

static inline void go_to(word w) {
    PC = w;
}

/********** MACHINE **********/

size_t expected_rom_size(void);
extern const char *default_romfname;
extern bool validate_rom(unsigned char *buf, size_t sz);
extern bool machine_is_iie(void);
extern bool machine_has_mousetext(void);

/********** MEMORY **********/

typedef struct RestartSw RestartSw;
struct RestartSw {
    bool ramrd : 1; // s/b auxrd
    bool ramwrt: 1; // s/b auxwrt
    bool intcxrom : 1;
    bool altzp : 1;
    bool intc8rom : 1;
    bool slotc3rom : 1;
    bool eightystore : 1;
    bool vertblank : 1;
    bool text : 1;
    bool mixed : 1;
    bool page2 : 1;
    bool hires : 1;
    bool altcharset : 1;
    bool eightycol : 1;
};
extern RestartSw rstsw;

extern void mem_init(void);
extern void mem_reset(void);
extern void mem_reboot(void);
extern const byte *getram(void);
extern byte peek(word loc);
extern void poke(word loc, byte val);
// These versions don't trigger debugger break-on-memory,
//  and don't affect or use floating bus values.
extern byte peek_sneaky(word loc);
extern void poke_sneaky(word loc, byte val);
extern bool mem_match(word loc, unsigned int nargs, ...);
extern byte *load_rom(const char *fname, size_t expected, bool exact);
extern void load_ram_finish(void);
extern size_t mem_transform_aux(word loc, bool wr);

static inline byte stack_get(void)
{
    return peek(STACK);
}

static inline byte stack_get_sneaky(void)
{
    return peek(STACK);
}

static inline void stack_put(byte val)
{
    poke(STACK, val);
}

static inline void stack_put_sneaky(byte val)
{
    poke_sneaky(STACK, val);
}

static inline byte stack_inc(void)
{
    return ++SP;
}

static inline byte stack_dec(void)
{
    return --SP;
}

static inline void stack_push(byte val)
{
    stack_put(val);
    (void) stack_dec();
}

static inline void stack_push_sneaky(byte val)
{
    stack_put_sneaky(val);
    (void) stack_dec();
}

static inline void stack_push_flags_or(byte val)
{
    stack_push(PFLAGS | (1 << PUNUSED) | val);
}

static inline byte stack_pop(void)
{
    (void) stack_inc();
    return stack_get();
}

static inline byte stack_pop_sneaky(void)
{
    (void) stack_inc();
    return stack_get_sneaky();
}

static inline byte pc_get_adv(void)
{
    byte op = peek(PC);
    PC_ADV;
    return op;
}

static inline word word_at(word loc)
{
    return WORD(peek_sneaky(loc), peek_sneaky(loc+1));
}

// s/b in CPU, but need to be decl'd after stack_pop etc
static inline void rts(void) {
    byte lo = stack_pop();
    byte hi = stack_pop();
    go_to(WORD(lo, hi)+1);
}

/********** EVENT **********/

enum EventType {
    EV_NONE,
    EV_INIT,
        /* Sent for options processing. Should do no "real" work,
           so that options are all verified for correctness before
           any are obeyed. */
    EV_START,
        /* Initialization that involves more work than option
           verification. Opening files, etc... */
    EV_REBOOT,
        /* Handler for emulation being restarted, e.g. via --watch. */
    EV_RESET,
        /* Handler for boot, and for any time the reset key is pressed. */
    EV_PRESTEP,
        /* Sent before an instruction will be executed, before we're
           even really certain that the PC is where it should be.
           This is specifically to provide opportunities to react
           to the current machine state and PC location, possibly by
           averting the PC location to somewhere else. If the PC is
           changed during PRESTEP, PRESTEP will be reissued
           for the new PC location (so other listeners can react
           to *that*. */
    EV_STEP,
        /* Sent when we're about to execute an instruction.
           Listeners are expected NOT to adjust the PC during this phase,
           as earlier handlers may have taken actions on the assumption
           that we're really here now, and a user using a debugger
           will have already seen this as the next instr. */
    EV_PEEK,
        /* Memory is being accessed via the "bus", in read-mode.
           Use peek_sneaky() to avoid triggering this. */
    EV_POKE,
        /* Memory is being written to, via the "bus in write-mode.
           Use poke_sneaky() to avoid triggering this. */
    EV_SWITCH,
        /* An MMU or display soft-switch was altered. */

    /* The following event types are ONLY sent if requested
       via the handler flags. */
    EV_CYCLE,
        /* Start of every cycle. */
    EV_FRAME,
        /* Called when ~ a 60th of a second (emulated time) has passed.
           Interfaces are automatically registered for this. */

    /* The following event types are ONLY sent to interfaces. */
    EV_UNHOOK,
        /* Disable the interface. Used e.g. to enter the debugger,
           or on DIE() to ensure the terminal is free for
           displaying exit messages. */
    EV_REHOOK,
        /* Re-enable the interface. Used e.g. when exiting the debugger. */
    EV_DISPLAY_TOUCH,
        /* Used to indicate that screen memory may have been changed,
           in a way that did not trigger POKE events. Interface
           should re-scan memory to be certain the screen's contents
           are correct. */
    EV_DISK_ACTIVE,
        /* A change in disk activity. Intended for interfaces
           to indicate via a "disk light" or some such. `val` holds
           the activity type: 0 for drives off, 1 for drive 1 motor
           running, and 2 for drive 2 motor running. */
};
typedef enum EventType EventType;

typedef struct Event Event;
struct Event {
    enum EventType type;
    bool suppress;
        /* Set to TRUE if a POKE to memory should be suppressed
           (because a handler already dealt with it in another way). */
    word loc;
        /* Location of a PEEK or POKE event; unused otherwise. */
    size_t aloc;
        /* In a PEEK or POKE event, will equal loc | 0x10000
           iff loc refers to a read or write at auxilliary memory. */
    int val;
        /* May be set by PEEK handlers to override what value will be
           read.
           POKE will set to the value that is planned to be written
           to LOC. */
};

typedef void (*event_handler)(Event *e);

extern void events_init(void);
extern void event_reghandler(event_handler h);
extern void event_unreghandler(event_handler h);
extern void event_fire_disk_active(int val);
extern int event_fire_peek(word loc);
extern bool event_fire_poke(word loc, byte val);
extern void event_fire(EventType type); // For all other events

// frame_timer: resets the timer if exists, creates if not
extern void frame_timer(unsigned int time, void (*fn)(void));
// frame_timer_reset: resets the timer if it exists, ignores if not
extern void frame_timer_reset(unsigned int time, void (*fn)(void));
extern void frame_timer_cancel(void (*fn)(void));

/********** INTERFACES **********/

typedef struct IfaceDesc IfaceDesc;
struct IfaceDesc {
    event_handler event;
    bool (*squawk)(int level, bool cont, const char *fmt, va_list args);
        // returns true to suppress default squawk handling
};

extern void interfaces_init(void);
extern void interfaces_start(void);
extern void iface_fire(Event *e); // For all other events
extern void squawk(int level, bool cont, const char *format, ...);

/********** PERIPHERALS **********/

// Doesn't nearly represent everything a card can see,
//  but enough to get started for now.

typedef byte (*periph_handler)(word loc, int val, int ploc, int psw);
// val: -1 if read, byte value if write.
// ploc: loc & 0x00FF if loc is in $CnXX (n == slot); -1 otherwise.
// psw:  loc & 0x000F if loc is in $C0nX (n == slot); -1 otherwise.

typedef struct PeriphDesc PeriphDesc;
struct PeriphDesc {
    void (*init)(void);
    periph_handler  handler;
};

extern void periph_init(void);
extern byte periph_sw_peek(word loc);
extern void periph_sw_poke(word loc, byte val);
extern byte periph_rom_peek(word loc);
extern void periph_rom_poke(word loc, byte val);

// Disk ][ controller
extern bool drive_spinning(void);

/********** FORMATS  **********/

#define NUM_TRACKS      35
#define SECTOR_SIZE     256

typedef struct DiskFormatDesc DiskFormatDesc;
struct DiskFormatDesc {
    void *privdat;
    bool writeprot;
    unsigned int halftrack;
    void (*spin)(DiskFormatDesc *, bool);
    byte (*read_byte)(DiskFormatDesc *);
    void (*write_byte)(DiskFormatDesc *, byte);
    void (*eject)(DiskFormatDesc *);
};

extern DiskFormatDesc disk_insert(const char *path);

/********** TRACE **********/

extern const char *trfile_name;
extern FILE *trfile;

extern void trace_reg(void);
extern void trace_step(Event *e);
extern void trace_on(char *format, ...);
extern void trace_off(void);
extern int  tracing(void);

/********** DEBUG **********/

typedef int (*printer)(const char * fmt, ...);
bool command_do(const char *line, printer pr);

/********** DEBUG **********/

extern void dbg_on(void);
extern void debugger(void);
extern bool debugging(void);
extern void breakpoint_set(word loc);

/********** UTIL **********/

extern void *xalloc(size_t sz);
extern int mmapfile(const char *fname, byte **buf, size_t *sz, int flags);
extern const char *get_file_ext(const char *path);
extern void util_print_state(FILE *f, word pc, Registers *reg);
extern bool util_isflashing(int c);
extern bool util_isreversed(int c, bool flash);
extern int util_todisplay(int c);
extern int util_toascii(int c);
extern int util_fromascii(int c);
extern int util_isprint(int c);
extern void util_reopen_stdin_tty(int flags);

/********** WATCH **********/

extern void setup_watches(void);
extern bool check_watches(void);

/* TBD */
extern word print_disasm(FILE *f, word pos, Registers *regs);

#define NS_PER_FRAME        16651559
#define CYCLES_PER_FRAME    17030

extern uintmax_t cycle_count;
extern uintmax_t instr_count;
extern uintmax_t frame_count;
extern bool text_flash;
static inline void cycle(void) { ++cycle_count; }
extern volatile sig_atomic_t sigint_received;
extern volatile sig_atomic_t sigwinch_received;
extern void unhandle_sigint(void);

#endif /* BOBBIN_INTERNAL_H */
