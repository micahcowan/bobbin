#include "bobbin-internal.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

//#define DISK_DEBUG
#ifdef DISK_DEBUG
# define D2DBG(...)  do { \
        if (pr_count == 1) \
            fprintf(stderr, __VA_ARGS__); \
    } while(0)
#else
# define D2DBG(...)
#endif

static bool motor_on;
static bool drive_two;
static bool write_mode;
static byte data_register; // only used for write
static byte *rombuf;
static byte *diskbuf;
static size_t disksz;
static const size_t dsk_disksz = 143360;
static const size_t nib_disksz = 232960;
static int pr_count;
static bool steppers[4];
static int  cog = 0;
static int  halftrack = 69; // start at innermost track

#define NIBBLE_TRACK_SIZE   6656
#define NIBBLE_SECTOR_SIZE  416

#define NUM_TRACKS      35
#define MAX_SECTORS     16
#define SECTOR_SIZE     256

static const byte sechdr[]  = { 0xD5, 0xAA, 0x96 };
static const byte datahdr[] = { 0xD5, 0xAA, 0xAD };
#define HDR_START           (-14)
#define HDR_VOL             (HDR_START + 3) // sizeof sechdr
#define HDR_TRACK           (HDR_VOL+2)
#define HDR_SECTOR          (HDR_TRACK+2)
#define HDR_CKSUM           (HDR_SECTOR+2)
#define HDR_DATAHDR         (HDR_CKSUM+2)
#define HDR_DATATWOS        (HDR_DATAHDR + 3) // sizeof datahdr
#define HDR_DATASIXES       (HDR_DATATWOS+86)
#define HDR_DATAEND         (HDR_DATASIXES+256)
#if (HDR_DATATWOS != 0)
// compile-time error
hdr_data_isnt_zero      HDR_DATA
#endif
static int secnum;
static int bytenum;
static int bitnum;

static uint64_t dirty_tracks;

static void init(void)
{
    rombuf = load_rom("cards/disk2.rom", 256, false);
    int err = mmapfile(cfg.disk, &diskbuf, &disksz, O_RDWR);
    if (diskbuf == NULL) {
        DIE(1,"Couldn't load/mmap disk %s: %s\n",
            cfg.disk, strerror(err));
    }
    if (disksz != nib_disksz) {
        DIE(0,"Wrong disk image size for %s:\n", cfg.disk);
        DIE(1,"  Expected %zu, got %zu.\n", nib_disksz, disksz);
    }
}

// NOTE: cog "left" and "right" refers only to the number line,
//       and not the physical cog or track head movement.
static inline bool cogleft(void)
{
    int cl = (cog + 3) % 4; // position to the "left" of cog
    return (!steppers[cog] && steppers[cl]);
}

// NOTE: cog "left" and "right" refers only to the number line,
//       and not the physical cog or track head movement.
static inline bool cogright(void)
{
    int cr = (cog + 1) % 4; // position to the "right" of cog
    return (!steppers[cog] && steppers[cr]);
}

static void adjust_track(void)
{
    D2DBG("halftrack: ");
    if (cogleft()) {
        cog = (cog + 3) % 4;
        if (halftrack > 0) --halftrack;
        D2DBG("dec to %d", halftrack);
    } else if (cogright()) {
        cog = (cog + 1) % 4;
        if (halftrack < 69) ++halftrack;
        D2DBG("inc to %d", halftrack);
    } else {
        D2DBG("no change (%d)", halftrack);
    }

    // No matter the result (even no change), reset sector
    bytenum = 0;
}

static void stepper_motor(byte psw)
{
    byte phase = psw >> 1;
    bool onoff = (psw & 1) != 0;

    D2DBG("phase %d %s ", (int)phase, onoff? "on, " : "off,");
    steppers[phase] = onoff;
    adjust_track();
}

static inline byte encode4x4(byte orig)
{
    return (orig | 0xAA);
}

