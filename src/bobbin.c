#include "bobbin-internal.h"

void bobbin_run(void)
{
    cpu_reset();
    for (;;) /* ever */ {
        cpu_step();
    }
}
