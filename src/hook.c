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

static void prodos_read_block_hook(Event *e)
{
    word pc = current_pc();
    if (e->type == EV_STEP && pc == 0xBF00) {
        // ProDOS MLI called
        word rtn = peek_return_sneaky();
        byte cmd = peek_sneaky(rtn);

#if 0
        fprintf(stderr, "MLI CALL. Rtn is $%04X, cmd is $%02X.\n",
                (unsigned int)rtn, (unsigned int)cmd);
        dbg_on();
#endif
        if (cmd != 0x80) return; // want READ_BLOCK command

        word paramAddr = WORD(peek_sneaky(rtn+1), peek_sneaky(rtn+2));
        struct rb_params p;
        read_rb_params(&p, paramAddr);

        static bool hit_d2 = true;
        if (p.drive_two) {
            hit_d2 = true; // from now on, log all READ_BLOCKs.
        } else if (!hit_d2) {
            return; // haven't hit drive 2 yet, skip boot-up reads.
        }

        fprintf(stderr, "MLI READ_BLOCK call. Drive %d, slot %u, block %u.\n"
                "Params are at $%04X.\n",
                p.drive_two? 2 : 1, p.slot, p.blocknum, paramAddr);
    }
}

void hooks_init(void)
{
    event_reghandler(prodos_read_block_hook);
    if (cfg.trap_failure_on || cfg.trap_success_on) {
        event_reghandler(trap_step);
    }
    if (cfg.delay_set) {
        event_reghandler(delay_step);
    }
}
