//  disasm.c
//
//  Copyright (c) 2023-2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <stdio.h>

static const char *get_op_mnem(byte op)
{
    // O-okay. I know this looks like complete butt,
    // and I could probably make something both prettier and also
    // shorter, by exploiting patterns in the op codes, and then just
    // pick out the exceptions. But the quickest path to "done"
    // was to just grab the mega switch statement from cpu.c's
    // cpu_step(), and edit it... so that's what I did. Maybe I'll fix
    // it up later, dunno.
    switch (op) {
        case 0x00:
            return "BRK";
        case 0x01:
            return "ORA";
        case 0x05:
            return "ORA";
        case 0x06:
            return "ASL";
        case 0x08:
            return "PHP";
        case 0x09:
            return "ORA";
        case 0x0A:
            return "ASL";
        case 0x0D:
            return "ORA";
        case 0x0E:
            return "ASL";
        case 0x10:
            return "BPL";
        case 0x11:
            return "ORA";
        case 0x15:
            return "ORA";
        case 0x16:
            return "ASL";
        case 0x18:
            return "CLC";
        case 0x19:
            return "ORA";
        case 0x1D:
            return "ORA";
        case 0x1E:
            return "ASL";


        case 0x20:
            return "JSR";
        case 0x21:
            return "AND";
        case 0x24:
            return "BIT";
        case 0x25:
            return "AND";
        case 0x26:
            return "ROL";
        case 0x28:
            return "PLP";
        case 0x29:
            return "AND";
        case 0x2A:
            return "ROL";
        case 0x2C:
            return "BIT";
        case 0x2D:
            return "AND";
        case 0x2E:
            return "ROL";
        case 0x30:
            return "BMI";
        case 0x31:
            return "AND";
        case 0x35:
            return "AND";
        case 0x36:
            return "ROL";
        case 0x38:
            return "SEC";
        case 0x39:
            return "AND";
        case 0x3D:
            return "AND";
        case 0x3E:
            return "ROL";


        case 0x40:
            return "RTI";
        case 0x41:
            return "EOR";
        case 0x45:
            return "EOR";
        case 0x46:
            return "LSR";
        case 0x48:
            return "PHA";
        case 0x49:
            return "EOR";
        case 0x4A:
            return "LSR";
        case 0x4C:
            return "JMP";
        case 0x4D:
            return "EOR";
        case 0x4E:
            return "LSR";
        case 0x50:
            return "BVC";
        case 0x51:
            return "EOR";
        case 0x55:
            return "EOR";
        case 0x56:
            return "LSR";
        case 0x58:
            return "CLI";
        case 0x59:
            return "EOR";
        case 0x5D:
            return "EOR";
        case 0x5E:
            return "LSR";


        case 0x60:
            return "RTS";
        case 0x61:
            return "ADC";
        case 0x65:
            return "ADC";
        case 0x66:
            return "ROR";
        case 0x68:
            return "PLA";
        case 0x69:
            return "ADC";
        case 0x6A:
            return "ROR";
        case 0x6C:
            return "JMP";
        case 0x6D:
            return "ADC";
        case 0x6E:
            return "ROR";
        case 0x70:
            return "BVS";
        case 0x71:
            return "ADC";
        case 0x75:
            return "ADC";
        case 0x76:
            return "ROR";
        case 0x78:
            return "SEI";
        case 0x79:
            return "ADC";
        case 0x7D:
            return "ADC";
        case 0x7E:
            return "ROR";


        case 0x81:
            return "STA";
        case 0x84:
            return "STY";
        case 0x85:
            return "STA";
        case 0x86:
            return "STX";
        case 0x88:
            return "DEY";
        case 0x8A:
            return "TXA";
        case 0x8C:
            return "STY";
        case 0x8D:
            return "STA";
        case 0x8E:
            return "STX";
        case 0x90:
            return "BCC";
        case 0x91:
            return "STA";
        case 0x94:
            return "STY";
        case 0x95:
            return "STA";
        case 0x96:
            return "STX";
        case 0x98:
            return "TYA";
        case 0x99:
            return "STA";
        case 0x9A:
            return "TXS";
        case 0x9D:
            return "STA";


        case 0xA0:
            return "LDY";
        case 0xA1:
            return "LDA";
        case 0xA2:
            return "LDX";
        case 0xA4:
            return "LDY";
        case 0xA5:
            return "LDA";
        case 0xA6:
            return "LDX";
        case 0xA8:
            return "TAY";
        case 0xA9:
            return "LDA";
        case 0xAA:
            return "TAX";
        case 0xAC:
            return "LDY";
        case 0xAD:
            return "LDA";
        case 0xAE:
            return "LDX";
        case 0xB0:
            return "BCS";
        case 0xB1:
            return "LDA";
        case 0xB4:
            return "LDY";
        case 0xB5:
            return "LDA";
        case 0xB6:
            return "LDX";
        case 0xB8:
            return "CLV";
        case 0xB9:
            return "LDA";
        case 0xBA:
            return "TSX";
        case 0xBC:
            return "LDY";
        case 0xBD:
            return "LDA";
        case 0xBE:
            return "LDX";

        case 0xC0:
            return "CPY";
        case 0xC1:
            return "CMP";
        case 0xC4:
            return "CPY";
        case 0xC5:
            return "CMP";
        case 0xC6:
            return "DEC";
        case 0xC8:
            return "INY";
        case 0xC9:
            return "CMP";
        case 0xCA:
            return "DEX";
        case 0xCC:
            return "CPY";
        case 0xCD:
            return "CMP";
        case 0xCE:
            return "DEC";
        case 0xD0:
            return "BNE";
        case 0xD1:
            return "CMP";
        case 0xD5:
            return "CMP";
        case 0xD6:
            return "DEC";
        case 0xD8:
            return "CLD";
        case 0xD9:
            return "CMP";
        case 0xDD:
            return "CMP";
        case 0xDE:
            return "DEC";


        case 0xE0:
            return "CPX";
        case 0xE1:
            return "SBC";
        case 0xE4:
            return "CPX";
        case 0xE5:
            return "SBC";
        case 0xE6:
            return "INC";
        case 0xE8:
            return "INX";
        case 0xE9:
            return "SBC";
        case 0xEA:
            return "NOP";
        case 0xEC:
            return "CPX";
        case 0xED:
            return "SBC";
        case 0xEE:
            return "INC";
        case 0xF0:
            return "BEQ";
        case 0xF1:
            return "SBC";
        case 0xF5:
            return "SBC";
        case 0xF6:
            return "INC";
        case 0xF8:
            return "SED";
        case 0xF9:
            return "SBC";
        case 0xFD:
            return "SBC";
        case 0xFE:
            return "INC";

        default:
            return "???";
    }
}

