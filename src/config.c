#include "bobbin-internal.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Config cfg = {
    .squawk_level = DEFAULT_LEVEL,
    .remain_after_pipe = false,
    .interface = NULL,
    .machine = "//e",
    .amt_ram = 128 * 1024,
    .load_rom = true,
    .ram_load_file = NULL,
    .ram_load_loc = 0,
    .turbo = true,
    .turbo_was_set = false,
    .simple_input_mode = "apple",
    .die_on_brk = false,
    .trace_file = "trace.log",
    .trace_start = 0,
    .trace_end = 0,
    .trap_success_on = false,
    .trap_failure_on = false,
};

typedef enum {
    T_BOOL = 0,
    T_STRING_ARG,
    T_WORD_ARG,
    T_ULONG_ARG,
    T_FN_ARG,
    T_FUNCTION,
    T_INCREMENT,
    T_INT_RESET,
    T_ALIAS,
} OptType;

typedef struct OptInfo OptInfo;
struct OptInfo {
    const char * const *    names;
    OptType                 type;
    void *                  arg;
    bool *                  was_set;
};

typedef const char * const AryOfStr;

#include "help-text.h"      // a make-generated file, from README.md

#include "option-names.h"   // a make-generated file, from README.md

/*
    static AryOfStr VERBOSE_OPT_NAMES[] = {"verbose","v",NULL};
    static AryOfStr QUIET_OPT_NAMES[] = {"quiet","q",NULL};

    ...etc. referenced below.
*/
static AryOfStr VV_OPT_NAMES[] = {"vv",NULL};
// ^ not a documented option, so we rolled this by hand. But

typedef char Alias[];
static Alias ALIAS_SIMPLE = "iface=simple";

struct fn { void (*fn)(void); };
struct fnarg { void (*fn)(const char *s); };
// ^ needed because using (void*) to store a fn pointer is not
//   kosher by the standard

void do_vv(void) { cfg.squawk_level += 2; }
struct fn vv = {do_vv};
void do_help(void);
struct fn help = {do_help};
void do_ram(const char *s);
struct fnarg ramfn = {do_ram};
void do_trace_to(const char *s);
struct fnarg trace_to_fn = {do_trace_to};

