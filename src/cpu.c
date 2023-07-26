#include "bobbin-internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

Cpu theCpu;

uintmax_t instr_count = 0;

void cpu_reset(void)
{
    PFLAGS |= PMASK(PUNUSED);
    /* Cycle details taken from https://www.pagetable.com/?p=410 */
    /* Cycles here are counted from 0 to match the source (for
       comparison purposes), but elsewhere counted from 1. */
    (void) peek(PC);
    cycle(); /* end of cycle 0 */
    (void) peek(PC);
    cycle();
    (void) peek(PC);
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

    byte pcL = peek(LOC_RESET);
    cycle(); /* end of cycle 6; read vector low byte */
    byte pcH = peek(LOC_RESET+1);
    go_to(WORD(pcL, pcH));
    cycle(); /* end of cycle 7 (8th); read vector high byte */
}

// Sign extend
#define SE(v)  (((v) & 0x80)? ((v) | 0xFF00) : (v))
// "Negation"
#define NEG(v) (~(((v) - 1) & 0xFFFF))

#define OP_READ_INDX(exec) \
    do { \
        PC_ADV; \
        cycle(); \
        (void) peek(XREG); \
        cycle(); \
        byte lo = peek(LO(immed + XREG)); \
        cycle(); \
        byte hi = peek(LO(immed + XREG + 1)); \
        cycle(); \
        byte val = peek(WORD(lo, hi)); \
        exec; \
        cycle(); \
    } while (0)

#define OP_READ_ZP(exec) \
    do { \
        PC_ADV; \
        cycle(); \
        byte val = peek(immed); \
        exec; \
        cycle(); \
    } while (0)

#define OP_RMW_ZP(exec) \
    do { \
        PC_ADV; \
        cycle(); \
        byte val = peek(immed); \
        cycle(); \
        poke(LO(immed), val); \
        cycle(); \
        exec; \
        poke(LO(immed), val); \
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
        byte val = peek(WORD(lo, hi)); \
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
        byte val = peek(addr); \
        cycle(); \
        poke(addr, val); \
        cycle(); \
        exec; \
        poke(addr,val); \
        cycle(); \
    } while (0)

#define OP_BRANCH(test) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        word orig = PC; \
        (void) peek(PC); \
        if (test) { \
            word offset = SE(immed); \
            word addr = PC + offset; \
            go_to(WORD(LO(addr), HI(PC))); \
            cycle(); /* 3 */\
            (void) peek(PC); \
            if (PC != addr) { \
                cycle(); /* 4 */ \
                go_to(addr); \
                (void) peek(PC); \
            } \
            cycle(); /* 4 or 5 */ \
        } else { \
            cycle(); /* 3 */ \
        } \
    } while (0)

#define OP_READ_INDY(exec) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        byte lo = peek(immed); \
        cycle(); /* 3 */ \
        byte hi = peek(immed + 1); \
        word addr = WORD(lo, hi) + YREG; \
        word wrAddr = WORD(LO(lo + YREG), hi); \
        cycle(); /* 4 */ \
        byte val = peek(wrAddr); \
        if (addr == wrAddr) { \
            exec; \
        } else { \
            cycle(); /* 5 */ \
            val = peek(addr); \
            exec; \
        } \
        cycle(); /* 5 or 6 */ \
    } while (0)

#define OP_READ_ZP_IDX(reg, exec) \
    do { \
        PC_ADV; \
        cycle(); \
        (void) peek(immed); \
        cycle(); \
        byte val = peek(LO(immed + reg)); \
        exec; \
    } while (0)

#define OP_RMW_ZP_IDX(reg, exec) \
    do { \
        PC_ADV; \
        cycle(); \
        (void) peek(immed); \
        cycle(); \
        byte addr = LO(immed + reg); \
        byte val = peek(addr); \
        cycle(); \
        poke(addr, val); \
        cycle(); \
        exec; \
        poke(addr, val); \
        cycle(); /* 6 */ \
    } while (0)

