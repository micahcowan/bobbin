#include "bobbin-internal.h"

#include <stdio.h>
#include <string.h>

Config cfg = {
    .remain_after_pipe = false,
    .interface = NULL,
    .machine = "//e",
    .simple_input_mode = "apple",
};

typedef enum {
    T_BOOL = 0,
    T_STRING_ARG,
    T_ALIAS,
} OptType;

typedef struct OptInfo OptInfo;
struct OptInfo {
    const char * const *    names;
    OptType                 type;
    void *                  arg;
};

typedef const char * const AryOfStr;
static AryOfStr INTERFACE_OPT_NAMES[] = {"interface","if","iface",NULL};
static AryOfStr MACHINE_OPT_NAMES[] = {"machine","m","mach",NULL};
static AryOfStr REMAIN_OPT_NAMES[] = {"remain-after-pipe","remain",NULL};
static AryOfStr SIMPLE_OPT_NAMES[] = {"simple",NULL};
static AryOfStr SIMPLE_INPUT_OPT_NAMES[] = {"simple-input",NULL};

typedef char Alias[];
static Alias ALIAS_SIMPLE = "iface=simple";

const OptInfo options[] = {
    { INTERFACE_OPT_NAMES, T_STRING_ARG, &cfg.interface },
    { MACHINE_OPT_NAMES, T_STRING_ARG, &cfg.machine },
    { REMAIN_OPT_NAMES, T_BOOL, &cfg.remain_after_pipe },
    { SIMPLE_OPT_NAMES, T_ALIAS, (char *)ALIAS_SIMPLE },
    { SIMPLE_INPUT_OPT_NAMES, T_STRING_ARG, &cfg.simple_input_mode },
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

recheck:// Past this point, can't assume opt points at a real argv[] item
        ;
        // Is there an '='?
        char *eq = strchr(opt, '=');
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

        switch (info->type) {
            case T_BOOL:
            {
                bool b = !(opt[0] == 'n' && opt[1] == 'o');

                (*(bool *)info->arg) = b;
            }
                break;
            case T_ALIAS:
            {
                // wTHis option is an alias for another one
                //  (most likely wth an arg). Restart for that.
                opt = (char *)info->arg;
                goto recheck;
            }
                break;
            case T_STRING_ARG:
            {
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
                break;
            default:
                DIE(3, "INTERNAL ERROR: unhandled optiont type for --%s.\n",
                    opt);
        }
        if (eq) *eq = '='; // put it back, in case it was an alias
                           //  and just  housekeeping
    }
}
