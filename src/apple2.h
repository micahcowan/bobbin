#ifndef APPLEII_H_
#define APPLEII_H_

/*
   Prefixes:
     FP  - AppleSoft BASIC (floating point) f/w
     INT - Integer BASIC f/w
     MON - Autostart F8 ROM
     OLD - pre-Autostart F8 ROM
     VEC - one of the address vectors in the last six bytes of firmware
*/

#define MON_IRQ         0xFA40
#define MON_BREAK       0xFA4C

#define VEC_NMI         0xFFFA
#define VEC_RESET       0xFFFC
#define VEC_IRQ         0xFFFE
#define VEC_BRK         VEC_IRQ

#endif // APPLEII_H_
