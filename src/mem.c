//  mem.c
//
//  Copyright (c) 2023-2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

/* For the ROM file, and error handling */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/mman.h>
#include <unistd.h>

// Enough of a RAM buffer to provide 128k
//  Note: memory at 0xC000 thru 0xCFFF, and 0x1C000 thru 0x1CFFF,
//  are read at alternate banks of RAM for locations $D000 - $DFFF
static byte membuf[128 * 1024];

SoftSwitches ss;

// Pointer to firmware, mapped into the Apple starting at $D000
static unsigned char *rombuf;

static const char * const switch_names[] = {
    "LC_PREWRITE",
    "LC_NO_WRITE",
    "LC_BANK_ONE",
    "LC_READ_BSR",
    NULL,
    NULL,
    NULL,
    NULL,

    "RAMRD",
    "RAMWRT",
    "INTCXROM",
    "ALTZP",
    "INTC8ROM",
    "SLOTC3ROM",
    "EIGHTYSTORE",
    "VERTBLANK",
    "TEXT",
    "MIXED",
    "PAGE2",
    "HIRES",
    "ALTCHARSET",
    "EIGHTYCOL",
};
static const char *bad_switch_name = "<NOT A SWITCH>";

static const char * const rom_dirs[] = {
    "BOBBIN_ROMDIR", // not a dirname, an env var name
    "./roms",        // also not a dirname; we'll use bobbin's dir instead
    ROMSRCHDIR,
};
static const char * const *romdirp = rom_dirs;
static const char * const * const romdend = rom_dirs + (sizeof rom_dirs)/(sizeof rom_dirs[0]);

const byte *getram(void)
{
    return membuf;
}

void mem_put(const byte *buf, unsigned long start, size_t sz) {
    if (start + sz > (sizeof membuf)) {
        sz = (sizeof membuf) - start;
    }
    memcpy(&membuf[start], buf, sz);
}

void swset(SoftSwitches ss, SoftSwitchFlagPos pos, bool val)
{
    int bynum = pos / 8;
    int bitnum = pos % 8;
    assert(bynum < (sizeof (SoftSwitches))/(sizeof ss[0]));
    if (val) {
        ss[bynum] |= (1 << bitnum);
    } else {
        ss[bynum] &= ~(1 << bitnum);
    }
}

bool swget(SoftSwitches ss, SoftSwitchFlagPos pos)
{
    int bynum = pos / 8;
    int bitnum = pos % 8;
    assert(bynum < (sizeof (SoftSwitches))/(sizeof ss[0]));
    return (ss[bynum] >> bitnum) & 0x01;
}

static void swsetfire(SoftSwitches ss, SoftSwitchFlagPos pos, bool val)
{
    bool oldval = swget(ss, pos);
    swset(ss, pos, val);
    if (oldval != val) {
        event_fire_switch(pos);
    }
}

const char *get_switch_name(SoftSwitchFlagPos f)
{
    const char *ret;
    if (f >= (sizeof switch_names)/(sizeof switch_names[0])) {
        return bad_switch_name;
    }
    ret = switch_names[f];
    if (ret == NULL) {
        return bad_switch_name;
    }
    return ret;
}

const char *mem_get_acctype_name(MemAccessType m)
{
    switch (m) {
        case MA_MAIN:
            return "MAIN";
        case MA_SLOTS:
            return "SLOTS";
        case MA_ROM:
            return "ROM";
        case MA_LC_BANK1:
            return "BANK1";
        case MA_LC_BANK2:
            return "BANK2";
        case MA_LANG_CARD:
            return "LCARD";
        default:
            return "<NOTYPE>";
    }
}

static const char *get_try_rom_path(const char *fname) {
    static char buf[256];
    const char *env;
    const char *dir;

    if (romdirp == &rom_dirs[0] && (env = getenv(*romdirp++)) != NULL) {
        dir = env;
    } else if (romdirp == &rom_dirs[1]) {
        ++romdirp; // for next call to get_try_rom_path()

        // Check where bobbin's path from argv[0] is
        char *slash = strrchr(program_name, '/');
        if (slash == NULL) {
            dir = "."; // fallback to cwd
        } else {
            if ((slash - program_name) > ((sizeof buf)-1))
                goto realdir;
            dir = buf;
            size_t dirlen = slash - program_name;
            memcpy(buf, program_name, dirlen);
            (void) snprintf(&buf[dirlen], ((sizeof buf) - dirlen), "/roms");
        }
    } else {
realdir:
        if (romdirp != romdend) {
            dir = *romdirp++;
        }
        else {
            return NULL;
        }
    }
    VERBOSE("Looking for ROM named \"%s\" in %s...\n", fname, dir);
    if (dir != buf) {
        (void) snprintf(buf, sizeof buf, "%s/%s", dir, fname);
    } else {
        char *end = strchr(buf, 0);
        (void) snprintf(end, (sizeof buf) - (end - buf), "/%s", fname);
    }
    return buf;
}

