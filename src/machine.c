#include "bobbin-internal.h"

typedef const char *StrArray[];
typedef const char * const * ConstStrPtr;
typedef const char * const * * ConstStrPtrPtr;

const char * const II_TAG = "original";
const char * const PLUS_TAG = "plus";
const char * const IIE_TAG = "twoey";
const char * const ENHANCED_TAG = "enhanced";

const StrArray II_ALIASES = {II_TAG,"][","II","two",NULL};
const StrArray PLUS_ALIASES = {PLUS_TAG,"+","][+","II+",
                                 "twoplus","autostart",NULL};
const StrArray IIE_ALIASES = {IIE_TAG,"][e","IIe",NULL};
const StrArray ENHANCED_ALIASES = {ENHANCED_TAG,"//e",NULL};

ConstStrPtr aliases[] = {
    II_ALIASES,
    PLUS_ALIASES,
    IIE_ALIASES,
    ENHANCED_ALIASES,
};

const char *find_alias(const char *machine)
{
    ConstStrPtrPtr machineVisitor = &aliases[0];
    ConstStrPtrPtr lomEnd = machineVisitor
        + (sizeof aliases)/(sizeof aliases[0]);
    for (; machineVisitor != lomEnd; ++machineVisitor) {
        const char *firstAlias = **machineVisitor;
        for (ConstStrPtr visit = *machineVisitor; *visit != NULL; ++visit) {
            if (STREQ(machine, *visit)) {
                return firstAlias;
            }
        }
    }

    return NULL;
}

void machine_init(void)
{
    const char *orig;
    if ((orig = find_alias(cfg.machine)) == NULL) {
        DIE(2, "Unrecognized machine name \"%s\".\n", cfg.machine);
    }
    // Can compare ==, because we have the original's pointer.
    if (orig == ENHANCED_TAG) {
        WARN("Default machine type is \"%s\", but that type is not actually\n",
             cfg.machine);
        WARN("supported in this development version.\n");
        DIE(2,"Try invoking with -m ][+ or -m plus\n");
    }
    if (orig == IIE_TAG) {
        DIE(2, "This development version does not yet "
               "support machine \"%s\".\n", cfg.machine);
    }
}
