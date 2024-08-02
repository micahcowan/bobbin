//  hook.c
//
//  Copyright (c) 2023 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

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

static FILE *memlog;
static SoftSwitches savedsw;
static void log_prodos_switches(Event *e)
{
    switch (e->type) {
        case EV_RESET:
            // Don't squawk about reset switch changes,
            // Just update savedsw.
            memcpy(savedsw, ss, (sizeof ss)/(sizeof ss[0]));
            break;
        case EV_SWITCH:
        {
            const char *name = get_switch_name(e->val);
            const char oldsw = swget(savedsw, e->val)? 'T' : 'F';
            const char newsw = swget(ss, e->val)? 'T' : 'F';
            fprintf(memlog, "%04X: sw chg: %s: %c -> %c\n",
                    (unsigned int)current_pc(), name, oldsw, newsw);
            swset(savedsw, e->val, swget(ss, e->val));
        }
            break;
        case EV_PEEK:
        case EV_POKE:
        {
            word loc = e->loc;
            size_t aloc = e->aloc;
            if (aloc >= 0xC000 && aloc < 0xC090 && aloc != 0xC030) {
                fprintf(memlog, "%04X: sw %s of %04zX\n",
                        (unsigned int)current_pc(),
                        e->type == EV_POKE? "wr" : "rd", aloc);
            } else if (loc >= 0xD000 && loc < 0xE000) {
                fprintf(memlog, "%04X: LC %s of %04zX (%s%s)\n",
                        (unsigned int)current_pc(),
                        e->type == EV_POKE? "wr" : "rd", aloc,
                        e->aux? "AUX ": "", mem_get_acctype_name(e->acctype));
            }
        }
            break;
        default:
            ;
    }
}

void hooks_init(void)
{
//#define MEMLOG
#ifdef MEMLOG
    memlog = fopen("memlog.log", "w");
    if (memlog == NULL) DIE(1,"Couldn't open memlog.\n");
    memset(savedsw, 0, (sizeof savedsw)/(sizeof savedsw[0]));
    event_reghandler(log_prodos_switches);
#endif

    if (cfg.trap_failure_on || cfg.trap_success_on) {
        event_reghandler(trap_step);
    }
}