byte *load_rom(const char *fname, size_t expected, bool exact)
{
    const char *rompath;
    const char *last_tried_path;
    int err = 0;
    byte *buf = NULL;
    size_t sz;

    if (exact) {
        VERBOSE("Loading user rom file \"%s\"\n", fname);
        last_tried_path = fname;;
        err = mmapfile(fname, &buf, &sz, O_RDONLY);
        if (buf == NULL) {
            DIE(1, "Couldn't open ROM file \"%s\": %s\n", fname,
                strerror(err));
        }
    } else {
        INFO("Searching for ROM file %s...\n", fname);
        romdirp = &rom_dirs[0]; // Reset search paths
        while (buf == NULL
                && (rompath = get_try_rom_path(fname)) != NULL) {
            last_tried_path = rompath;
            err = mmapfile(rompath, &buf, &sz, O_RDONLY);
            if (err != ENOENT) break;
        }
        if (buf == NULL) {
            DIE(1, "Couldn't open ROM file \"%s\": %s\n", last_tried_path, strerror(err));
        } else {
            INFO("FOUND ROM file \"%s\".\n", rompath);
        }
    }

    if (sz != expected) {
        // XXX expected size will need to be configurable
        DIE(0, "ROM file \"%s\" has unexpected file size %zu\n",
            last_tried_path, sz);
        DIE(1, "  (expected %zu).\n", expected);
    }

    return buf;
}

static void load_machine_rom(void)
{
    if (cfg.rom_load_file) {
        rombuf = load_rom(cfg.rom_load_file, expected_rom_size(), true);
    } else {
        rombuf = load_rom(default_romfname, expected_rom_size(), false);
        validate_rom(rombuf, expected_rom_size());
    }
}

static void fillmem(void)
{
    /* Immitate the on-boot memory pattern. */
    for (size_t z=0; z != sizeof membuf; ++z) {
        if (!(z & 0x2))
            membuf[z] = 0xFF;
    }

    /* Seed random generator */
    struct timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    srandom(tm.tv_nsec);

    /* For now at least, do what AppleWin does, and
     * set specific bytes to garbage. */
    for (size_t z=0; z < sizeof membuf; z += 0x200) {
        membuf[z + 0x28] = random();
        membuf[z + 0x29] = random();
        membuf[z + 0x68] = random();
        membuf[z + 0x69] = random();
    }
}

static inline void mem_init_langcard(void)
{
    ss[0] = 0;
}

void mem_init(void)
{
    fillmem();

    if (cfg.load_rom) {
        load_machine_rom();
    }

    mem_init_langcard();
}

void mem_reset(void)
{
    byte preserve = ss[0];
    memset(ss, 0, (sizeof ss)/(sizeof ss[0]));
    ss[0] = preserve;
    swset(ss, ss_text, true);
}

void mem_reboot(void)
{
    fillmem();

    mem_init_langcard();

    mem_reset();
}

static bool is_aux_mem(word loc, bool wr)
{
    bool aux = false;

    // Why the !wr check below? Reading AUX memory when we don't have it
    // should result in reading MAIN memory, _but writing_ AUX memory
    // should go to the bit bucket. We need to indicate that this write
    // would in fact go to aux, so it can be properly thrown away.
    if (!wr && cfg.amt_ram <= LOC_AUX_START) {
        // Auxilliary memory has been disabled.
        return false;
    }
    aux = (swget(ss, ss_altzp)
           && (loc < LOC_STACK_END || loc > SS_START));
    if (swget(ss, ss_eightystore)
        && ((loc >= LOC_TEXT1 && loc < LOC_TEXT2)
            || (swget(ss, ss_hires)
                && (loc >= LOC_HIRES1 && loc < LOC_HIRES2)))) {
        aux = aux || swget(ss, ss_page2);
    } else {
        aux = aux || (loc >= LOC_STACK_END && loc < SS_START
                      && ((!wr && swget(ss, ss_ramrd))
                          || (wr && swget(ss, ss_ramwrt))));
    }

    return aux;
}

