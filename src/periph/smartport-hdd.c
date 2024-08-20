//  periph/smartport-hdd.c
//
//  Copyright (c) 2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

#include "bobbin-internal.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>

#define BadCmd  0x01
#define BadPCnt 0x04
#define BusErr  0x06
#define BadCtl  0x21
#define IOError 0x27
#define DevDiscon 0x28
#define BadBlock 0x2D

#define RETURN_ERROR(err)    do { \
        ACC = err; \
        PPUT(PCARRY, true); \
    } while(0)

struct SPDev {
    const char *fname;
    FILE       *fh;
    off_t       sz;
    byte        bsz[3];
};

const static unsigned int MAX_NDEV = 4;
unsigned int ndev = 0;
static struct SPDev devices[4];

// Bytes to identify this slot as a SmartPort card.
static const byte id_bytes[] = { 0xFF, 0x20, 0xFF, 0x00,
                                 0xFF, 0x03, 0xFF, 0x00 };

// Entry points
static const byte smartport_ep  = 0x20;
static const byte prodos_ep     = smartport_ep - 3;


static byte get_status_byte(struct SPDev *d)
{
    /*
      bit
        7 0 = character device, 1 = block device
        6 1 = write allowed
        5 1 = read allowed
        4 1 = device on line or disk in drive
        3 0 = format allowed
        2 0 = medium write protected (block devices only)
        1 1 = device currently interrupting
        0 1 = device currently open (character devices only)
     */
    // XXX For now, always return 0xFC
    return 0xFC;
}

static void handle_status_device(byte unit, word stlist)
{
    DEBUG("SP status device, unit=%d\n", (int)unit);
    if (unit == 0) {
        YREG = 0;
        XREG = 8; // Writing "8 bytes" (but really only two, rest are
                  //  "reserved").
        PPUT(PCARRY, false); // Call succeeded

        poke(stlist, ndev);
        poke(stlist+1, 0xFF); // Don't handle interrupts
    }
    else if (unit <= ndev) {
        YREG = 0;
        XREG = 4; // Writing four bytes
        PPUT(PCARRY, false); // Call succeeded

        struct SPDev *d = &devices[unit-1];
        poke(stlist, get_status_byte(d));
        for (int i=0; i!=3; ++i) {
            poke(stlist+1+i, d->bsz[i]);
        }
    }
    else {
        RETURN_ERROR(DevDiscon);
    }
}

static void put_id_string(unsigned int *x, word basep, const char *fname)
{
    basep += *x; // Adjust for current position

    const char *name = strrchr(fname, '/');
    if (!name) name = fname;

    int len = 0;
    // NOTE: assumes ASCII-ish names. Otherwise will be (harmless) garbage.
    for (int i=0; i!=16; ++i) {
        if (name[len] != '\0') {
            char c = name[len] & 0x7F;
            if (c < 0x20) {
                c = '?'; // control character -> ?
            } else if (c > 0x5F) {
                c &= 0xDF; // capitalize (also changes some symbols, & DEL)
            }
            poke(basep+1+i, c);
            ++len;
        }
        else {
            poke(basep+1+i, 0x20); // SPACE
        }
    }
    poke(basep, len);

    *x += 17; // char-count + 16 bytes
}

static void handle_status_dib(byte unit, word stlist)
{
    DEBUG("SP status DIB, unit=%d\n", (int)unit);
    if (unit == 0 || unit > ndev) {
        RETURN_ERROR(DevDiscon);
    }

    struct SPDev *d = &devices[unit-1];
    unsigned int x = 0;

    poke(stlist+x++, get_status_byte(d));
    for (int i=0; i!=3; ++i) {
        poke(stlist+x++, d->bsz[i]);
    }
    put_id_string(&x, stlist, d->fname);

    poke(stlist+x++, 0x07); // Device type code (= SCSI HDD)
    poke(stlist+x++, 0x00); // Device subtype code
    poke(stlist+x++, 0x00); // Device fw version (lo)
    poke(stlist+x++, 0x00); // Device fw version (hi)

    XREG = x;
    YREG = 0;
    PPUT(PCARRY, false); // Successful call
}

static void handle_status(word params)
{
    byte pcount = peek(params);
    params += 1;
    byte unit = peek(params);
    params += 1;
    byte stllo = peek(params);
    params += 1;
    byte stlhi = peek(params);
    params += 1;
    word stlist = WORD(stllo, stlhi);
    byte code = peek(params);
    
    if (pcount != 3) {
        RETURN_ERROR(BadPCnt);
    }

    switch (code) {
        case 0x00:
            handle_status_device(unit, stlist);
            break;
        case 0x03:
            handle_status_dib(unit, stlist);
            break;
        default:
            DEBUG("Unsupported SP STATUS call %02X.\n", (unsigned int)code);
            RETURN_ERROR(BadCtl);
    }
}