#if DSK
// read_byte. Current implementation has extremely unrealistic timing,
// and just immediately feeds the program what it wants to see, without
// regard to sync bits or disk rotation speed or position, etc.
static byte read_byte(void)
{
    byte val = 0;

    if (bytenum >= HDR_DATAEND) {
        bytenum = HDR_START;
        bitnum = 0;
        secnum = (secnum + 1) % MAX_SECTORS;
    }

    int orignum = bytenum;
    if (bytenum >= HDR_DATASIXES) {
    } else if (bytenum >= HDR_DATATWOS) {
#if 0
        size_t pos = (halftrack/2) * MAX_SECTORS * SECTOR_SIZE;
        pos += secnum * SECTOR_SIZE;
        pos += (bytenum - HDR_DATATWOS) * 3;
        size_t pos1 =
        byte un = (diskbuf[pos
#endif
    } else {
        // Sector header + data header
        switch (bytenum) {
            case HDR_START:
            case HDR_START+1:
            case HDR_START+2:
                val = sechdr[bytenum - HDR_START];
                break;
            case HDR_VOL:
            case HDR_VOL+1:
                val = encode4x4(0); // don't care about a real vol number
                break;
            case HDR_TRACK:
                val = encode4x4(halftrack >> 2);
                break;
            case HDR_TRACK+1:
                val = encode4x4(halftrack >> 1);
                break;
            case HDR_SECTOR:
                val = encode4x4(secnum >> 1);
                break;
            case HDR_SECTOR+1:
                val = encode4x4(secnum);
                break;
            case HDR_CKSUM:
            case HDR_CKSUM+1:
                val = encode4x4(0); // XXX for now
                break;
            case HDR_DATAHDR:
            case HDR_DATAHDR+1:
            case HDR_DATAHDR+2:
                val = datahdr[bytenum - HDR_DATAHDR];
                break;
            default:
                ;
        }

        ++bytenum;
    }

    D2DBG("read_byte #%d: $%02X", orignum, val);
    return val;
}
#endif // DSK

static bool last_was_read;
// For .nib disks
static byte read_byte(void)
{
    last_was_read = true;
    size_t pos = (halftrack/2) * NIBBLE_TRACK_SIZE;
    pos += (bytenum % NIBBLE_TRACK_SIZE);
    byte val = diskbuf[pos];
    bytenum = (bytenum + 1) % NIBBLE_TRACK_SIZE;
    return val;
}

// For .nib disks
static void write_byte(byte val)
{
    if ((val & 0x80) == 0) {
        D2DBG("dodged write $%02X", val);
        return; // must have high bit
    }
    dirty_tracks |= 1 << (halftrack/2);
    size_t pos = (halftrack/2) * NIBBLE_TRACK_SIZE;
    pos += (bytenum % NIBBLE_TRACK_SIZE);

    D2DBG("write byte $%02X at pos $%04zX", (unsigned int)val, pos);

    last_was_read = false;
    diskbuf[pos] = val;
    bytenum = (bytenum + 1) % NIBBLE_TRACK_SIZE;
}

static void turn_off_motor(void)
{
    motor_on = false;
    // For now, sync the entire disk
    if (dirty_tracks != 0) {
        errno = 0;
        int err = msync(diskbuf, disksz, MS_SYNC);
        if (err < 0) {
            DIE(1,"Couldn't sync to disk file %s: %s\n",
                cfg.disk, strerror(errno));
        }
        dirty_tracks = 0;
    }
    event_fire_disk_active(0);
}

static int lastsw = -1;
static int lastpc = -1;
static byte handler(word loc, int val, int ploc, int psw)
{
    byte ret = 0;
    if (ploc != -1) {
        return rombuf[ploc];
    }

    // Soft-switch handling
    if (lastsw == -1) {
        lastsw = psw;
        lastpc = current_pc();
        pr_count = 1;
    } else if (lastsw == psw && lastpc == current_pc()) {
        ++pr_count;
    } else {
        int wascount = pr_count;
        pr_count = 1;
        if (wascount > 1) {
            D2DBG("  (^ repeat %d times).\n", wascount);
        }
        lastsw = psw;
        lastpc = current_pc();
    }

    frame_timer_reset(60, turn_off_motor);
    D2DBG("disk sw $%02X, PC = $%04X   ", psw, lastpc);
    if (val != -1)
        data_register = val; // ANY write sets data register
    if (psw < 8) {
        stepper_motor(psw);
    } else switch (psw) {
        case 0x08:
            if (motor_on) {
                frame_timer(60, turn_off_motor);
            }
            break;
        case 0x09:
            frame_timer_cancel(turn_off_motor);
            motor_on = true;
            event_fire_disk_active(drive_two? 2 : 1);
            break;
        case 0x0A:
            drive_two = false;
            if (motor_on)
                event_fire_disk_active(1);
            break;
        case 0x0B:
            drive_two = true;
            if (motor_on)
                event_fire_disk_active(2);
            break;
        case 0x0C:
            if (!motor_on || drive_two) {
                // do nothing
            } else if (write_mode) {
                // XXX ignores timing
                write_byte(data_register);
                data_register = 0; // "shifted out".
            } else {
                // XXX any even-numbered switch can be used
                //  to read a byte. But for now we do so only
                //  through the sanctioned switch for that purpose.
                ret = data_register = read_byte();
            }
            break;
        case 0x0D:
#if 0
            if (!motor_on || drive_two) {
                // do nothing
            } else if (write_mode) {
                data_register = (val == -1? 0: val);
            } else {
                // XXX should return whether disk is write-protected...
            }
#endif
            break;
        case 0x0E:
            write_mode = false;
            break;
        case 0x0F:
            write_mode = true;
            break;
        default:
            ;
    }

    D2DBG("\n");

    return ret;
}

PeriphDesc disk2card = {
    init,
    handler,
};