#define OP_READ_ABS_IDX(reg, exec) \
    do { \
        PC_ADV; \
        cycle(); \
        byte lo = immed; \
        byte hi = pc_get_adv(); \
        word addr = WORD(lo, hi) + reg; \
        word wrAddr = WORD(LO(lo + reg), hi); \
        cycle(); /* 3 */ \
        byte val = peek(wrAddr); \
        if (addr == wrAddr) { \
            exec; \
        } else { \
            cycle(); \
            val = peek(addr); \
            exec; \
        } \
    } while (0)

#define OP_RMW_ABS_IDX(reg, exec) \
    do { \
        PC_ADV; \
        cycle(); \
        byte lo = immed; \
        byte hi = pc_get_adv(); \
        word addr = WORD(lo, hi) + reg; \
        word wrAddr = WORD(LO(lo + reg), hi); \
        cycle(); /* 3 */ \
        byte val = peek(wrAddr); \
        cycle(); /* 4 */ \
        val = peek(addr); \
        cycle(); /* 5 */ \
        poke(addr, val); \
        cycle(); /* 6 */ \
        exec; \
        poke(addr, val); \
        cycle(); /* 7 */ \
    } while (0)

#define OP_WRITE_INDX(valReg) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        (void) peek(immed); \
        cycle(); /* 3 */ \
        immed += XREG; \
        byte lo = peek(immed); \
        cycle(); /* 4 */ \
        immed += 1; \
        byte hi = peek(immed); \
        cycle(); /* 5 */ \
        poke(WORD(lo, hi), valReg); \
        cycle(); /* 6 */ \
    } while (0)

#define OP_WRITE_ZP(reg) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        poke(immed, reg); \
        cycle(); /* 3 */ \
    } while (0)

#define OP_WRITE_ZP_IDX(idxReg, valReg) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        (void) peek(immed); \
        cycle(); /* 3 */ \
        poke(LO(immed + idxReg), valReg); \
        cycle();  /* 4 */ \
    } while (0)

#define OP_WRITE_ABS_IDX(idxReg, valReg) \
    do { \
        byte lo = immed; \
        PC_ADV; \
        cycle(); /* 2 */ \
        byte hi = pc_get_adv(); \
        cycle(); /* 3 */ \
        (void) peek(WORD(LO(lo + idxReg), hi)); \
        cycle(); /* 4 */ \
        poke(WORD(lo, hi) + idxReg, valReg); \
        cycle(); /* 5 */ \
    } while (0)

#define OP_WRITE_INDY(valReg) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        byte lo = peek(immed); \
        cycle(); /* 3 */ \
        byte hi = peek(LO(immed + 1)); \
        cycle(); /* 4 */ \
        (void) peek(WORD(LO(lo+YREG), hi)); \
        cycle(); /* 5 */ \
        poke(WORD(lo, hi)+YREG, valReg); \
        cycle(); /* 6 */ \
    } while (0)

#define OP_WRITE_ABS(valReg) \
    do { \
        byte lo = immed; \
        PC_ADV; \
        cycle(); /* 2 */ \
        byte hi = pc_get_adv(); \
        cycle(); /* 3 */ \
        poke(WORD(lo, hi), valReg); \
        cycle(); /* 4 */ \
    } while (0)

// Fix up flags
static inline byte ff(byte val)
{
    PPUT(PZERO, val == 0);
    PPUT(PNEG, val & 0x80);
    return val;
}

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
    return ff(LO(val << 1));
}

static inline byte do_rol(byte val)
{
    byte c = PTEST(PCARRY);
    carry_from_msb(val);
    return ff(LO(val << 1 | c));
}

static inline byte do_lsr(byte val)
{
    carry_from_lsb(val);
    return ff(LO(val >> 1));
}