const OptInfo options[] = {
    { HELP_OPT_NAMES, T_FUNCTION, &help },
    { QUIET_OPT_NAMES, T_INT_RESET, &cfg.squawk_level },
    { VERBOSE_OPT_NAMES, T_INCREMENT, &cfg.squawk_level },
    { VV_OPT_NAMES, T_FUNCTION, &vv },
    { MACHINE_OPT_NAMES, T_STRING_ARG, &cfg.machine },
    { TURBO_OPT_NAMES, T_BOOL, &cfg.turbo, &cfg.turbo_was_set },
    { RAM_OPT_NAMES, T_FN_ARG, &ramfn },
    { ROM_OPT_NAMES, T_BOOL, &cfg.load_rom },
    { LOAD_OPT_NAMES, T_STRING_ARG, &cfg.ram_load_file },
    { LOAD_AT_OPT_NAMES, T_ULONG_ARG, &cfg.ram_load_loc },
    { IF_OPT_NAMES, T_STRING_ARG, &cfg.interface },
    { SIMPLE_OPT_NAMES, T_ALIAS, (char *)ALIAS_SIMPLE },
    { REMAIN_OPT_NAMES, T_BOOL, &cfg.remain_after_pipe },
    { REMAIN_TTY_OPT_NAMES, T_BOOL, &cfg.remain_tty },
    { SIMPLE_INPUT_OPT_NAMES, T_STRING_ARG, &cfg.simple_input_mode },
    { DIE_ON_BRK_OPT_NAMES, T_BOOL, &cfg.die_on_brk },
    { TRACE_FILE_OPT_NAMES, T_STRING_ARG, &cfg.trace_file },
    { TRACE_TO_OPT_NAMES, T_FN_ARG, &trace_to_fn },
    { TRAP_FAILURE_OPT_NAMES, T_WORD_ARG, &cfg.trap_failure,
        &cfg.trap_failure_on },
    { TRAP_SUCCESS_OPT_NAMES, T_WORD_ARG, &cfg.trap_success,
        &cfg.trap_success_on },
    { START_AT_OPT_NAMES, T_WORD_ARG, &cfg.start_loc, &cfg.start_loc_set },
    { DELAY_UNTIL_PC_OPT_NAMES, T_WORD_ARG, &cfg.delay_until, &cfg.delay_set },
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
            if (info->type != T_BOOL)
                info = NULL;
        }
        if (!info) DIE(2, "Unknown option \"--%s\".\n", opt);

        // Mark the option as set.
        if (info->was_set != NULL)
            (*info->was_set) = true;

        switch (info->type) {
            case T_STRING_ARG:
            case T_FN_ARG:
            case T_WORD_ARG:
            case T_ULONG_ARG:
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
            default:
                ;
        }

        switch (info->type) {
            case T_FUNCTION:
            {
                ((struct fn *)info->arg)->fn();
            }
                break;
            case T_FN_ARG:
            {
                ((struct fnarg *)info->arg)->fn(arg);
            }
                break;
            case T_INCREMENT:
                ++(*(int *)info->arg);
                break;
            case T_INT_RESET:
                (*(int *)info->arg) = 0;
                break;
            case T_BOOL:
            {
                bool b = !(opt[0] == 'n' && opt[1] == 'o');

                (*(bool *)info->arg) = b;
            }
                break;
            case T_ALIAS:
            {
                // This option is an alias for another one
                //  (most likely wth an arg). Restart for that.
                opt = (char *)info->arg;
                goto recheck;
            }
                break;
            case T_ULONG_ARG:
            case T_WORD_ARG:
            {
                char *end;
                if (arg[0] == '$') {
                    ++arg;
                }
                errno = 0;
                unsigned long ul = strtoul(arg, &end, 16);
                if (*end != '\0') {
                    DIE(2,"Garbage at end of arg to --%s.\n", opt);
                } else if (errno == ERANGE || errno == EINVAL) {
                    DIE(2,"Could not parse numeric arg to --%s.\n", opt);
                }
                if (info->type == T_WORD_ARG) {
                    if (ul > 65535) {
                        DIE(2,"Argument to --%s is too large (max 0xFFFF).\n",
                            opt);
                    }
                    // need to make VERY sure that it's a word,
                    // represented in Apple 's little-endian format
                    // (regardless of native architecture).
                    (*(word *)info->arg) = WORD(ul & 0x7F, (ul >> 8) & 0x7F);;
                } else {
                    (*(unsigned long *)info->arg) = ul;
                }
            }
                break;
            case T_STRING_ARG:
            {
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
} // do_config()

void do_help(void)
{
    fputs(help_text, stdout);
    exit(0);
}

void do_ram(const char *v) {
    char *end_;
    unsigned char *end; // C standard says char might be signed,
                        // but that args to tolower() must be an int value.
                        // "epresentable as an unsigned char". So this is
                        // me trying to be pedantically safe.
    errno = 0;
    size_t amt = strtoul(v, &end_, 10);
    end = (unsigned char *)end_;

#define tl(c)  tolower(c)
    if (!(*end == '\0'
          || (tl(*end) == 'k'
              && (*(end+1) == '\0'
                  || (tl(*(end+1)) == 'b' && *(end+2) == '\0')
                  || (tl(*(end+1)) == 'i' && tl(*(end+2) == 'b')
                      && *(end+3) == '\0'))))) {
#undef tl
        // Something besides "k", "kb", or "kib" followed the number
        DIE(0, "Unrecognized characters \"%s\" following (possibly empty)\n",
            end_);
        DIE(2, " numeric portion, in argument to --ram.\n");
    } else if (errno == ERANGE || errno == EINVAL) {
        DIE(2,"Could not parse numeric arg to --ram.\n");
    }

    if (amt < 4
        || (amt <= 48 && (amt % 4) != 0)
        || (amt >48 && amt != 64 && amt != 128)) {

        DIE(2, "Unsupported value %zu for --ram.\n", amt);
    }

    if (amt == 28 || amt == 40 || amt == 44) {
        WARN("--ram value %zuk is not possible on a real Apple machine.\n",
             amt);
        WARN("  Proceeding anyway.\n");
    }

    cfg.amt_ram = amt * 1024;
}

void do_trace_to(const char *arg) {
    char *end;
    errno = 0;
    cfg.trace_end = strtoumax(arg, &end, 10);
    if (errno == ERANGE || errno == EINVAL || cfg.trace_end == UINTMAX_MAX) {
        DIE(2, "Couldn't parse numeric arg to --trace-to.\n");
    }
    if (*end == '\0') {
        cfg.trace_start = cfg.trace_end - 255;
    } else if (*end == '!') {
        arg = end + 1;
        errno = 0;
        unsigned long count = strtoul(arg, &end, 10);
        if (errno == ERANGE || errno == EINVAL || count == 0) {
            DIE(2, "Couldn't parse numeric arg to --trace-to (after !).\n");
        }
        if (*end != '\0') {
            DIE(2,"Garbage at end of arg to --trace-to.\n");
        }
        cfg.trace_start = cfg.trace_end - (count - 1);
    } else {
        DIE(2,"Garbage at end of arg to --trace-to.\n");
    }

    ++cfg.trace_end;
}