#define T_UNKNOWN       0
#define T_IMPLIED       1
#define T_INDX          2
#define T_ZP            3
#define T_IMMEDIATE     4
#define T_ABSOLUTE      5
#define T_RELATIVE      6
#define T_INDY          7
#define T_ZP_X          8
#define T_ABS_Y         9
#define T_ABS_X         10
#define T_ZP_Y          11
#define T_JMP_IND       12

static const char n_oprnd[]
    = {0, 0, 1, 1, 1, 2, 1, 1, 1, 2, 2, 1, 2};

#define PRA FILE *f, word pc, byte a[2]
#define RP  return fprintf
int pr_none(PRA) { return 0; }
int pr_indx(PRA) { RP(f, "($%02X,x)", a[0]); }
int pr_zp(PRA) { RP(f, "$%02X", a[0]); }
int pr_imm(PRA) { RP(f, "#$%02X", a[0]); }
int pr_abs(PRA) { RP(f, "$%04X", WORD(a[0],a[1])); }
int pr_indy(PRA) { RP(f, "($%02X),y", a[0]); }
int pr_zp_x(PRA) { RP(f, "$%02X,x", a[0]); }
int pr_zp_y(PRA) { RP(f, "$%02X,y", a[0]); }
int pr_abs_x(PRA) { RP(f, "$%04X,x", WORD(a[0],a[1])); }
int pr_abs_y(PRA) { RP(f, "$%04X,y", WORD(a[0],a[1])); }
int pr_jmp_ind(PRA) { RP(f, "($%04X)", WORD(a[0],a[1])); }

int pr_rel(PRA) {
    word offset = a[0];
    if (offset & 0x80) offset |= 0xFF00;

    RP(f, "$%04X", (unsigned int)((pc + offset + 2) & 0xFFFF));
}

int (* const handlers[])(PRA) = {
    pr_none,
    pr_none,
    pr_indx,
    pr_zp,
    pr_imm,
    pr_abs,
    pr_rel,
    pr_indy,
    pr_zp_x,
    pr_abs_y,
    pr_abs_x,
    pr_zp_y,
    pr_jmp_ind,
};

