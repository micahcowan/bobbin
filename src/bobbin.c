#include "bobbin-internal.h"

void bobbin_run(void)
{
    mem_init();
    cpu_reset();
    for (;;) /* ever */ {
        cpu_step();
    }
}
