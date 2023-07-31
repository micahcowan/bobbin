#ifndef APPLEII_H_
#define APPLEII_H_

/*
   Prefixes:
     ZP  - Zero-Page RZM location used by f/w
     LOC - RAM location used by f/w
     SS  - Soft Switches and other $C0XX stuff
     FP  - AppleSoft BASIC (floating point) f/w
     INT - Integer BASIC f/w
     MON - Autostart F8 ROM
     OLD - pre-Autostart F8 ROM
     VEC - one of the address vectors in the last six bytes of firmware
*/

#define ZP_TXTTAB       0x67
#define ZP_VARTAB       0x69 // LOMEM
#define ZP_ARYTAB       0x6B
#define ZP_STREND       0x6D
#define ZP_FRETOP       0x6F
#define ZP_MEMSIZE      0x73 // HIMEM
#define ZP_PRGEND       0xAF

#define LOC_SOFTEV      0x03F2
#define LOC_PWREDUP     0x03F4

#define SS_KBD          0xC000
#define SS_KBDSTROBE    0xC010

#define INT_BASIC       0xE000 /* jmp to cold start */
#define INT_BASIC2      0xE003 /* jmp to warm start */
#define INT_SETPROMPT   0xE006

#define FP_BASIC        0xE000 /* jmp to cold start */
#define FP_BASIC2       0xE003 /* jmp to warm start */

#define MON_IRQ         0xFA40
#define MON_BREAK       0xFA4C
#define MON_GETLNZ      0xFD67
#define MON_GETLN       0xFD6A
#define MON_NXTCHR      0xFD75
#define MON_COUT1       0xFDF0
#define MON_MONZ        0xFF69

#define VEC_NMI         0xFFFA
#define VEC_RESET       0xFFFC
#define VEC_IRQ         0xFFFE
#define VEC_BRK         VEC_IRQ

#endif // APPLEII_H_
