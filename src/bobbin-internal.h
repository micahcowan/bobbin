#ifndef BOBBIN_INTERNAL_H
#define BOBBIN_INTERNAL_H

#define _XOPEN_SOURCE   700

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint16_t    word;
typedef uint8_t     byte;

#define STREQ(a, b) (!strcmp(a, b))

#define LOC_NMI     0xFFFA
#define LOC_RESET   0xFFFC
#define LOC_IRQ     0xFFFE
#define LOC_BRK     LOC_IRQ

#define BYTE(b)         ((b) & 0xFF)
#define WORD(lo, hi)    (0xFFFF & ((BYTE(hi) << 8) | BYTE(lo)))
#define HI(w)           ((0xFF00 & (w)) >> 8)
#define LO(w)           (0x00FF & (w))

extern void bobbin_run(void);

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
            fprintf(stderr, __VA_ARGS__); \
        } \
    } while (0)
#define SQUAWK(level, ...) \
    do { \
        if (SQUAWK_OK(level)) { \
            fprintf(stderr, "%s: ", program_name); \
            fprintf(stderr, __VA_ARGS__); \
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
    const char *    interface;
    const char *    machine;
    size_t          amt_ram;

    // "simple" interface config:
    bool            remain_after_pipe;
    const char *    simple_input_mode;
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

extern void cpu_reset(void);
extern void cpu_step(void);

static inline void go_to(word w) {
    theCpu.regs.pc = w;
}

/********** MEMORY **********/

extern const char *bobbin_test;

extern void mem_init(void);
extern byte peek(word loc);
extern void poke(word loc, byte val);
// These versions don't trigger debugger break-on-memory,
//  and don't affect or use floating bus values.
extern byte peek_sneaky(word loc);
extern void poke_sneaky(word loc, byte val);
extern bool mem_match(word loc, unsigned int nargs, ...);

static inline byte stack_get(void)
{
    return peek(STACK);
}

static inline void stack_put(byte val)
{
    poke(STACK, val);
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

static inline void stack_push_flags_or(byte val)
{
    stack_push(PFLAGS | (1 << PUNUSED) | val);
}

static inline byte stack_pop(void)
{
    (void) stack_inc();
    return stack_get();
}

static inline byte pc_get_adv(void)
{
    byte op = peek(PC);
    PC_ADV;
    return op;
}

// s/b in CPU, but need to be decl'd after stack_pop etc
static inline void rts(void) {
    byte lo = stack_pop();
    byte hi = stack_pop();
    go_to(WORD(lo, hi)+1);
}

/********** INTERFACES **********/

typedef struct IfaceDesc IfaceDesc;
struct IfaceDesc {
    void (*init)(void); // Process options
    void (*start)(void);// *Actually* start interface
    void (*step)(void);
    int  (*peek)(word loc);
    int  (*poke)(word loc, byte val);
};

extern void interfaces_init(void);
extern void interfaces_start(void);

extern void iface_step(void);
extern int  iface_poke(word loc, byte val);
                                  // if -1 is NOT returned, suppress
                                  // actual memory write (because iface
                                  // intercepted the write)

extern int  iface_peek(word loc); // returns -1 if no change over "real" mem

/********** TRACE **********/

extern const char *trfile_name;
extern FILE *trfile;

extern word current_instruction;

extern void trace_on(char *format, ...);
extern void trace_off(void);
extern int  tracing(void);

extern void trace_instr(void);
extern int  trace_peek_sneaky(word loc);
extern int  trace_peek(word loc);

/********** DEBUG **********/

extern void debugger(void);

/********** UTIL **********/

extern void util_print_state(FILE *f);
extern int util_toascii(int c);
extern int util_fromascii(int c);
extern int util_isprint(int c);

/* TBD */
extern word print_disasm(FILE *f, word pos, Registers *regs);
extern unsigned long long cycle_count;
static inline void cycle(void) { ++cycle_count; }
extern sig_atomic_t sigint_received;
extern const char *default_romfname;
extern bool validate_rom(unsigned char *buf, size_t sz);

#endif /* BOBBIN_INTERNAL_H */
