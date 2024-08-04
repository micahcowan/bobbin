//  delay-pc.c
//
//  Copyright (c) 2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>

#define INVALID_LOC (-1)

struct dlypc_record {
    struct dlypc_record *   next;
    int                     delay_pc;
    const char *            load_fname;
    unsigned long           load_loc;
    int                     jump_loc;
    bool                    basic_fixup;
};

// Template record for copying to new records.
struct dlypc_record initrec = {
    .next = NULL,
    .delay_pc = INVALID_LOC,
    .load_fname = NULL,
    .load_loc = 0,
    .jump_loc = INVALID_LOC,
    .basic_fixup = false,
};

struct dlypc_record *head = NULL;
struct dlypc_record *tail = NULL;
struct dlypc_record *cur  = NULL;

/********************/

static bool check_asoft_link(const byte *buf, size_t start, size_t sz,
                             word w, long *chlenp)
{
    long chlen;
    bool val = true;
    for (chlen = 0; chlen < 65536; ++chlen) {
        if (w == 0x0000) {
            val = true;
            break;
        } else if (w < start || w > start + sz - sizeof(word)) {
            val = false;
            break;
        } else {
            // next link
            w = WORD(buf[w - start], buf[w - start + 1]);
        }
    }
    if (chlen == 65536)
        chlen = -1;
    if (val)
        *chlenp = chlen;
    return val;
}

static void adjust_asoft_start(const char *fname, const byte **bufp,
                               size_t *szp, word load_loc) {
    /*
       If we're loading an AppleSoft BASIC file, there is some
       possibility that the first two bytes are not actually part of
       the AppleSoft program to load, but are a word representing
       the size of the file, minus one (= size of program, plus one?)
       Apparently this is how Apple DOS stores the file, and there
       are tokenizer programs that output this way.

       In every case where we determine tha word 0 is a file length and
       not the first BASIC link, we complain with a warning to avoid
       tokenizers that prefix a file length header word.

       If word zero is not the file length, word zero is the first BASIC
       link. But check it anyway, and error if it's invalid.

       Otheriwse, if word zero is not a valid BASIC link, then we assume
       it's a file length and skip it.

       Otherwise, if word zero is a valid BASIC link, and word one is not,
       we assume word zero is the first BASIC link (and word one is
       the first line number).

       If both word zero and word one are valid BASIC links, we
       use word zero, UNLESS it appears to have a loop in the chain
       while word one doesn't, in which case we use word one.

       The notion of a "valid BASIC link" is determined as:
         (a) it refers to a memory location that resides within the
             loaded file's range, or else has the value zero.
         (b) if it refers to a memory location, the value at that
             location must itself a valid BASIC link (this definition
             is recursive)
       Chain linkage is only followed up to 65,535 links. If linkage
       exceeds this number (may be a looping chain), it is still
       considered a "valid BASIC link", but the chain length is reported
       as -1, for the purposes of comparing chain lengths (see above, in
       the case where both words zero and one are "valid BASIC links").

       Note that things like backwards-links, or links that jump forward
       by more than 256 bytes, or line numbers whose values jump around,
       are not checked (even though it is not normally possible to
       create such programs), because people do pull shenanigans with
       the BASIC program representation, and we want to be as permissive
       as possible.

       It is possible, though unlikely, for a value to be both a valid
       file length, *and* a valid BASIC link, if the file length is
       greater than the --load-at location (or default $801), and the
       value of that file length as a memory pointer just so happens to
       fall at a valid link.

       It is similarly possible, though unlikely, for a value to be both
       a valid line number, and a valid BASIC link.
    */
    if (*szp < sizeof (word)) {
        DIE(0, "--load-basic-bin: file \"%s\" has insufficient length (%zu)\n",
            fname, *szp);
        DIE(2, "  to be a valid AppleSoft BASIC program binary.\n");
    }

    INFO("Determining whether AppleSoft program in file \"%s\" begins\n",
         fname);
    INFO("  at the first word, or the second.\n");

    bool adjusted = false;
    const word lenval = (*szp)-1;
    word w0 = WORD((*bufp)[0], (*bufp)[1]);
    long w0chain;
    bool w0valid = check_asoft_link(*bufp, load_loc, *szp,
                                    w0, &w0chain);
#define VPFX "    ..."
    if (w0valid) {
        VERBOSE(VPFX "word 0 is a valid BASIC chain of length %ld.\n",
                w0chain);
    } else {
        VERBOSE(VPFX "word 0 is not a valid BASIC chain.\n");
    }

    bool is_lenval = (w0 == lenval);
    VERBOSE(VPFX "word 0 is%s a valid file length.\n", is_lenval? "" : " not");
    if (is_lenval) {
        // Nothing to do yet, futher checking is required.
    } else if (!w0valid) {
        // Not the length value, but also not a valid BASIC link. Error.
        DIE(0, "First word of --load-basic-bin file \"%s\" is neither a\n",
            fname);
        DIE(0, "  valid filesize value, nor a valid AppleSoft link!\n");
        DIE(2, "  Not a valud AppleSoft BASIC program binary.\n");
    } else {
        // is not the length value, but is a BASIC link. We have enough info.
        goto no_adjust;
    }

    // From here on out, we know word zero was a valid file length!

    long w1chain;
    bool w1valid =
        (*szp > 2 * sizeof (word)) // don't read word 1 if it doesn't exist
        && check_asoft_link(*bufp + sizeof (word), load_loc,
                            *szp - sizeof (word),
                            WORD((*bufp)[2], (*bufp)[3]), // w1
                            &w1chain);
    if (w1valid) {
        VERBOSE(VPFX "word 1 is a valid BASIC chain of length %ld.\n",
                w1chain);
    } else {
        VERBOSE(VPFX "word 1 is not a valid BASIC chain.\n");
    }

    if (w0valid) {
        // handled further below
    } else if (!w1valid) {
        DIE(0, "Neither word 0 nor word 1 are valid AppleSoft BASIC links.\n");
        DIE(0, "--load-basic-bin file \"%s\" is not an AppleSoft BASIC\n",
            fname);
        DIE(2, "  program binary.\n");
    } else {
        goto adjust; // word 0 is the length value, is not a valid link.
    }

    // Okay, both word 0 and word 1 are vaild BASIC links...
    WARN("AMBIGUOUS BASIC FILE! Word 0 and word 1 of --load-basic-bin\n");
    WARN("  file \"%s\" are BOTH valid program starts, but word 0\n",
         fname);
    WARN("  could ALSO be a valid file length header.\n");
    WARN("If the program listing appears to be wrong, please use a\n");
    WARN("  different AppleSoft tokenizer program\n");
    WARN("  (such as `bobbin --tokenize`), and start with a line\n");
    WARN("  number below 2000.\n");
    if (w0chain == -1 && w1chain != -1) {
        WARN("Using word 1, as word 0 appears to be an infinite chain.\n");
        goto adjust;
    }
    goto no_adjust;

#undef VPFX
adjust:
    WARN("file-length prefix detected for AppleSoft binary\n");
    WARN("  file \"%s\". Please use a tokenizer\n",
         fname);
    WARN("  that doesn't write this prefix (such as bobbin --tokenize.\n");
    adjusted = true;
    *bufp += 2;
    *szp -= 2;
    // fall through
no_adjust:
    INFO("Determination: AppleSoft program begins at word %s.\n",
         adjusted? "one" : "zero");
}

