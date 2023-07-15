#ifndef BOBBIN_INTERNAL_H
#define BOBBIN_INTERNAL_H

#define _XOPEN_SOURCE   700

#include <stdint.h>

typedef uint16_t    word;
typedef uint8_t     byte;

#define LOC_NMI     0xFFFA
#define LOC_RESET   0xFFFC
#define LOC_IRQ     0xFFFE
#define LOC_BRK     LOC_IRQ

#define BYTE(b)         ((b) & 0xFF)
#define WORD(lo, hi)    (0xFFFF & ((BYTE(hi) << 8) | BYTE(lo)))

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

#define PC      theCpu.regs.pc
#define SP      theCpu.regs.sp
#define STACK   WORD(SP, 0x01)

extern void cpu_reset(void);
extern void cpu_step(void);

static inline void go_to(word w) {
    theCpu.regs.pc = w;
}

/********** MEMORY **********/

extern byte mem_get_byte(word loc);

static inline byte stack_get(void)
{
    return mem_get_byte(STACK);
}

static inline byte stack_dec(void)
{
    return --SP;
}


/* TBD */
static inline void cycle(void) {}

#endif /* BOBBIN_INTERNAL_H */