static inline byte do_ror(byte val)
{
    byte c = PTEST(PCARRY);
    carry_from_lsb(val);
    return ff(LO(val >> 1 | (c << 7)));
}

static inline void do_bit(byte val)
{
    byte band = ACC & val;
    PPUT(PNEG, val & 0x80);
    PPUT(POVERFL, val & 0x40);
    PPUT(PZERO, band == 0);
}

static inline void do_adc(byte val)
{
    if (PTEST(PDEC)) {
        byte sumL = (ACC & 0xF) + (val & 0xF) + PGET(PCARRY);
        byte sumH = (ACC >> 4) + (val >> 4) + (sumL > 9);
        if (sumL > 9) sumL += 6;
        PPUT(PZERO, ((ACC + val + PGET(PCARRY)) & 0xFF) == 0);
        PPUT(PNEG, (sumH & 0x8) != 0);
        PPUT(POVERFL, (((sumH << 4) ^ ACC) & 0x80)
                        && !((ACC ^ val) & 0x80));
        if (sumH > 9) sumH += 6;
        PPUT(PCARRY, sumH > 9);
        ACC = LO(((sumH << 4) | (sumL & 0xF)));
    } else {
        word sum = ACC + val + PGET(PCARRY);
        PPUT(PNEG, sum & 0x80);
        int v = ((0x80 & ACC) == (0x80 & val)) && ((0x80 & ACC) != (0x80 & sum));
        PPUT(POVERFL, v);
        PPUT(PZERO, LO(sum) == 0);
        PPUT(PCARRY, sum & 0x100);
        ACC = LO(sum);
    }
}

static void do_sbc_seq3(byte val, Registers *reg)
{
    byte diffL = (reg->a & 0xF) - (val & 0xF) - !RPGET(reg->p, PCARRY);
    if (diffL & 0x10) diffL -= 6;
    byte diffH = (reg->a >> 4) - (val >> 4) - ((diffL & 0x80) != 0);
    if (diffH & 0x10) diffH -= 6;

    reg->a = LO(((diffH << 4) | (diffL & 0xF)));
}

static void do_sbc_bin(byte val, Registers *reg)
{
    word diff = reg->a - val - !PGET(PCARRY);
    RPPUT(reg->p, PNEG, diff & 0x80);
    int v = ((0x80 & reg->a) != (0x80 & val)) && ((0x80 & reg->a) != (0x80 & diff));
    RPPUT(reg->p, POVERFL, v);
    RPPUT(reg->p, PZERO, LO(diff) == 0);
    int borrow = (reg->a < val) || (reg->a == val && !RPGET(reg->p, PCARRY));
    RPPUT(reg->p, PCARRY, !borrow);
    reg->a = LO(diff);
}

static void do_sbc(byte val)
{
    if (PTEST(PDEC)) {
        // 6502 BCD SBC implementation based on
        //  http://www.6502.org/tutorials/decimal_mode.html
        //
        Registers decReg = theCpu.regs;
        do_sbc_seq3(val, &decReg);

        Registers binReg = theCpu.regs;
        do_sbc_bin(val, &binReg);

        theCpu.regs.a = decReg.a;
        theCpu.regs.p = binReg.p;
    } else {
        do_sbc_bin(val, &theCpu.regs);
    }
}

static inline void do_cmp(byte a, byte b)
{
    byte diff = a - b;
    PPUT(PNEG, diff & 0x80);
    PPUT(PZERO, diff == 0);
    PPUT(PCARRY, a >= b);
}