void mem_get_true_access(word loc, bool wr, size_t *bufloc, bool *in_aux, MemAccessType *access)
{
    if (loc < SS_START) {
        *access = MA_MAIN;
        *bufloc = loc;
    } else if (loc < LOC_ROM_START) {
        *access = MA_SLOTS;
        *bufloc = loc;
    } else if (rombuf && (!cfg.lang_card
                   || (wr? swget(ss, ss_lc_no_write)
                         : !swget(ss, ss_lc_read_bsr)))) {
        *access = MA_ROM;
        size_t romsz = expected_rom_size();
        *bufloc = loc - (LOC_ADDRESSABLE_END - romsz);
    } else if (cfg.lang_card && swget(ss, ss_lc_bank_one)
               && loc < LOC_BSR_END) {
        *access = MA_LC_BANK1;
        *bufloc = loc - (LOC_BSR_START - LOC_BSR1_START);
    } else if (loc < LOC_BSR_END) {
        *access = MA_LC_BANK2;
        *bufloc = loc;
    } else {
        *access = MA_LANG_CARD;
        *bufloc = loc;
    }

    // If not ROM and not SLOTS, check for aux
    if (*access == MA_ROM || *access == MA_SLOTS) {
        *in_aux = false;
    } else {
        *in_aux = is_aux_mem(loc, wr);
        if (*in_aux) *bufloc |= LOC_AUX_START;
    }
}

static int maybe_language_card(word loc, bool wr)
{
    if ((loc & 0xFFF0) != SS_LANG_CARD) return -1;
    if (wr)
        swsetfire(ss, ss_lc_prewrite, false);

    if ( ! (loc & 0x0001)) {
        swsetfire(ss, ss_lc_prewrite, false);
        swsetfire(ss, ss_lc_no_write, true);
    } else {
        if (swget(ss, ss_lc_prewrite)) {
            swsetfire(ss, ss_lc_no_write, false);
        }
        if (!wr) {
            swsetfire(ss, ss_lc_prewrite, true);
        }
    }

    swsetfire(ss, ss_lc_bank_one, (loc & 0x0008) != 0);
    swsetfire(ss, ss_lc_read_bsr, ((loc & 0x0002) >> 1) == (loc & 0x0001));

    return 0; // s/b floating bus
}

int slot_access_switches(word loc, int val)
{
    int ret = -1;
    bool wr = (val != -1);
    SoftSwitchFlagPos f = ss_no_switch;
    bool fval;

    //RestartSw oldsw = rstsw;
    if (loc >= 0xC300 && loc < 0xC400 && !swget(ss, ss_slotc3rom)
            && machine_is_iie()) {
        f = ss_intc8rom;
        fval = true;
    } else if (loc == 0xCFFF && machine_is_iie()) {
        f = ss_intc8rom;
        fval = false;
    } else if ((loc & 0xFFF0) == 0xC050) {
        fval = loc & 1;
        switch (loc & 0x000F) {
            case 0:
            case 1:
                f = ss_text;
                break;
            case 2:
            case 3:
                f = ss_mixed;
                break;
            case 4:
            case 5:
                f = ss_page2;
                break;
            case 6:
            case 7:
                f = ss_hires;
                break;
            // annunciators not yet handled
        }
    } else if (loc >= 0xC090 && loc < 0xC100) {
        if (wr) {
            periph_sw_poke(loc, val);
        } else {
            ret = periph_sw_peek(loc);
        }
    } else if (!wr || ((loc & 0xFFF0) != 0xC000) || !machine_is_iie()) {
        // skip the rest, they need writes
    } else {
        fval = loc & 1;
        switch(loc & 0x000F) {
            case 0:
            case 1:
                f = ss_eightystore;
                break;
            case 2:
            case 3:
                f = ss_ramrd;
                break;
            case 4:
            case 5:
                f = ss_ramwrt;
                break;
            case 6:
            case 7:
                f = ss_intcxrom;
                break;
            case 8:
            case 9:
                f = ss_altzp;
                break;
            case 0xA:
            case 0xB:
                f = ss_slotc3rom;
                break;
            case 0xC:
            case 0xD:
                f = ss_eightycol;
                break;
            case 0xE:
            case 0xF:
                f = ss_altcharset;
                break;
        }
        // prsw();
    }
    if (f != ss_no_switch) {
        swsetfire(ss, f, fval);
    }
    return ret;
}