static void read_block(byte unit, off_t blkpos, word buffer)
{
    if (unit == 0 || unit > ndev) {
        WARN("Bad read_black unit number %d\n", (int)unit);
        RETURN_ERROR(DevDiscon);
    }

    struct SPDev *d = &devices[unit-1];
    off_t sekpos = blkpos * 512;
    DEBUG("read_block, unit=%d, blk=%zu, buf=%04lX\n", (int)unit,
          (size_t)blkpos, (unsigned long)buffer);
    errno = 0;
    if (fseeko(d->fh, sekpos, SEEK_SET) < 0) {
        int err = errno;
        WARN("Bad smartport block requested for \"%s\", offset %zu.\n",
             d->fname, (size_t)sekpos);
        RETURN_ERROR(BadBlock);
    }

    unsigned char buf[512];
    errno = 0;
    if (fread(buf, 1, 512, d->fh) < 512) {
        int err = errno;
        DIE(1, "Couldn't read 512 bytes at offset %zu in hdd \"%s\".\n",
            (size_t)sekpos, d->fname);
    }
    mem_put(buf, buffer, 512);

    PPUT(PCARRY, false);
}

static void handle_sp_read_block(word params)
{
    byte pcount = peek(params++);
    byte unit   = peek(params++);
    byte buflo  = peek(params++);
    byte bufhi  = peek(params++);
    byte blklo  = peek(params++);
    byte blkmd  = peek(params++);
    byte blkhi  = peek(params++);

    if (pcount != 3) {
        WARN("Bad read_block pcount: %d\n", (int)pcount);
        RETURN_ERROR(BadPCnt);
    }

    off_t blkpos = (blkhi << 16) | (blkmd << 8) | blklo;
    read_block(unit, blkpos, WORD(buflo, bufhi));
}

static void handle_smartport_entry(void)
{
    // Get the return value off the stack
    byte lo = stack_pop();
    byte hi = stack_pop();
    word rts = WORD(lo, hi);
    word params = rts;

    // Push back on, the original return + 3 for the parameters after
    // the call
    rts += 3;
    stack_push(HI(rts));
    stack_push(LO(rts));
    
    // Start of params (one more than the return value)
    params += 1;
    byte fn = peek(params);
    params += 1;
    params = word_at(params); // Now get the "rest" of the params' location

    // BCD is guaranteed to be unset when SmartPort call exits.
    PPUT(PDEC, false);
    // Unused proc flag is guaranteed to be set.
    PPUT(PUNUSED, true);
    ACC = 0;

    switch (fn) {
        case 0x00:
            handle_status(params);
            break;
        case 0x01:
            handle_sp_read_block(params);
            break;
/*
        case 0x02:
            handle_sp_write_block(params);
            break;
*/
/*
        case 0x03:
            handle_format(params);
            break;
        case 0x04:
            handle_control(params);
            break;
        case 0x05:
            handle_init(params);
            break;
        case 0x08:
            handle_read(params);
            break;
        case 0x09:
            handle_write(params);
            break;
*/
        default:
            DEBUG("Unsupported SmartPort call: 0x%0X\n", (unsigned int)fn);
            //handle_unknown(fn, params);
            RETURN_ERROR(BadCmd);
            break;
    }
}


static void handle_prodos_status(byte unit)
{
    if (unit != 0 && unit <= ndev) {
        DEBUG("ProDOS block entry STATUS cmd, unit %d\n", (int)unit);
        struct SPDev *d = &devices[unit-1];
        XREG = d->bsz[0];
        YREG = d->bsz[1];
        PPUT(PCARRY, false); // No error.
    }
    else {
        DEBUG("*BAD* ProDOS block entry STATUS unit number: %d\n", (int)unit);
        ACC = DevDiscon;
    }
}

