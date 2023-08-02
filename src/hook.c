#include "bobbin-internal.h"

#include <assert.h>

static bool did_delayed_jmp = false;

static void trap_step(void)
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

void rh_prestep(void)
{
    if (cfg.delay_set && PC == cfg.delay_until && !did_delayed_jmp) {
        did_delayed_jmp = true;
        if (cfg.start_loc_set) {
            PC = cfg.start_loc;
        }
        load_ram_finish();
        rh_display_touched();
    }
    iface_prestep();
}

void rh_step(void)
{
    word pc = current_pc();
    trace_step();
    trap_step();
    iface_step();
    // For "user" hooks, we should emit a warning, and reset PC after
    // any hook that set it. But for our internal stuff, stronger action
    // is called for.
    assert(pc == current_pc());
}

int rh_peek(word loc)
{
    word pc = current_pc();
    return iface_peek(loc);
    // For "user" hooks, we should emit a warning, and reset PC after
    // any hook that set it. But for our internal stuff, stronger action
    // is called for.
    assert(pc == current_pc());
}

bool rh_poke(word loc, byte val)
{
    word pc = current_pc();
    return iface_poke(loc, val);
    // For "user" hooks, we should emit a warning, and reset PC after
    // any hook that set it. But for our internal stuff, stronger action
    // is called for.
    assert(pc == current_pc());
}

void rh_display_touched(void)
{
    iface_display_touched();
}

void rh_reboot(void)
{
    mem_reboot();
    //iface_reboot();
    iface_display_touched();
    cpu_reset();
    did_delayed_jmp = false;
}
