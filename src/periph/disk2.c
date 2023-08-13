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
static DiskFormatDesc disk1;
static byte data_register; // only used for write
static byte *rombuf;
static const size_t dsk_disksz = 143360;
static int pr_count;
static bool steppers[4];
static int  cog = 0;

bool drive_spinning(void)
{
    return motor_on;
}

static void init(void)
{
    rombuf = load_rom("cards/disk2.rom", 256, false);
    disk1 = disk_insert(cfg.disk);
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
        if (disk1.halftrack > 0) --disk1.halftrack;
        D2DBG("dec to %d", disk1.halftrack);
    } else if (cogright()) {
        cog = (cog + 1) % 4;
        if (disk1.halftrack < 69) ++disk1.halftrack;
        D2DBG("inc to %d", disk1.halftrack);
    } else {
        D2DBG("no change (%d)", disk1.halftrack);
    }
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

static void turn_off_motor(void)
{
    motor_on = false;
    disk1.spin(&disk1, false);
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
            disk1.spin(&disk1, true);
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
                disk1.write_byte(&disk1, data_register);
                data_register = 0; // "shifted out".
            } else {
                // XXX any even-numbered switch can be used
                //  to read a byte. But for now we do so only
                //  through the sanctioned switch for that purpose.
                ret = data_register = disk1.read_byte(&disk1);
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
