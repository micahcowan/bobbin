//  machine.c
//
//  Copyright (c) 2023-2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "sha-256.h"

typedef const char *StrArray[];
typedef const char * const * ConstStrPtr;
typedef const char * const * * ConstStrPtrPtr;

typedef const byte                Sha256Sum[32];
typedef const byte              * Sha256SumPtr;
typedef const Sha256SumPtr        Sha256SumPtrAry[];
typedef const Sha256SumPtr      * Sha256SumPtrPtr; // const byte * const *

static const Sha256Sum ORIGINAL_SUM_A
  = { 0x68, 0xd9, 0xdb, 0x6b, 0xb4, 0xc3, 0x05, 0xd4,
      0x0c, 0x3f, 0xa8, 0x9f, 0xa0, 0xf2, 0xd7, 0xb7,
      0xf7, 0x15, 0x16, 0xa9, 0x43, 0x1e, 0x31, 0x38,
      0x85, 0x9f, 0x23, 0xbc, 0x5b, 0xdd, 0xb2, 0xd1 };
static const Sha256Sum PLUS_SUM_A
  = { 0x37, 0x8b, 0xa0, 0x0c, 0x86, 0xa6, 0x4c, 0xca,
      0x49, 0xce, 0xda, 0xca, 0x7d, 0xe8, 0xd5, 0xd3,
      0x51, 0x98, 0x3e, 0xbc, 0x29, 0x5d, 0x9d, 0x11,
      0xe0, 0x75, 0x2f, 0xeb, 0xfc, 0x34, 0x62, 0x49 }; 
static const Sha256Sum PLUS_SUM_B
 = {  0xfc, 0x3e, 0x9d, 0x41, 0xe9, 0x42, 0x85, 0x34,
      0xa8, 0x83, 0xdf, 0x5a, 0xa1, 0x0e, 0xb5, 0x5b,
      0x73, 0xea, 0x53, 0xd2, 0xfc, 0xbb, 0x3e, 0xe4,
      0xf3, 0x9b, 0xed, 0x1b, 0x07, 0xa8, 0x29, 0x05 };
static const Sha256Sum TWOEY_SUM_A
 = {  0x1f, 0xb8, 0x12, 0x58, 0x4c, 0x66, 0x33, 0xfa,
      0x16, 0xb7, 0x7b, 0x20, 0x91, 0x59, 0x86, 0xed,
      0x11, 0x78, 0xd1, 0xe6, 0xfc, 0x07, 0xa6, 0x47,
      0xf7, 0xee, 0x8d, 0x4e, 0x6a, 0xb9, 0xd4, 0x0b };
static const Sha256Sum ENHANCED_SUM_A
 = {  0xaa, 0xb3, 0x8a, 0x03, 0xca, 0x8d, 0xea, 0xbb,
      0xb2, 0xf8, 0x68, 0x73, 0x31, 0x48, 0xc2, 0xef,
      0xd6, 0xf6, 0x55, 0xa5, 0x9c, 0xd9, 0xc5, 0xd0,
      0x58, 0xef, 0x3e, 0x0b, 0x7a, 0xa8, 0x6a, 0x1a };
static const Sha256Sum ZERO_SUM
  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; 

static const Sha256SumPtrAry ORIGINAL_SUMS = {
    ORIGINAL_SUM_A,
    NULL,
};

static const Sha256SumPtrAry PLUS_SUMS = {
    PLUS_SUM_A, // Perhaps an older version sent to dealers?
    PLUS_SUM_B, // this is the preferred one, fixed BRK vector
    NULL,
};

static const Sha256SumPtrAry TWOEY_SUMS = {
    TWOEY_SUM_A,
    NULL,
};

static const Sha256SumPtrAry ENHANCED_SUMS = {
    ENHANCED_SUM_A,
    NULL,
};

static const Sha256SumPtrAry PLACEHOLDER_SUMS = {
    ZERO_SUM,
    NULL,
};

#include "machine-names.h"

typedef struct Alias Alias;
struct Alias {
    Sha256SumPtrPtr     sums;
    ConstStrPtr         strList;
};

static const Alias aliases[] = {
    { ORIGINAL_SUMS, ORIGINAL_ALIASES },
    { PLUS_SUMS, PLUS_ALIASES },
    { TWOEY_SUMS, TWOEY_ALIASES },
    { ENHANCED_SUMS, ENHANCED_ALIASES },
};

static Sha256SumPtrPtr   acceptable_sums = NULL;

static size_t expected_size;

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

            if (STREQCASE(machine, *visit)) {
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

    if (cfg.tokenize || cfg.detokenize) {
        WARN("--%stokenize may not work properly with unsupported ROMs.\n",
             cfg.detokenize? "de" : "");
        WARN("Continue at your peril!\n");
    }

    return false;
}

const char *default_romfname;
static bool is_iie = false;
static bool is_enhanced_iie = false;

void machine_init(void)
{
    const char *orig;
    if (cfg.tokenize) {
        // Unconditionally force machine type to ][+
        orig = PLUS_TAG;
        if (cfg.machine_set) {
            const char *tag = find_alias(cfg.machine);
            if (tag != orig) {
                WARN("WARNING: You specified \"-m %s\", but --tokenize"
                     " forced to %s.\n", cfg.machine, orig);
            }
        }
        orig = find_alias(orig);     // We have to run this
                                     // to set up `acceptable_sums`
    } else if (cfg.detokenize) {
        // Unconditionally force machine type to ][+
        orig = TWOEY_TAG;
        if (cfg.machine_set) {
            const char *tag = find_alias(cfg.machine);
            if (tag != orig) {
                WARN("WARNING: You specified \"-m %s\", but --detokenize"
                     " forced to %s.\n", cfg.machine, orig);
            }
        }
        orig = find_alias(orig);     // We have to run this
                                     // to set up `acceptable_sums`
    } else if (!cfg.machine_set && cfg.runbasicfile) {
        // If --run-basic was specified we default to enhanced Apple IIe
        // because a #! line may not be able to accept multiple arguments (so, no way to specify -m)
        // Enhanced Apple IIe supports lowercase and is more compatible
        orig = find_alias(ENHANCED_TAG);
    } else if ((orig = find_alias(cfg.machine)) == NULL) {
        DIE(2, "Unrecognized machine name \"%s\".\n", cfg.machine);
    }
    // Can compare == (instead of STREQ), because we have
    //  the original's pointer.
    if (orig == TWOEY_TAG) {
        default_romfname = "apple2e.rom";
        expected_size = 16 * 1024;
        is_iie = true;
    }
    if (orig == ENHANCED_TAG) {
        default_romfname = "apple2e_enh.rom";
        expected_size = 16 * 1024;
        is_iie = true;
        is_enhanced_iie = true;
    }
    if (orig == ORIGINAL_TAG) {
        default_romfname = "apple2.rom";
        expected_size = 12 * 1024;
    }
    if (orig == PLUS_TAG) {
        default_romfname = "apple2plus.rom";
        expected_size = 12 * 1024;
    }
}

size_t expected_rom_size(void)
{
    return expected_size;
}

bool machine_is_iie(void)
{
    return is_iie;
}

bool machine_is_enhanced_iie(void)
{
    return is_enhanced_iie;
}

bool machine_has_mousetext(void)
{
    return is_enhanced_iie;
}
