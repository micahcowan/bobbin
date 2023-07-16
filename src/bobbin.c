#include "bobbin-internal.h"
#include "iface-simple.h"

extern void signals_init(void);

void bobbin_run(void)
{
    signals_init();
    iface_simple_instr_init();
    mem_init();
    cpu_reset();
    for (;;) /* ever */ {
        trace_instr();
        cpu_step();
    }
}