static void handle_prodos_entry(void)
{
    byte cmd  = peek(0x42);
    byte unit = peek(0x43);

    /*
     Decode the ProDOS "unit number":

       7  6  5  4  3  2  1  0
      +--+--+--+--+--+--+--+--+
      |DR|  SLOT  | NOT USED  |
      +--+--+--+--+--+--+--+--+
    */
    byte drive = (unit & 0x80)? 2 : 1; // 1-indexed
    byte slot = (unit & 0x70) >> 4;

    // Assumes we're at slot 5
    if (slot != 5) {
        drive += 2;
    }

    word buffer = WORD(peek(0x44), peek(0x45));
    word blknum = WORD(peek(0x46), peek(0x47));

    // BCD is guaranteed to be unset when SmartPort call exits.
    PPUT(PDEC, false);
    // Unused proc flag is guaranteed to be set.
    PPUT(PUNUSED, true);

    ACC = 0; // No error code

    switch (cmd) {
        case 0:
            handle_prodos_status(drive);
            break;
        case 1:
            DEBUG("ProDOS READ cmd\n");
            read_block(drive, blknum, buffer);
            break;
        case 2:
            //break;
        default:
            DEBUG("Unsupported ProDOS block command.\n");
            DEBUG_CONT("cmd=%02X, unit=%02X\n");
            PPUT(PCARRY, true);
            ACC = BadCmd;
    }
}

static void handle_event(Event *e)
{
    if (e->type != EV_PRESTEP)
        return;

    // For now, assume slot is always slot 5
    if (PC == (0xC500 | smartport_ep)) {
        handle_smartport_entry();
    }
    else if (PC == (0xC500 | prodos_ep)) {
        handle_prodos_entry();
    }
    if (e->type != EV_PRESTEP ||
        (PC != (0xC500 | smartport_ep) && PC != (0xC500 | prodos_ep))) {

        return;
    }
}

static void init(void)
{
    if (!cfg.hdd_set)
        return;

    for (struct SPDev *d = devices; d != devices + ndev; ++d) {
        errno = 0;
        d->fh = fopen(d->fname, "rb+");
        int err = errno;
        if (!d->fh) {
            DIE(1, "Couldn't open hdd file \"%s\": %s\n", d->fname,
                strerror(err));
        }

        int fd = fileno(d->fh);
        errno = 0;
        struct stat s;
        if (fstat(fd, &s) < 0) {
            err = errno;
            DIE(1, "Couldn't stat hdd file \"%s\": %s\n", d->fname,
                strerror(err));
        }

        d->sz = s.st_size;
        unsigned long bcount = d->sz / 512;
        if (bcount * 512 != d->sz) {
            DIE(1, "HDD image file \"%s\" is not a multiple of 512 in length.\n");
        }
        for (int i=0; i!=3; ++i) {
            d->bsz[i] = bcount & 0xFF;
            bcount >>= 8;
        }
    }

    event_reghandler(handle_event);
}

static byte handler(word loc, int val, int ploc, int psw)
{
    // Is it a soft switch? We're not doing those, so we should probably
    // know if ProDOS is trying to access them...
    if (psw != -1) {
        DEBUG("SmartPort switch tickled at %04X.\n", (unsigned int)loc);
        return 0;
    }

    // ID bytes?
    if (ploc < sizeof id_bytes) {
        return id_bytes[ploc];
    }
    
    // Entry-point? Or apparently sometimes the exact final byte value
    if (ploc == prodos_ep || ploc == smartport_ep) {
        return 0x60; // RTS
    }

    // Undocumented, and not seen from other emulators, but ProDOS 8
    //  code appears to treat $FB as a sort of identifier byte.
    //  ID_Byte & $02 == set? SCSI card.
    if (ploc == 0xFB) {
        // Yes, SCSI card
        // return 0x02;

        // Returning $02 set just results in a spurious smartport STATUS call
        // to "unit 2", which apparently is "the SCSI unit". Maybe
        // we don't want that.
        return 0x00;
    }

    // Lo/hi bytes of number-of-blocks?
    if (ploc == 0xFC || ploc == 0xFD) {
        // Must do STATUS to find out, so $0000.
        return 0x00;
    }

    // Status byte?
    if (ploc == 0xFE) {
        /* $CnFE status byte
           bit 7 - Medium is removable.
           bit 6 - Device is interruptable.
           bit 5-4 - Number of volumes on the device (0-3).
           bit 3 - The device supports formatting.
           bit 2 - The device can be written to.
           bit 1 - The device can be read from (must be on).
           bit 0 - The device's status can be read (must be on). */
        return 0x37;
    }

    // Final byte? (identify entry point)
    if (ploc == 0xFF) {
        return prodos_ep;
    }

    // Anything else, return BRK.
    return 0x00;
}

void smartport_add_image(const char *fname)
{
    if (ndev == MAX_NDEV) {
        DIE(2, "Can't give --hdd more than four times.\n");
    }

    struct SPDev *d = &devices[ndev++];
    d->fname = fname;
}

PeriphDesc smartport = {
    init,
    handler,
};