static int get_op_type(byte op)
{
    if ((op & 3) == 3)
        return T_UNKNOWN;

    if (op == 0x20)
        return T_ABSOLUTE;
    if (op == 0x6C)
        return T_JMP_IND;

    switch (op & 0x9F) {
        case 0x00: case 0x08: case 0x0A: case 0x18: case 0x88:
        case 0x8A: case 0x98: case 0x9A:
            return T_IMPLIED;
        case 0x01: case 0x81:
            return T_INDX;
        default: case 0x02: case 0x12: case 0x14: case 0x1A:
        case 0x1C: case 0x92:
            return T_UNKNOWN;
        case 0x04: case 0x05: case 0x06: case 0x84: case 0x85: case 0x86:
            return T_ZP;
        case 0x09: case 0x80: case 0x82: case 0x89:
            return T_IMMEDIATE;
        case 0x0C: case 0x0D: case 0x0E: case 0x8C: case 0x8D: case 0x8E:
            return T_ABSOLUTE;
        case 0x10: case 0x90:
            return T_RELATIVE;
        case 0x11: case 0x91:
            return T_INDY;
        case 0x15: case 0x16: case 0x94: case 0x95:
            return T_ZP_X;
        case 0x19: case 0x99:
            return T_ABS_Y;
        case 0x1D: case 0x1E: case 0x9C: case 0x9D:
            return T_ABS_X;
        case 0x96:
            return (op == 0x96 || op == 0xB6)? T_ZP_Y : T_ZP_X;
        case 0x9E:
            return (op == 0xBE)? T_ABS_Y : T_ABS_X;
    }
}

static void pracc_abs(FILE *f, word addr)
{
    fprintf(f, "%04X: ", addr);
    for (int i=0; i!=5; ++i) {
        byte b = peek_sneaky(addr++);
        fprintf(f, " %02X", b);
    }
}

static void pracc_zp(FILE *f, byte addr)
{
    fprintf(f, "%02X: %02X %02X   ",
            addr,
            peek_sneaky(addr),
            peek_sneaky(LO(addr+1)));
}

static void print_access(FILE *f, word pc, const Registers *regs,
                         int type, byte m[2])
{
    switch (type) {
        case T_ZP:
            pracc_abs(f, WORD(m[0], 0));
            break;
        case T_ABSOLUTE:
            pracc_abs(f, WORD(m[0], m[1]));
            break;
        case T_ZP_X:
            pracc_abs(f, WORD(LO(m[0] + regs->x), 0));
            break;
        case T_ZP_Y:
            pracc_abs(f, WORD(LO(m[0] + regs->y), 0));
            break;
        case T_ABS_X:
            pracc_abs(f, WORD(m[0], m[1]) + regs->x);
            break;
        case T_ABS_Y:
            pracc_abs(f, WORD(m[0], m[1]) + regs->y);
            break;

        case T_INDY:
            {
                pracc_zp(f, m[0]);
                byte lo = peek_sneaky(m[0]);
                byte hi = peek_sneaky(m[0]+1);
                pracc_abs(f, WORD(lo, hi) + regs->y);
            }
            break;
        case T_INDX:
            {
                pracc_zp(f, LO(m[0] + regs->x));
                byte lo = peek_sneaky(m[0]);
                byte hi = peek_sneaky(m[0]+1);
                pracc_abs(f, WORD(lo, hi));
            }
            break;
        case T_JMP_IND:
            pracc_abs(f, WORD(m[0], m[1]));
            break;
        default:
            ;
    }
}

word print_disasm(FILE *f, word pc, const Registers *regs)
{
    byte m[3];
    for (int i=0; i != (sizeof m); ++i) {
        m[i] = peek_sneaky(pc+i);
    }

    const char *mnem = get_op_mnem(m[0]);
    int t = get_op_type(m[0]);
    int n = n_oprnd[t];

    fprintf(f, "%04X:  ", pc);
    for (int i=0; i != (sizeof m); ++i) {
        if (i > n) {
            fprintf(f, "   ");
        } else {
            fprintf(f, " %02X", m[i]);
        }
    }

    // print mnemonic
    fprintf(f, "    %s ", mnem);

    int cnt = handlers[t](f, pc, &m[1]);

    // pad out the disassembly
    const int pad = 13; // how much space the args should take up,
                        //  *after* the mnemonic.
    if (pad > cnt) {
        for (int i=cnt; i != pad; ++i) {
            fputc(' ', f);
        }
    }

    // put extra information about any memory we're accessing
    print_access(f, pc, regs, t, &m[1]);
    fputc('\n', f);

    return pc + 1 + n;
}