static void load_file_into_mem(const char *fname, word load_loc,
                               bool basic_fixup) {
    int err;
    byte *allocbuf;
    const byte *usebuf;
    size_t allocsz, sz;

    err = mmapfile(fname, &allocbuf, &sz, O_RDONLY);
    allocsz = sz;
    if (allocbuf == NULL) {
        DIE(1, "Couldn't mmap --load file \"%s\": %s\n",
            fname, strerror(err));
    }

    usebuf = allocbuf;
    if (basic_fixup) {
        adjust_asoft_start(fname, &usebuf, &sz, load_loc);
    }

    if ((sz + load_loc) > (128 * 1024)) {
        WARN("--load file \"%s\" ($%lX + $%04lX) will exceed the end of\n",
             fname, (unsigned long)load_loc, (unsigned long)sz);
        WARN("emulated memory ($20000)! Truncating to fit.\n");
        sz = 0x20000 - load_loc;
    }

    mem_put(usebuf, load_loc, sz);

    INFO("%zu bytes loaded into RAM from file \"%s\",\n",
         sz, fname);
    INFO("  starting at memory location $%04X.\n",
         (unsigned int)load_loc);
    // XXX Warning, this ^ is a lie for some values ($C000 - $CFFF), and
    //  aux mem locations

    if (basic_fixup) {
        // This was an AppleSoft BASIC file. Fixup some
        // zero-page values.
        poke_sneaky(ZP_TXTTAB, LO(load_loc)   /* s/b $01 */);
        poke_sneaky(ZP_TXTTAB+1, HI(load_loc) /* s/b $08 */);
        byte lo = LO(load_loc + sz);
        byte hi = HI(load_loc + sz);
        poke_sneaky(ZP_VARTAB, lo);
        poke_sneaky(ZP_VARTAB+1, hi);
        poke_sneaky(ZP_PRGEND, lo);
        poke_sneaky(ZP_PRGEND+1, hi);
        poke_sneaky(ZP_ARYTAB, lo);
        poke_sneaky(ZP_ARYTAB+1, hi);
        poke_sneaky(ZP_STREND, lo);
        poke_sneaky(ZP_STREND+1, hi);
        INFO("--load-basic-bin: AppleSoft settings adjusted.\n");
        VERBOSE("BASIC program start = $%X, end = $%X.\n",
                (unsigned int)(load_loc),
                (unsigned int)WORD(lo, hi));
    }

    munmap(allocbuf, allocsz);
}

