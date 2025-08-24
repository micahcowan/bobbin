;
; Common header for 65C02 instruction tests
; Split from the original 65C02_extended_opcodes_test.ca65 by Klaus Dormann
; Modified to use trap addresses instead of infinite loops for better emulation
;

; C O N F I G U R A T I O N

;ROM_vectors writable (0=no, 1=yes)
ROM_vectors = 1

;load_data_direct (0=move from code segment, 1=load directly)
load_data_direct = 1

;I_flag behavior (0=force enabled, 1=force disabled, 2=prohibit change, 3=allow change)
I_flag = 3

;configure memory - try to stay away from memory used by the system
;zero_page memory start address, $4e (78) consecutive Bytes required
zero_page = $a  

;data_segment memory start address, $63 (99) consecutive Bytes required
data_segment = $200
    .if (data_segment & $ff) <> 0
        .error "low byte of data_segment MUST be $00 !!"
    .endif

;code_segment memory start address, 10kB of consecutive space required
code_segment = $400  

;added WDC only opcodes WAI & STP (0=test as NOPs, >0=no test)
wdc_op = 1

;added Rockwell & WDC opcodes BBR, BBS, RMB & SMB
;(0=test as NOPs, 1=full test, >1=no test) 
rkwl_wdc_op = 1

;skip testing all undefined opcodes override
;0=test as NOP, >0=skip
skip_nop = 0

;report errors through I/O channel (0=use standard self trap loops, 1=include report.i65)
report = 0

;RAM integrity test option. Checks for undesired RAM writes.
ram_top = -1

; E N D   O F   C O N F I G U R A T I O N
        
; Modified macros to use trap addresses instead of infinite loops
    .if report = 0
        .macro  trap
        jmp $0001       ; Signal failure to emulator
        .endmacro
        .macro  trap_eq
        bne :+          ; Skip if not equal
        jmp $0001       ; Failed equal (zero)
:
        .endmacro
        .macro  trap_ne
        beq :+          ; Skip if equal
        jmp $0001       ; Failed not equal (non zero)
:
        .endmacro
        .macro  trap_cs
        bcc :+          ; Skip if carry clear
        jmp $0001       ; Failed carry set
:
        .endmacro
        .macro  trap_cc
        bcs :+          ; Skip if carry set
        jmp $0001       ; Failed carry clear
:
        .endmacro
        .macro  trap_mi
        bpl :+          ; Skip if plus
        jmp $0001       ; Failed minus (bit 7 set)
:
        .endmacro
        .macro  trap_pl
        bmi :+          ; Skip if minus
        jmp $0001       ; Failed plus (bit 7 clear)
:
        .endmacro
        .macro  trap_vs
        bvc :+          ; Skip if overflow clear
        jmp $0001       ; Failed overflow set
:
        .endmacro
        .macro  trap_vc
        bvs :+          ; Skip if overflow set
        jmp $0001       ; Failed overflow clear
:
        .endmacro
        .macro  success
        jmp $0002       ; Signal success to emulator
        .endmacro
    .endif

    .define equ =

carry   equ %00000001   ;flag bits in status
zero    equ %00000010
intdis  equ %00000100
decmode equ %00001000
break   equ %00010000
reserv  equ %00100000
overfl  equ %01000000
minus   equ %10000000

fc      equ carry
fz      equ zero
fzc     equ zero+carry
fn      equ minus
fnc     equ minus+carry
fnz     equ minus+zero
fnzc    equ minus+zero+carry
fv      equ overfl
fvc     equ overfl+carry
fvz     equ overfl+zero
fvzc    equ overfl+zero+carry
fvn     equ overfl+minus
fvnc    equ overfl+minus+carry
fvnz    equ overfl+minus+zero
fvnzc   equ overfl+minus+zero+carry
fao     equ break+reserv    ;bits always on after PHP, BRK
fai     equ fao+intdis      ;+ forced interrupt disable
faod    equ fao+decmode     ;+ ignore decimal
faid    equ fai+decmode     ;+ ignore decimal
m8      equ $ff             ;8 bit mask
m8i     equ $ff&~intdis     ;8 bit mask - interrupt disable

; Memory layout
    .segment "ZEROPAGE"
    .org    zero_page