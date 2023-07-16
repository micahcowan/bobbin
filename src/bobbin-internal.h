#ifndef BOBBIN_INTERNAL_H
#define BOBBIN_INTERNAL_H

#define _XOPEN_SOURCE   700

#include <signal.h>
#include <stdint.h>
#include <stdio.h>

typedef uint16_t    word;
typedef uint8_t     byte;

#define LOC_NMI     0xFFFA
#define LOC_RESET   0xFFFC
#define LOC_IRQ     0xFFFE
#define LOC_BRK     LOC_IRQ

#define BYTE(b)         ((b) & 0xFF)
#define WORD(lo, hi)    (0xFFFF & ((BYTE(hi) << 8) | BYTE(lo)))
#define HI(w)           ((0xFF00 & (w)) >> 8)
#define LO(w)           (0x00FF & (w))

extern void bobbin_run(void);

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

extern void mem_init(void);
extern byte mem_get_byte(word loc);
extern byte mem_get_byte_nobus(word loc);
extern void mem_put_byte(word loc, byte val);
extern void mem_put_byte_nobus(word loc, byte val);

static inline byte stack_get(void)
{
    return mem_get_byte(STACK);
}

static inline void stack_put(byte val)
{
    mem_put_byte(STACK, val);
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
    byte op = mem_get_byte(PC);
    PC_ADV;
    return op;
}

/********** TRACE **********/

extern word current_instruction;

extern void trace_instr(void);
extern int  trace_mem_get_byte(word loc);

/********** DEBUG **********/

extern void debugger(void);

/********** UTIL **********/

extern void util_print_state(FILE *f);
extern int util_toascii(int c);
extern int util_fromascii(int c);
extern int util_isprint(int c);

/* TBD */
extern word print_disasm(FILE *f, word pos, Registers *regs);
static inline void cycle(void) {}
extern sig_atomic_t sigterm_received;

#endif /* BOBBIN_INTERNAL_H */
