#include "bobbin-internal.h"

Cpu theCpu;

void cpu_reset(void)
{
    /* Cycle details taken from https://www.pagetable.com/?p=410 */
    /* Cycles here are counted from 0 to match the source (for
       comparison purposes), but elsewhere counted from 1. */
    (void) mem_get_byte(PC);
    cycle(); /* end of cycle 0 */
    (void) mem_get_byte(PC);
    cycle();
    (void) mem_get_byte(PC);
    cycle();

    (void) stack_get();
    (void) stack_dec();
    cycle(); /* end of cycle 3; fake push of PCH */
    (void) stack_get();
    (void) stack_dec();
    cycle(); /* end of cycle 4; fake push of PCL */
    (void) stack_get();
    (void) stack_dec();
    cycle(); /* end of cycle 5; fake push of status */

    byte pcL = mem_get_byte(LOC_RESET);
    cycle(); /* end of cycle 6; read vector low byte */
    byte pcH = mem_get_byte(LOC_RESET+1);
    go_to(WORD(pcL, pcH));
    cycle(); /* end of cycle 7 (8th); read vector high byte */
}

#define OP_READ_INDX(exec) \
    do { \
        PC_ADV; \
        cycle(); \
        (void) mem_get_byte(XREG); \
        cycle(); \
        byte lo = mem_get_byte(LO(immed + XREG)); \
        cycle(); \
        byte hi = mem_get_byte(LO(immed + XREG + 1)); \
        cycle(); \
        byte val = mem_get_byte(WORD(lo, hi)); \
        exec; \
        cycle(); \
    } while (0)

#define OP_READ_ZP(exec) \
    do { \
        PC_ADV; \
        cycle(); \
        byte val = mem_get_byte(immed); \
        exec; \
        cycle(); \
    } while (0)

#define OP_RMW_ZP(exec) \
    do { \
        PC_ADV; \
        cycle(); \
        byte val = mem_get_byte(immed); \
        cycle(); \
        mem_put_byte(LO(immed), val); \
        cycle(); \
        exec; \
        mem_put_byte(LO(immed), val); \
        cycle(); \
    } while (0)

#define OP_READ_IMM(exec) \
    do { \
        byte val = immed; /* just in case */ \
        PC_ADV; \
        exec; \
        cycle(); \
    } while (0)

#define OP_RMW_IMPL(exec) \
    do { \
        exec; \
        cycle(); \
    } while (0)

#define OP_READ_ABS(exec) \
    do { \
        byte lo = immed; \
        PC_ADV; \
        cycle(); \
        byte hi = pc_get_adv(); \
        cycle(); \
        byte val = mem_get_byte(WORD(lo, hi)); \
        exec; \
        cycle(); \
    } while (0)

#define OP_RMW_ABS(exec) \
    do { \
        byte lo = immed; \
        PC_ADV; \
        cycle(); \
        byte hi = pc_get_adv(); \
        cycle(); \
        word addr = WORD(lo, hi); \
        byte val = mem_get_byte(addr); \
        cycle(); \
        mem_put_byte(addr, val); \
        cycle(); \
        exec; \
        mem_put_byte(addr,val); \
        cycle(); \
    } while (0)

#define OP_BRANCH(test) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        (void) mem_get_byte(PC); \
        if (test) { \
            word addr = PC + immed; \
            PC = WORD(HI(PC), LO(addr)); \
            cycle(); /* 3 */\
            (void) mem_get_byte(PC); \
            if (PC != addr) { \
                cycle(); /* 4 */ \
                PC = addr; \
                (void) mem_get_byte(PC); \
            } \
            cycle(); /* 4 or 5 */ \
        } else { \
            PC_ADV; \
            cycle(); /* 3 */ \
        } \
    } while (0)

static inline void carry_from_msb(byte val)
{
    PPUT(PCARRY, val & 0x80);
}

static inline void carry_from_lsb(byte val)
{
    PPUT(PCARRY, val & 0x01);
}

static inline byte do_asl(byte val)
{
    carry_from_msb(val);
    return val << 1;
}

void cpu_step(void)
{
    /* Cycle references taken from https://www.nesdev.org/6502_cpu.txt. */
    byte op = pc_get_adv();
    cycle(); // end 1
    byte immed = mem_get_byte(PC);

    switch (op) {
        case 0x01: // ORA, (MEM,x).
            OP_READ_INDX(ACC |= val);
            break;
        case 0x05: // ORA, ZP
            OP_READ_ZP(ACC |= val);
            break;
        case 0x06: // ASL, ZP
            OP_RMW_ZP(val = do_asl(val));
            break;
        case 0x08: // PHP (impl.)
            cycle();
            stack_push_flags_or(1 << PCARRY);
            cycle();
            break;
        case 0x09: // ORA, immed.
            OP_READ_IMM(ACC |= immed);
            break;
        case 0x0A: // ASL, impl.
            OP_RMW_IMPL(ACC = do_asl(ACC));
            break;
        case 0x0D: // ORA, abs
            OP_READ_ABS(ACC |= val);
            break;
        case 0x0E: // ASL, abs
            OP_RMW_ABS(val = do_asl(val));
            break;
        case 0x10: // BPL
            OP_BRANCH(!PTEST(PNEG));
            break;
        default: // BRK
            // XXX cycles and behavior not realistic
            //  for non-break unsupported op-codes
            PC_ADV;
            cycle(); // end 2

            stack_push(HI(PC));
            cycle(); // 3
            stack_push(LO(PC));
            cycle(); // 4
            stack_push_flags_or(0);
            cycle(); // 5

            byte pcL = mem_get_byte(LOC_BRK);
            cycle(); // 6
            byte pcH = mem_get_byte(LOC_BRK + 1);
            go_to(WORD(pcL, pcH));
            cycle(); // 7
            break;
    }
}