void cpu_step(void)
{
    /* Cycle references taken from https://www.nesdev.org/6502_cpu.txt. */
    byte op = pc_get_adv();
    cycle(); // end 1

    byte immed = peek(PC);

    switch (op) {
        case 0x01: // ORA, (MEM,x).
            OP_READ_INDX(ff(ACC |= val));
            break;
        case 0x05: // ORA, ZP
            OP_READ_ZP(ff(ACC |= val));
            break;
        case 0x06: // ASL, ZP
            OP_RMW_ZP(val = do_asl(val));
            break;
        case 0x08: // PHP (impl.)
            cycle();
            stack_push_flags_or(1 << PBRK);
            cycle();
            break;
        case 0x09: // ORA, immed.
            OP_READ_IMM(ff(ACC |= immed));
            break;
        case 0x0A: // ASL, impl.
            OP_RMW_IMPL(ACC = do_asl(ACC));
            break;
        case 0x0D: // ORA, abs
            OP_READ_ABS(ff(ACC |= val));
            break;
        case 0x0E: // ASL, abs
            OP_RMW_ABS(val = do_asl(val));
            break;
        case 0x10: // BPL
            OP_BRANCH(!PTEST(PNEG));
            break;
        case 0x11: // ORA, (MEM),y
            OP_READ_INDY(ff(ACC |= val));
            break;
        case 0x15: // ORA, ZP,x
            OP_READ_ZP_IDX(XREG, ff(ACC |= val));
            break;
        case 0x16: // ASL, ZP,x
            OP_RMW_ZP_IDX(XREG, val = do_asl(val));
            break;
        case 0x18: // CLC (impl.)
            OP_RMW_IMPL(PPUT(PCARRY, 0));
            break;
        case 0x19: // ORA, MEM,y
            OP_READ_ABS_IDX(YREG, ff(ACC |= val));
            break;
        case 0x1D: // ORA, MEM,x
            OP_READ_ABS_IDX(XREG, ff(ACC |= val));
            break;
        case 0x1E: // ASL, MEM,x
            OP_RMW_ABS_IDX(XREG, val = do_asl(val));
            break;


        case 0x20: // JSR
            {
                byte lo = immed;
                PC_ADV;
                cycle();
                (void) stack_get();
                cycle();
                stack_push(HI(PC));
                cycle();
                stack_push(LO(PC));
                cycle();
                word dest = WORD(lo, peek(PC));
                go_to(dest);
                cycle();
            }
            break;
        case 0x21: // AND, (MEM,x)
            OP_READ_INDX(ff(ACC &= val));
            break;
        case 0x24: // BIT, ZP
            OP_READ_ZP(do_bit(val));
            break;
        case 0x25: // AND, ZP
            OP_READ_ZP(ff(ACC &= val));
            break;
        case 0x26: // ROL, ZP
            OP_RMW_ZP(val = do_rol(val));
            break;
        case 0x28: // PLP (impl.)
            {
                cycle();
                stack_inc();
                cycle();
                byte p = peek(STACK);
                // Leave BRK flag alone; always set UNUSED
                PFLAGS = (p & 0xCF) | PMASK(PUNUSED);
                cycle();
            }
            break;
        case 0x29: // AND, imm
            OP_READ_IMM(ff(ACC &= val));
            break;
        case 0x2A: // ROL, impl.
            OP_RMW_IMPL(ACC = do_rol(ACC));
            break;
        case 0x2C: // BIT, abs
            OP_READ_ABS(do_bit(val));
            break;
        case 0x2D: // AND, abs
            OP_READ_ABS(ff(ACC &= val));
            break;
        case 0x2E: // ROL, abs
            OP_RMW_ABS(val = do_rol(val));
            break;
        case 0x30: // BMI
            OP_BRANCH(PTEST(PNEG));
            break;
        case 0x31: // AND, (MEM),y
            OP_READ_INDY(ff(ACC &= val));
            break;
        case 0x35: // AND, ZP,x
            OP_READ_ZP_IDX(XREG, ff(ACC &= val));
            break;
        case 0x36: // ROL, ZP,x
            OP_RMW_ZP_IDX(XREG, val = do_rol(val));
            break;
        case 0x38: // SEC (impl.)
            OP_RMW_IMPL(PPUT(PCARRY, 1));
            break;
        case 0x39: // AND, MEM,y
            OP_READ_ABS_IDX(YREG, ff(ACC &= val));
            break;
        case 0x3D: // AND, MEM,x
            OP_READ_ABS_IDX(XREG, ff(ACC &= val));
            break;
        case 0x3E: // ROL, MEM,x
            OP_RMW_ABS_IDX(XREG, val = do_rol(val));
            break;


        case 0x40: // RTI
            {
                cycle(); // end 2
                byte p = stack_pop();
                cycle(); // 3
                PFLAGS = (p & 0xCF) | PMASK(PUNUSED);
                byte lo = stack_pop();
                cycle(); // 4
                go_to(WORD(lo, HI(PC)));
                byte hi = stack_pop();
                cycle(); // 5
                go_to(WORD(lo, hi));
                (void) peek(STACK);
                cycle(); // 6
            }
            break;
        case 0x41: // EOR, (MEM,x)
            OP_READ_INDX(ff(ACC ^= val));
            break;
        case 0x45: // EOR, ZP
            OP_READ_ZP(ff(ACC ^= val));
            break;
        case 0x46: // LSR, ZP
            OP_RMW_ZP(val = do_lsr(val));
            break;
        case 0x48: // PHA
            cycle();
            stack_push(ACC);
            cycle();
            break;
        case 0x49: // EOR, imm
            OP_READ_IMM(ff(ACC ^= val));
            break;
        case 0x4A: // LSR, impl.
            OP_RMW_IMPL(ACC = do_lsr(ACC));
            break;
        case 0x4C: // JMP
            {
                byte lo = immed;
                PC_ADV;
                cycle();
                byte hi = pc_get_adv();
                word dest = WORD(lo, hi);
                go_to(dest);
                cycle();
            }
            break;
        case 0x4D: // EOR, abs
            OP_READ_ABS(ff(ACC ^= val));
            break;
        case 0x4E: // LSR, abs
            OP_RMW_ABS(val = do_lsr(val));
            break;
        case 0x50: // BVC
            OP_BRANCH(!PTEST(POVERFL));
            break;
        case 0x51: // EOR, (MEM),y
            OP_READ_INDY(ff(ACC ^= val));
            break;
        case 0x55: // EOR, ZP,x
            OP_READ_ZP_IDX(XREG, ff(ACC ^= val));
            break;
        case 0x56: // LSR, ZP,x
            OP_RMW_ZP_IDX(XREG, val = do_lsr(val));
            break;
        case 0x58: // CLI
            OP_RMW_IMPL(PPUT(PINT, 0));
            break;
        case 0x59: // EOR, MEM,y
            OP_READ_ABS_IDX(YREG, ff(ACC ^= val));
            break;
        case 0x5D: // EOR, MEM,x
            OP_READ_ABS_IDX(XREG, ff(ACC ^= val));
            break;
        case 0x5E: // LSR, MEM,x
            OP_RMW_ABS_IDX(XREG, val = do_lsr(val));
            break;


        case 0x60: // RTS
            {
                word orig = PC;
                cycle(); // end 2
                byte lo = stack_pop();
                cycle(); // 3
                go_to(WORD(lo, HI(PC)));
                (void) stack_pop();
                cycle(); // 4
                byte hi = peek(STACK);
                word dest = WORD(lo, hi);
                go_to(dest);
                cycle(); // 5
                PC_ADV;
                cycle(); // 6
            }
            break;
        case 0x61: // ADC, (MEM,x)
            OP_READ_INDX(do_adc(val));
            break;
        case 0x65: // ADC, ZP
            OP_READ_ZP(do_adc(val));
            break;
        case 0x66: // ROR, ZP
            OP_RMW_ZP(val = do_ror(val));
            break;
        case 0x68: // PLA
            cycle();
            (void) stack_pop();
            cycle();
            ff(ACC = peek(STACK));
            cycle();
            break;
        case 0x69: // ADC, imm
            OP_READ_IMM(do_adc(val));
            break;
        case 0x6A: // ROR, impl.
            OP_RMW_IMPL(ACC = do_ror(ACC));
            break;
        case 0x6C: // JMP ()
            {
                byte lo = immed;
                PC_ADV;
                cycle(); // 2
                byte hi = pc_get_adv();
                word addr = WORD(lo,hi);
                cycle(); // 3
                lo = peek(addr);
                cycle(); // 4
                hi = peek(WORD(LO(addr+1),HI(addr)));
                word dest = WORD(lo, hi);
                go_to(dest);
                cycle(); // 5
            }
            break;
        case 0x6D: // ADC, abs
            OP_READ_ABS(do_adc(val));
            break;
        case 0x6E: // ROR, abs
            OP_RMW_ABS(val = do_ror(val));
            break;
        case 0x70: // BVS
            OP_BRANCH(PTEST(POVERFL));
            break;
        case 0x71: // ADC, (MEM),y
            OP_READ_INDY(do_adc(val));
            break;
        case 0x75: // ADC, ZP,x
            OP_READ_ZP_IDX(XREG, do_adc(val));
            break;
        case 0x76: // ROR, ZP,x
            OP_RMW_ZP_IDX(XREG, val = do_ror(val));
            break;
        case 0x78: // SEI
            OP_RMW_IMPL(PPUT(PINT, 1));
            break;
        case 0x79: // ADC MEM,y
            OP_READ_ABS_IDX(YREG, do_adc(val));
            break;
        case 0x7D: // ADC, MEM,x
            OP_READ_ABS_IDX(XREG, do_adc(val));
            break;
        case 0x7E: // ROR, MEM,x
            OP_RMW_ABS_IDX(XREG, val = do_ror(val));
            break;


        case 0x81: // STA, (MEM,x)
            OP_WRITE_INDX(ACC);
            break;
        case 0x84: // STY, ZP
            OP_WRITE_ZP(YREG);
            break;
        case 0x85: // STA, ZP
            OP_WRITE_ZP(ACC);
            break;
        case 0x86: // STX, ZP
            OP_WRITE_ZP(XREG);
            break;
        case 0x88: // DEY
            OP_RMW_IMPL(ff(--YREG));
            break;
        case 0x8A: // TXA
            OP_RMW_IMPL(ff(ACC = XREG));
            break;
        case 0x8C: // STY, abs
            OP_WRITE_ABS(YREG);
            break;
        case 0x8D: // STA, abs
            OP_WRITE_ABS(ACC);
            break;
        case 0x8E: // STX, abs
            OP_WRITE_ABS(XREG);
            break;
        case 0x90: // BCC
            OP_BRANCH(!PTEST(PCARRY));
            break;
        case 0x91: // STA, (MEM),y
            OP_WRITE_INDY(ACC);
            break;
        case 0x94: // STY, ZP,x
            OP_WRITE_ZP_IDX(XREG, YREG);
            break;
        case 0x95: // STA, ZP,x
            OP_WRITE_ZP_IDX(XREG, ACC);
            break;
        case 0x96: // STX, ZP,y
            OP_WRITE_ZP_IDX(YREG, XREG);
            break;
        case 0x98: // TYA
            OP_RMW_IMPL(ff(ACC = YREG));
            break;
        case 0x99: // STA, MEM,y
            OP_WRITE_ABS_IDX(YREG, ACC);
            break;
        case 0x9A: // TXS
            OP_RMW_IMPL(SP = XREG); // No flag changes!
            break;
        case 0x9D: // STA, MEM,x
            OP_WRITE_ABS_IDX(XREG, ACC);
            break;


        case 0xA0: // LDY, immed.
            OP_READ_IMM(ff(YREG = val));
            break;
        case 0xA1: // LDA, (MEM,x)
            OP_READ_INDX(ff(ACC = val));
            break;
        case 0xA2: // LDX, immed.
            OP_READ_IMM(ff(XREG = val));
            break;
        case 0xA4: // LDY, ZP
            OP_READ_ZP(ff(YREG = val));
            break;
        case 0xA5: // LDA, ZP
            OP_READ_ZP(ff(ACC = val));
            break;
        case 0xA6: // LDX, ZP
            OP_READ_ZP(ff(XREG = val));
            break;
        case 0xA8: // TAY
            OP_RMW_IMPL(ff(YREG = ACC));
            break;
        case 0xA9: // LDA, immed.
            OP_READ_IMM(ff(ACC = val));
            break;
        case 0xAA: // TAX
            OP_RMW_IMPL(ff(XREG = ACC));
            break;
        case 0xAC: // LDY, abs
            OP_READ_ABS(ff(YREG = val));
            break;
        case 0xAD: // LDA, abs
            OP_READ_ABS(ff(ACC = val));
            break;
        case 0xAE: // LDX, abs
            OP_READ_ABS(ff(XREG = val));
            break;
        case 0xB0: // BCS
            OP_BRANCH(PTEST(PCARRY));
            break;
        case 0xB1: // LDA, (MEM),y
            OP_READ_INDY(ff(ACC = val));
            break;
        case 0xB4: // LDY, ZP,x
            OP_READ_ZP_IDX(XREG, ff(YREG = val));
            break;
        case 0xB5: // LDA, ZP,x
            OP_READ_ZP_IDX(XREG, ff(ACC = val));
            break;
        case 0xB6: // LDX, ZP,y
            OP_READ_ZP_IDX(YREG, ff(XREG = val));
            break;
        case 0xB8: // CLV
            OP_RMW_IMPL(PPUT(POVERFL, 0));
            break;
        case 0xB9: // LDA, MEM,y
            OP_READ_ABS_IDX(YREG, ff(ACC = val));
            break;
        case 0xBA: // TSX
            OP_RMW_IMPL(ff(XREG = SP));
            break;
        case 0xBC: // LDY MEM,x
            OP_READ_ABS_IDX(XREG, ff(YREG = val));
            break;
        case 0xBD: // LDA MEM,x
            OP_READ_ABS_IDX(XREG, ff(ACC = val));
            break;
        case 0xBE: // LDX MEM,y
            OP_READ_ABS_IDX(YREG, ff(XREG = val));
            break;


        case 0xC0: // CPY, immed.
            OP_READ_IMM(do_cmp(YREG, val));
            break;
        case 0xC1: // CMP, (MEM,x)
            OP_READ_INDX(do_cmp(ACC, val));
            break;
        case 0xC4: // CPY, ZP
            OP_READ_ZP(do_cmp(YREG, val));
            break;
        case 0xC5: // CMP, ZP
            OP_READ_ZP(do_cmp(ACC, val));
            break;
        case 0xC6: // DEC, ZP
            OP_RMW_ZP(ff(--val));
            break;
        case 0xC8: // INY, impl.
            OP_RMW_IMPL(ff(++YREG));
            break;
        case 0xC9: // CMP, immed.
            OP_READ_IMM(do_cmp(ACC, val));
            break;
        case 0xCA: // DEX, immed.
            OP_RMW_IMPL(ff(--XREG));
            break;
        case 0xCC: // CPY, abs.
            OP_READ_ABS(do_cmp(YREG, val));
            break;
        case 0xCD: // CMP, abs.
            OP_READ_ABS(do_cmp(ACC, val));
            break;
        case 0xCE: // DEC, abs.
            OP_RMW_ABS(ff(--val));
            break;
        case 0xD0: // BNE
            OP_BRANCH(!PTEST(PZERO));
            break;
        case 0xD1: // CMP, (MEM),y
            OP_READ_INDY(do_cmp(ACC, val));
            break;
        case 0xD5: // CMP, ZP,x
            OP_READ_ZP_IDX(XREG, do_cmp(ACC, val));
            break;
        case 0xD6: // DEC, ZP,x
            OP_RMW_ZP_IDX(XREG, ff(--val));
            break;
        case 0xD8: // CLD
            OP_RMW_IMPL(PPUT(PDEC, 0));
            break;
        case 0xD9: // CMP, MEM,y
            OP_READ_ABS_IDX(YREG, do_cmp(ACC, val));
            break;
        case 0xDD: // CMP, MEM,x
            OP_READ_ABS_IDX(XREG, do_cmp(ACC, val));
            break;
        case 0xDE: // DEC, MEM,x
            OP_RMW_ABS_IDX(XREG, ff(--val));
            break;


        case 0xE0: // CPX, immed.
            OP_READ_IMM(do_cmp(XREG, val));
            break;
        case 0xE1: // SBC, (MEM,x)
            OP_READ_INDX(do_sbc(val));
            break;
        case 0xE4: // CPX, ZP
            OP_READ_ZP(do_cmp(XREG, val));
            break;
        case 0xE5: // SBC, ZP
            OP_READ_ZP(do_sbc(val));
            break;
        case 0xE6: // INC, ZP
            OP_RMW_ZP(ff(++val));
            break;
        case 0xE8: // INX (impl.)
            OP_RMW_IMPL(ff(++XREG));
            break;
        case 0xE9: // SBC, immed.
            OP_READ_IMM(do_sbc(val));
            break;
        case 0xEA: // NOP
            OP_RMW_IMPL(); // empty statement
            break;
        case 0xEC: // CPX, abs.
            OP_READ_ABS(do_cmp(XREG, val));
            break;
        case 0xED: // SBC, abs.
            OP_READ_ABS(do_sbc(val));
            break;
        case 0xEE: // INC, abs.
            OP_RMW_ABS(ff(++val));
            break;
        case 0xF0: // BEQ
            OP_BRANCH(PTEST(PZERO));
            break;
        case 0xF1: // SBC, (MEM),y
            OP_READ_INDY(do_sbc(val));
            break;
        case 0xF5: // SBC, ZP,x
            OP_READ_ZP_IDX(XREG, do_sbc(val));
            break;
        case 0xF6: // INC, ZP,x
            OP_RMW_ZP_IDX(XREG, ff(++val));
            break;
        case 0xF8: // SED
            OP_RMW_IMPL(PPUT(PDEC, 1));
            break;
        case 0xF9: // SBC, MEM,y
            OP_READ_ABS_IDX(YREG, do_sbc(val));
            break;
        case 0xFD: // SBC, MEM,x
            OP_READ_ABS_IDX(XREG, do_sbc(val));
            break;
        case 0xFE: // INC, MEM,x
            OP_RMW_ABS_IDX(XREG, ff(++val));
            break;
            

        case 0x00: // BRK
        default:   // UNRECOGNIZED OPCODE (treat as BRK)
            {
                if (cfg.die_on_brk) {
                    DIE(0, "%s (--die-on-brk)\n",
                        op == 0? "BRK" : "ILLEGAL OP");
                    DIE(0, "  (CPU state follows.)\n");

                    util_print_state(stderr);
                    exit(3);
                }

                // XXX cycles and behavior not realistic
                //  for non-break unsupported op-codes
                PC_ADV;
                cycle(); // end 2

                stack_push(HI(PC));
                cycle(); // 3
                stack_push(LO(PC));
                cycle(); // 4
                stack_push_flags_or(PMASK(PBRK));
                cycle(); // 5

                byte pcL = peek(LOC_BRK);
                cycle(); // 6
                byte pcH = peek(LOC_BRK + 1);
                go_to(WORD(pcL, pcH));
                PPUT(PINT,1);
                cycle(); // 7
            }
            break;
    }

    ++instr_count;
}
