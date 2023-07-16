#ifndef BOBBIN_IFACE_SIMPLE_H
#define BOBBIN_IFACE_SIMPLE_H

#include "bobbin-internal.h"

extern void iface_simple_instr_init(void);
extern void iface_simple_instr_hook(void);
extern int iface_simple_getb_hook(word loc);

#endif /* BOBBIN_IFACE_SIMPLE_H */
