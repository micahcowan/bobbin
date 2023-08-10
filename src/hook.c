#include "bobbin-internal.h"

void trap_step(void)
{
    if (cfg.trap_failure_on && current_pc() == cfg.trap_failure) {
        fputs("*** ERROR TRAP REACHED ***\n", stderr);
        fprintf(stderr, "Instr #: %ju\n", instr_count);
        fprintf(stderr, "Failed testcase: %02X\n",
                peek_sneaky(0x200));

        // Get PC for if we did an RTS
        Registers regs = theCpu.regs;
        byte lo = peek_sneaky(0x100 | ++regs.sp);
        byte hi = peek_sneaky(0x100 | ++regs.sp);
        regs.pc = WORD(lo, hi)+1;
        regs.pc -= 3; // back up to the instr that "called" us.
        util_print_state(stderr, regs.pc, &regs);
        exit(3);
    } else if (cfg.trap_success_on && current_pc() == cfg.trap_success) {
        fputs(".-= !!! REPORT SUCCESS !!! =-.\n", stderr);
        exit(0);
    }
}
