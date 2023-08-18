#include "bobbin-internal.h"

static void trap_step(Event *e)
{
    if (e->type != EV_STEP) return;
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

static bool did_delayed_jmp = false;
void delay_step(Event *e)
{
    if (e->type == EV_REBOOT) {
        did_delayed_jmp = false;
    }
    else if (e->type == EV_STEP
             && PC == cfg.delay_until && !did_delayed_jmp) {
        did_delayed_jmp = true;
        if (cfg.start_loc_set) {
            INFO("Jumping PC to $%04X due to --delay-until option.\n",
                 cfg.start_loc_set);
            PC = cfg.start_loc;
        }
        load_ram_finish();
        event_fire(EV_DISPLAY_TOUCH);
    }
}

struct rb_params {
    bool drive_two;
    unsigned int slot;
    unsigned int blocknum;
};

static void read_rb_params(struct rb_params *p, word addr)
{
    byte unitNum = peek_sneaky(addr + 1);
    p->drive_two = (unitNum & 0x80) != 0;
    p->slot = (unitNum & 0x70) >> 4;
    p->blocknum = WORD(peek_sneaky(addr + 4), peek_sneaky(addr + 5));
}

static byte last_unitNum;
static void prodos_hook(Event *e)
{
    byte unitNum = peek_sneaky(0x43);
    if (unitNum != last_unitNum) {
        fprintf(stderr, "unitNum changed from $%02X to $%02X.\n",
                (unsigned int)last_unitNum, (unsigned int)unitNum);
#if 0
        if (last_unitNum == 0xE0 && unitNum == 0x60) {
            trace_on("$E0 -> $60");
        } else if (last_unitNum == 0x60 && unitNum == 0xE0 && tracing()) {
            trace_off();
        }
#endif
        last_unitNum = unitNum;
        util_print_state(stderr, current_pc(), &theCpu.regs);
        fputc('\n', stderr);
    }
    if (e->type == EV_STEP && current_pc() == 0xD000) {
        fprintf(stderr, "BlockIO entered. Drive %d, slot %d, block %d.\n",
                (unitNum & 0x80) != 0? 2 : 1, (unitNum & 0x70) >> 4,
                peek_sneaky(0x46));
    }
}

void hooks_init(void)
{
    if (cfg.trap_failure_on || cfg.trap_success_on) {
        event_reghandler(trap_step);
    }
    if (cfg.delay_set) {
        event_reghandler(delay_step);
    }
}