static byte *slot_area_access_sneaky(word loc, bool wr)
{
    if (wr || loc < LOC_SLOTS_START || loc >= LOC_SLOTS_END
        || !machine_is_iie()) return NULL;

    if ((!swget(ss, ss_slotc3rom) && loc >= 0xC300 && loc < 0xC400)
        || (swget(ss, ss_intc8rom) && loc >= 0xC800)
        || swget(ss, ss_intcxrom)) {

        size_t romsz = expected_rom_size();
        assert(loc >= (LOC_ADDRESSABLE_END - romsz));
        return &rombuf[loc - (LOC_ADDRESSABLE_END - romsz)];
    } else {
        return NULL;
    }
}

static int switch_reads(word loc)
{
    if ((loc & 0xFFF0) != 0xC010) return -1;
    int val = -1;
    bool b;

    switch (loc & 0x000F) {
        case 0:
            // Any key down. Interface must handle.
            return -1;
        case 1:
            b = !swget(ss, ss_lc_bank_one);
            break;
        case 2:
            b = swget(ss, ss_lc_read_bsr);
            break;
        case 3:
            b = swget(ss, ss_ramrd); // s/b auxrd
            break;
        case 4:
            b = swget(ss, ss_ramwrt);// s/b auxwrt
            break;
        case 5:
            b = swget(ss, ss_intcxrom);
            break;
        case 6:
            b = swget(ss, ss_altzp);
            break;
        case 7:
            b = swget(ss, ss_slotc3rom);
            break;
        case 8:
            b = swget(ss, ss_eightystore);
            break;
        case 9:
            b = swget(ss, ss_vertblank);
            break;
        case 0xA:
            b = swget(ss, ss_text);
            break;
        case 0xB:
            b = swget(ss, ss_mixed);
            break;
        case 0xC:
            b = swget(ss, ss_page2);
            break;
        case 0xD:
            b = swget(ss, ss_hires);
            break;
        case 0xE:
            b = swget(ss, ss_altcharset);
            break;
        case 0xF:
            b = swget(ss, ss_eightycol);
            break;
    }

    // XXX rest of bits ought to be floating-bus
    return (!!b) << 7;
}

byte peek(word loc)
{
    int t = event_fire_peek(loc);
    if (t < 0) maybe_language_card(loc, false);
    if (t < 0) t = slot_access_switches(loc, -1);
    if (t >= 0) {
        return (byte) t;
    }

    return peek_sneaky(loc);
}

byte peek_sneaky(word loc)
{
    int t = -1; // PEEK_SNEAKY event handler?
    if (t >= 0) {
        return (byte) t;
    }

    byte *mem;
    int val;
    if ((mem = slot_area_access_sneaky(loc, false)) != NULL) {
        return *mem;
    }
    if ((val = switch_reads(loc)) != -1) {
        return val;
    }
    if (loc >= LOC_SLOTS_START && loc < LOC_SLOT_EXPANDED_AREA) {
        return periph_rom_peek(loc);
    }
    if (loc >= cfg.amt_ram && loc < SS_START) {
        return 0; // s/b floating bus? not sure, in sneaky
    }

    size_t bufloc;
    bool aux;
    MemAccessType acc;
    mem_get_true_access(loc, false, &bufloc, &aux, &acc);

    if (acc == MA_ROM) {
        return rombuf[bufloc];
    }
    else {
        return membuf[bufloc];
    }
}

void poke(word loc, byte val)
{
    if (event_fire_poke(loc, val))
        return;
    if (maybe_language_card(loc, true) >= 0)
        return;
    if (loc >= 0xC000 && loc < 0xC100) {
        (void) slot_access_switches(loc, val);
        return;
    }
    poke_sneaky(loc, val);
}

void poke_sneaky(word loc, byte val)
{
    // XXX should handle slot-area writes

    size_t bufloc;
    bool aux;
    MemAccessType acc;
    mem_get_true_access(loc, true, &bufloc, &aux, &acc);

    if (acc != MA_ROM && acc != MA_SLOTS
        && (!aux || cfg.amt_ram > LOC_AUX_START)) {

        membuf[bufloc] = val;
    }
}

bool mem_match(word loc, unsigned int nargs, ...)
{
    bool status = true;
    va_list args;
    va_start(args, nargs);

    for (; nargs != 0; ++loc, --nargs) {
        if (peek_sneaky(loc) != va_arg(args, int)) {
            status = false;
        }
    }

    va_end(args);
    return status;
}