static void process_record(struct dlypc_record *rec) {
    if (rec->load_fname != NULL)
        load_file_into_mem(rec->load_fname, rec->load_loc, rec->basic_fixup);
    if (rec->jump_loc != INVALID_LOC) {
        INFO("Jumping PC to $%04X (--jump-to).\n",
             (unsigned int)rec->jump_loc);
        PC = rec->jump_loc;
    }
    else {
        VERBOSE("    (PC is $%04X)\n", (unsigned int)PC);
    }
}

static void delay_step(Event *e)
{
    if (e->type == EV_PRESTEP && cur != NULL &&
        (cur->delay_pc == INVALID_LOC || PC == cur->delay_pc)) {

        if (cur->delay_pc != INVALID_LOC) {
            INFO("PC reached trigger at %04X (--delay-until-pc).\n",
                 (unsigned int)cur->delay_pc);
        }
        process_record(cur);
        cur = cur->next;
    }
}

static void alloc_rec_at_tail(void)
{
    struct dlypc_record *newrec = xalloc(sizeof *newrec);
    *newrec = initrec;

    if (tail == NULL) {
        head = tail = newrec;
    }
    else {
        tail->next = newrec;
        tail = newrec;
    }
}

static void ensure_tail_exists(void)
{
    if (tail == NULL)
        alloc_rec_at_tail();
}

/********** External-Linkage Functions **********/

void dlypc_init(void) {
    event_reghandler(delay_step);
}

void dlypc_delay_until(word loc) {
    // --delay-pc-until always starts a new "moment"/record
    alloc_rec_at_tail();
    tail->delay_pc = loc;
}

void dlypc_load(const char *fname) {
    ensure_tail_exists();
    if (tail->load_fname != NULL) {
        alloc_rec_at_tail();
    }
    size_t len = strlen(fname) + 1;
    char *newname = xalloc(len);
    memcpy(newname, fname, len);
    tail->load_fname = newname;
}

void dlypc_load_basic(const char *fname) {
    alloc_rec_at_tail();
    if (head == tail) {
        // Specified first, so add a --delay-until-pc INPUT
        INFO("--load-basic-bin is the first specified action:.\n");
        INFO("Adding --delay-until-pc INPUT before --load-basic-bin.\n");
        tail->delay_pc = MON_KEYIN;
    }
    else {
        INFO("--load-basic-bin was not the first specified action:.\n");
        INFO("NOT prepending --delay-until-pc INPUT.\n");
    }
    dlypc_load(fname);
    tail->load_loc = 0x801; // Where BASIC programs reside
    tail->basic_fixup = true;
}

void dlypc_load_at(word loc) {
    ensure_tail_exists();
    tail->load_loc = loc;
}

void dlypc_jump_to(word loc) {
    ensure_tail_exists();
    tail->jump_loc = loc;
}

void dlypc_reboot(void) {
    cur = head;
}

// iterator abstraction for traversing the files to be loaded
struct dlypc_file_iter {
    struct dlypc_record *r;
};

struct dlypc_file_iter *dlypc_file_iter_new(void) {
    struct dlypc_file_iter *iter = xalloc(sizeof *iter);
    iter->r = head;
    return iter;
}

const char *dlypc_file_iter_getnext(struct dlypc_file_iter *iter) {
    // Does the current record have a file name? Return it.
    // If not, find next one that does, return it.
    for (;;) { // ever
        if (iter->r == NULL) {
            // List exhausted: no more filenames remain.
            return NULL;
        }
        if (iter->r->load_fname != NULL) {
            const char *fname = iter->r->load_fname;
            iter->r = iter->r->next;
            return fname;
        }
        iter->r = iter->r->next; // No filename, try next?
    }
}

void dlypc_file_iter_destroy(struct dlypc_file_iter *iter) {
    free(iter);
}

