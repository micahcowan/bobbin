#include "bobbin-internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "sha-256.h"

typedef const char *StrArray[];
typedef const char * const * ConstStrPtr;
typedef const char * const * * ConstStrPtrPtr;

static const char * const II_TAG = "original";
static const char * const PLUS_TAG = "plus";
static const char * const IIE_TAG = "twoey";
static const char * const ENHANCED_TAG = "enhanced";

typedef const byte                Sha256Sum[32];
typedef const byte              * Sha256SumPtr;
typedef const Sha256SumPtr        Sha256SumPtrAry[];
typedef const Sha256SumPtr      * Sha256SumPtrPtr; // const byte * const *

static const Sha256Sum II_SUM_A
  = { 0x68, 0xd9, 0xdb, 0x6b, 0xb4, 0xc3, 0x05, 0xd4,
      0x0c, 0x3f, 0xa8, 0x9f, 0xa0, 0xf2, 0xd7, 0xb7,
      0xf7, 0x15, 0x16, 0xa9, 0x43, 0x1e, 0x31, 0x38,
      0x85, 0x9f, 0x23, 0xbc, 0x5b, 0xdd, 0xb2, 0xd1 };
static const Sha256Sum PLUS_SUM_A
  = { 0x37, 0x8b, 0xa0, 0x0c, 0x86, 0xa6, 0x4c, 0xca,
      0x49, 0xce, 0xda, 0xca, 0x7d, 0xe8, 0xd5, 0xd3,
      0x51, 0x98, 0x3e, 0xbc, 0x29, 0x5d, 0x9d, 0x11,
      0xe0, 0x75, 0x2f, 0xeb, 0xfc, 0x34, 0x62, 0x49 }; 
static const Sha256Sum ZERO_SUM
  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; 

static const Sha256SumPtrAry II_SUMS = {
    II_SUM_A,
    NULL,
};

static const Sha256SumPtrAry PLUS_SUMS = {
    PLUS_SUM_A,
    NULL,
};

static const Sha256SumPtrAry PLACEHOLDER_SUMS = {
    ZERO_SUM,
    NULL,
};

static const StrArray II_ALIASES = {II_TAG,"][","II","two","woz","int","integer",NULL};
static const StrArray PLUS_ALIASES = {PLUS_TAG,"+","][+","II+",
                                 "twoplus","autostart",NULL};
static const StrArray IIE_ALIASES = {IIE_TAG,"][e","IIe",NULL};
static const StrArray ENHANCED_ALIASES = {ENHANCED_TAG,"//e",NULL};

typedef struct Alias Alias;
struct Alias {
    Sha256SumPtrPtr     sums;
    ConstStrPtr         strList;
};

static const Alias aliases[] = {
    { II_SUMS, II_ALIASES },
    { PLUS_SUMS, PLUS_ALIASES },
    { PLACEHOLDER_SUMS, IIE_ALIASES },
    { PLACEHOLDER_SUMS, ENHANCED_ALIASES },
};

static Sha256SumPtrPtr   acceptable_sums = NULL;

static const char *find_alias(const char *machine)
{
    const Alias *machineVisitor = &aliases[0];
    const Alias *lomEnd = machineVisitor
        + (sizeof aliases)/(sizeof aliases[0]);
    for (; machineVisitor != lomEnd; ++machineVisitor) {
        const char *firstAlias = *machineVisitor->strList;
        for (ConstStrPtr visit = machineVisitor->strList;
             *visit != NULL;
             ++visit) {

            if (STREQ(machine, *visit)) {
                acceptable_sums = machineVisitor->sums;
                return firstAlias;
            }
        }
    }

    return NULL;
}

void print_sum(const byte *sum, FILE *fp, int level)
{
    for (const byte *p = sum; p != sum + SIZE_OF_SHA_256_HASH; ++p) {
        SQUAWK_CONT(level, "%02x", (unsigned int)*p);
    }
}

#define MEMEQ(a, b, c)  (!memcmp(a,b,c))
bool validate_rom(unsigned char *buf, size_t sz)
{
    byte theHash[SIZE_OF_SHA_256_HASH];

    calc_sha_256(theHash, buf, sz);

    for (Sha256SumPtrPtr visit = acceptable_sums;
         *visit != NULL; ++visit)
    {
        if (MEMEQ(theHash, *visit, SIZE_OF_SHA_256_HASH)) {
            if (INFO_OK) {
                INFO("ROM file checksum is good:\n");
                INFO("  ");
                print_sum(theHash, stderr, INFO_LEVEL);
                INFO_CONT("\n");
            }

            return true;
        }
    }

    // No valid ROM found. Complain about it.
    WARN("WARNING!!! ROM file does not match an expected checksum.\n");
    if (INFO_OK) {
        INFO("ROM file checksum:\n");
        INFO("  ");
        print_sum(theHash, stderr, INFO_LEVEL);
        INFO_CONT("\n");

        INFO("Expected checksums for this machine:\n");
        for (Sha256SumPtrPtr visit = acceptable_sums;
             *visit != NULL; ++visit)
        {
            INFO("  ");
            print_sum(*visit, stderr, INFO_LEVEL);
            INFO_CONT("\n");
        }
    }

    return false;
}

const char *default_romfname;

void machine_init(void)
{
    const char *orig;
    if ((orig = find_alias(cfg.machine)) == NULL) {
        DIE(2, "Unrecognized machine name \"%s\".\n", cfg.machine);
    }
    // Can compare == (instead of STREQ), because we have
    //  the original's pointer.
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
    if (orig == II_TAG) {
        default_romfname = "apple2.rom";
    }
    if (orig == PLUS_TAG) {
        default_romfname = "apple2plus.rom";
    }
}
