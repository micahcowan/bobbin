#include "bobbin-internal.h"

Cpu theCpu;

void cpu_reset(void)
{
    /* Cycle details taken from https://www.pagetable.com/?p=410 */
    (void) mem_get_byte(PC);
    cycle();
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
    cycle(); /* end of cycle 7; read vector high byte */
}

void cpu_step(void)
{
}
