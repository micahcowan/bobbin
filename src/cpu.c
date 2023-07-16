#include "bobbin-internal.h"

Cpu theCpu;

void cpu_reset(void)
{
    PFLAGS |= PMASK(PUNUSED);
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
        word orig = PC; \
        (void) mem_get_byte(PC); \
        if (test) { \
            word offset = immed; \
            if (offset & 0x80) offset |= 0xFF00; \
            word addr = PC + offset; \
            go_to(WORD(HI(PC), LO(addr))); \
            cycle(); /* 3 */\
            (void) mem_get_byte(PC); \
            if (PC != addr) { \
                cycle(); /* 4 */ \
                go_to(addr); \
                (void) mem_get_byte(PC); \
            } \
            cycle(); /* 4 or 5 */ \
        } else { \
            /* https://www.nesdev.org/6502_cpu.txt
                says "else increment PC", but that's
                clearly an error. */ \
            cycle(); /* 3 */ \
        } \
    } while (0)

#define OP_READ_INDY(exec) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        byte lo = mem_get_byte(immed); \
        cycle(); /* 3 */ \
        byte hi = mem_get_byte(immed + 1); \
        word addr = WORD(lo, hi) + YREG; \
        word wrAddr = WORD(LO(lo + YREG), hi); \
        cycle(); /* 4 */ \
        byte val = mem_get_byte(wrAddr); \
        if (addr == wrAddr) { \
            exec; \
        } else { \
            cycle(); /* 5 */ \
            val = mem_get_byte(addr); \
            exec; \
        } \
        cycle(); /* 5 or 6 */ \
    } while (0)

#define OP_READ_ZP_IDX(reg, exec) \
    do { \
        PC_ADV; \
        cycle(); \
        (void) mem_get_byte(immed); \
        cycle(); \
        byte val = mem_get_byte(LO(immed + reg)); \
        exec; \
    } while (0)

#define OP_RMW_ZP_IDX(reg, exec) \
    do { \
        PC_ADV; \
        cycle(); \
        (void) mem_get_byte(immed); \
        cycle(); \
        byte addr = LO(immed + reg); \
        byte val = mem_get_byte(addr); \
        cycle(); \
        mem_put_byte(addr, val); \
        cycle(); \
        exec; \
        mem_put_byte(addr, val); \
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
        byte val = mem_get_byte(wrAddr); \
        if (addr == wrAddr) { \
            exec; \
        } else { \
            cycle(); \
            val = mem_get_byte(addr); \
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
        byte val = mem_get_byte(wrAddr); \
        cycle(); /* 4 */ \
        val = mem_get_byte(addr); \
        cycle(); /* 5 */ \
        mem_put_byte(addr, val); \
        cycle(); /* 6 */ \
        exec; \
        mem_put_byte(addr, val); \
        cycle(); /* 7 */ \
    } while (0)

#define OP_WRITE_INDX(valReg) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        (void) mem_get_byte(immed); \
        cycle(); /* 3 */ \
        immed += XREG; \
        byte lo = mem_get_byte(immed); \
        cycle(); /* 4 */ \
        immed += 1; \
        byte hi = mem_get_byte(immed); \
        cycle(); /* 5 */ \
        mem_put_byte(WORD(lo, hi), valReg); \
        cycle(); /* 6 */ \
    } while (0)

#define OP_WRITE_ZP(reg) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        mem_put_byte(immed, reg); \
        cycle(); /* 3 */ \
    } while (0)

#define OP_WRITE_ZP_IDX(idxReg, valReg) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        (void) mem_get_byte(immed); \
        cycle(); /* 3 */ \
        mem_put_byte(LO(immed + idxReg), valReg); \
        cycle();  /* 4 */ \
    } while (0)

#define OP_WRITE_ABS_IDX(idxReg, valReg) \
    do { \
        byte lo = immed; \
        PC_ADV; \
        cycle(); /* 2 */ \
        byte hi = pc_get_adv(); \
        cycle(); /* 3 */ \
        (void) mem_get_byte(WORD(LO(lo + idxReg), hi)); \
        cycle(); /* 4 */ \
        mem_put_byte(WORD(lo, hi) + idxReg, valReg); \
        cycle(); /* 5 */ \
    } while (0)

#define OP_WRITE_INDY(valReg) \
    do { \
        PC_ADV; \
        cycle(); /* 2 */ \
        byte lo = mem_get_byte(immed); \
        cycle(); /* 3 */ \
        byte hi = mem_get_byte(LO(immed + 1)); \
        cycle(); /* 4 */ \
        (void) mem_get_byte(WORD(LO(lo+YREG), hi)); \
        cycle(); /* 5 */ \
        mem_put_byte(WORD(lo, hi)+YREG, valReg); \
        cycle(); /* 6 */ \
    } while (0)

