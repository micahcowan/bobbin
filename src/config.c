#include "bobbin-internal.h"

#include <stdio.h>
#include <string.h>

Config cfg = {
    .remain_after_pipe = false,
    .interface = NULL,
    .machine = "//e",
};

typedef enum {
    T_BOOL = 0,
    T_STRING_ARG = 1,
} OptType;

typedef struct OptInfo OptInfo;
struct OptInfo {
    const char * const *    names;
    OptType                 type;
    void *                  arg;
};

const char * const INTERFACE_OPT_NAMES[] = {"interface","if","iface",NULL};
const char * const MACHINE_OPT_NAMES[] = {"machine","m","mach",NULL};
const char * const REMAIN_OPT_NAMES[] = {"remain-after-pipe","remain",NULL};

const OptInfo options[] = {
    { INTERFACE_OPT_NAMES, T_STRING_ARG, &cfg.interface },
    { MACHINE_OPT_NAMES, T_STRING_ARG, &cfg.machine },
    { REMAIN_OPT_NAMES, T_BOOL, &cfg.remain_after_pipe },
};

static const OptInfo *find_option(const char *opt)
{
    const OptInfo *visit = &options[0];
    const OptInfo *end   = visit + (sizeof options)/(sizeof options[0]);

    for (; visit != end; ++visit) {
        for (const char * const *name = visit->names;
             *name != NULL;
             ++name) {

            if (STREQ(*name, opt)) {
                return visit;
            }
        }
    }

    return NULL;
}

void do_config(int c, char **v)
{
    // Set up config
    
    ++v; // skip program name
    for (; *v != NULL; ++v) {
        // Does it start with "-" or "--"?
        if (**v != '-')
            DIE(2, "Unexpected non-option argument \"%s\".\n", *v);
        // Skip dashes (--)
        do { 
            ++(*v);
        } while (**v == '-');

        const char *opt = *v;
        // Is there an '='?
        char *eq = strchr(*v, '=');
        const char *arg = NULL;
        if (eq) {
            *eq = '\0';
            arg = eq+1;
        }

        const OptInfo *info = find_option(opt);
        if (!info && opt[0] == 'n' && opt[1] == 'o') {
            const char *yesopt = opt + 2;
            while (*yesopt == '-') ++yesopt;
            info = find_option(yesopt);
        }
        if (!info) DIE(2, "Unknown option \"--%s\".\n", opt);

        if (info->type == T_BOOL) {
            bool b = !(opt[0] == 'n' && opt[1] == 'o');

            (*(bool *)info->arg) = b;
        } else {
            // All options get args, for now
            if (!arg) {
                // See if there's a following arg
                ++v;
                if (!*v) {
                    DIE(2, "Option \"--%s\" requires an argument.\n", opt);
                }
                else {
                    arg = *v;
                }
            }

            (*(const char **)info->arg) = arg;
        }
    }
}
