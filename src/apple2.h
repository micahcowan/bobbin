#ifndef APPLEII_H_
#define APPLEII_H_

/*
   Prefixes:
     ZP  - Zero-Page RZM location used by f/w
     LOC - (mostly) RAM location used by f/w. Or catch-all.
     SS  - Soft Switches and other $C0XX stuff
     FP  - AppleSoft BASIC (floating point) f/w
     INT - Integer BASIC f/w
     MON - Autostart F8 ROM
     OLD - pre-Autostart F8 ROM
     VEC - one of the address vectors in the last six bytes of firmware
*/

#define ZP_START        0x00
#define ZP_DATAFLG      0x13
#define ZP_CH           0x24
#define ZP_CV           0x25
#define ZP_PROMPT       0x33
#define ZP_LINNUM       0x50
#define ZP_TXTTAB       0x67
#define ZP_VARTAB       0x69 // LOMEM
#define ZP_ARYTAB       0x6B
#define ZP_STREND       0x6D
#define ZP_FRETOP       0x6F
#define ZP_MEMSIZE      0x73 // HIMEM
#define ZP_PRGEND       0xAF
#define ZP_CHRGET       0xB1
#define ZP_CHRGOT       0xB7
#define ZP_TXTPTR       0xB8
#define ZP_END          0x0100

#define LOC_STACK       0x0100
#define LOC_STACK_END   0x0200
#define LOC_INBUF       0x0200
#define LOC_SOFTEV      0x03F2
#define LOC_PWREDUP     0x03F4
#define LOC_TEXT1       0x0400
#define LOC_TEXTEND     0x0800
#define LOC_TEXT2       0x0800
#define LOC_ASOFT_PROG  0x0801
#define LOC_HIRES1      0x2000
#define LOC_HIRES2      0x4000
#define LOC_HIRESEND    0x6000

#define SS_START        0xC000
#define SS_KBD          0xC000
#define SS_KBDSTROBE    0xC010
#define SS_LANG_CARD    0xC080
#define LOC_BSR1_START  0xC000 /* This refers to where it is in membuf,
                                  not address-space */
#define LOC_SLOTS_START 0xC100
#define LOC_SLOT_EXPANDED_AREA  0xC800

#define LOC_SLOTS_END   0xD000
#define LOC_ROM_START   0xD000
#define LOC_BSR1_END    0xD000
#define LOC_BSR2_START  0xD000
#define LOC_BSR_START   0xD000
#define INT_BASIC       0xE000 /* jmp to cold start */
#define LOC_BSR2_END    0xE000
#define LOC_BSR_END     0xE000
#define INT_BASIC2      0xE003 /* jmp to warm start */
#define INT_SETPROMPT   0xE006

#define FP_BASIC        0xE000 /* jmp to cold start */
#define FP_BASIC2       0xE003 /* jmp to warm start */
#define FP_LIST         0xD6A5
#define FP_NEWSTT       0xD7D2

// The following locations may be specific to the ][+ firmware only
#define FP_ERROR2       0xD419 /* error handling when no ON ERR */
#define FP_RESTART      0xD43C /* CR emitted before prompt in AppleSoft */
#define FP_NOT_NUMBERED 0xD456
#define FP_LINE_EXISTS  0xD471
#define FP_CK_PAST_LINE 0xD63A /* If carry is clear at this loc,
                                  we found where to insert a new BASIC
                                  line, but it's not past the end of
                                  the program. */
#define FP_STORE_NONTOK     0xD5CF
#define FP_LOAD_TOK_CHR1    0xD5AB
#define FP_LOAD_TOK_CHR2    0xD5C1
#define FP_RUN          0xD912
#define MON_CAPTST1     0xFD80  /* set carry clear here to avoid
                                   apple ][+ converting upper -> lowercase */
// End ][+-specific locs

#define MON_IRQ         0xFA40
#define MON_BREAK       0xFA4C
#define MON_KEYIN       0xFD1B
#define MON_GETLNZ      0xFD67
#define MON_GETLN       0xFD6A
#define MON_NXTCHR      0xFD75
#define MON_COUT1       0xFDF0
#define MON_GO          0xFEB6
#define MON_MONZ        0xFF69

#define VEC_NMI         0xFFFA
#define VEC_RESET       0xFFFC
#define VEC_IRQ         0xFFFE
#define VEC_BRK         VEC_IRQ

// Warning, the following definitions won't fit in a word!
#define LOC_ADDRESSABLE_END     0x10000
#define LOC_AUX_START   0x10000
#endif // APPLEII_H_