#define OP_WRITE_ABS(valReg) \
    do { \
        byte lo = immed; \
        PC_ADV; \
        cycle(); /* 2 */ \
        byte hi = pc_get_adv(); \
        cycle(); /* 3 */ \
        mem_put_byte(WORD(lo, hi), valReg); \
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
    return val << 1;
}

static inline byte do_rol(byte val)
{
    carry_from_msb(val);
    return val << 1 | PTEST(PCARRY);
}

static inline byte do_lsr(byte val)
{
    carry_from_lsb(val);
    return val >> 1;
}

static inline byte do_ror(byte val)
{
    carry_from_lsb(val);
    return val >> 1 | (PTEST(PCARRY) << 7);
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
        // BCD. Implementation based on description
        //  from https://www.nesdev.org/6502_cpu.txt
        byte sumL = (ACC & 0xF) + (val & 0xF) + PGET(PCARRY);
        byte sumH = (ACC >> 4) + (val >> 4) + (sumL & 0x10);
        if (sumL > 9) sumL += 6;
        PPUT(PZERO, ((ACC + val + PGET(PCARRY)) & 0xFF) == 0);
        PPUT(PNEG, (sumH & 0x8) != 0);
        PPUT(POVERFL, (((sumH << 4) ^ ACC) & 0x80)
                        && !((ACC ^ val) & 0x80));
        if (sumH > 9) sumH += 6;
        PPUT(PCARRY, sumH > 0xF);
        ACC = LO(((sumH << 4) | (sumL & 0xF)));
    } else {
        word sum = ACC + val + PGET(PCARRY);
        PPUT(PNEG, sum & 0x80);
        PPUT(POVERFL, (val & 0x80) != (sum & 0x80));
        PPUT(PZERO, LO(sum) == 0);
        PPUT(PCARRY, sum & 0x100);
        ACC = LO(sum);
    }
}

static inline void do_sbc(byte val)
{
    if (PTEST(PDEC)) {
        // BCD. Implementation based on description
        //  from https://www.nesdev.org/6502_cpu.txt
        byte diffL = (ACC & 0xF) - (val & 0xF) - !PGET(PCARRY);
        if (diffL & 0x10) diffL -= 6;
        byte diffH = (ACC >> 4) - (val >> 4) - (diffL & 0x10);
        if (diffH & 0x10) diffH += 6;

        PPUT(PZERO, ((ACC + val + PGET(PCARRY)) & 0xFF) == 0);
        PPUT(PNEG, (diffH & 0x8) != 0);
        PPUT(POVERFL, (((diffH << 4) ^ ACC) & 0x80)
                        && !((ACC ^ val) & 0x80));
        if (diffH > 9) diffH += 6;

        word diff = ACC - val - !PGET(PCARRY);
        PPUT(PNEG, diff & 0x80);
        PPUT(POVERFL, (val & 0x80) != (diff & 0x80));
        PPUT(PZERO, LO(diff) == 0);
        int borrow = (ACC < val) || (ACC == val && !PGET(PCARRY));
        PPUT(PCARRY, !borrow);

        ACC = LO(((diffH << 4) | (diffL & 0xF)));
    } else {
        word diff = ACC - val - !PGET(PCARRY);
        PPUT(PNEG, diff & 0x80);
        PPUT(POVERFL, (val & 0x80) != (diff & 0x80));
        PPUT(PZERO, LO(diff) == 0);
        int borrow = (ACC < val) || (ACC == val && !PGET(PCARRY));
        PPUT(PCARRY, !borrow);
        ACC = LO(diff);
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

    byte immed = mem_get_byte(PC);

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
            OP_RMW_ABS_IDX(XREG, ff(ACC |= val));
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
                word dest = WORD(lo, mem_get_byte(PC));
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
                stack_dec();
                cycle();
                byte p = mem_get_byte(STACK);
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
            OP_RMW_ABS_IDX(XREG, ff(ACC &= val));
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
                (void) mem_get_byte(STACK);
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
            OP_RMW_ZP_IDX(XREG, ff(ACC ^= val));
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
                byte hi = mem_get_byte(STACK);
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
            ACC = mem_get_byte(STACK);
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
                lo = mem_get_byte(addr);
                cycle(); // 4
                hi = mem_get_byte(WORD(LO(addr+1),HI(addr)));
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
            OP_READ_ABS_IDX(YREG, ff(ACC = val));
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
            

        default: // BRK
            {
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
            }
            break;
    }
}
