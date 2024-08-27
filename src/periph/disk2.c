//  periph/disk2.c
//
//  Copyright (c) 2023-2024 Micah John Cowan.
//  This code is licensed under the MIT license.
//  See the accompanying LICENSE file for details.

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

static bool initialized;
static bool motor_on;
static bool drive_two;
static bool write_mode;
static DiskFormatDesc disk1;
static DiskFormatDesc disk2;
static byte data_register; // only used for write
static byte *rombuf;
static const size_t dsk_disksz = 143360;
static int pr_count;
static bool steppers[4];
static int cog1 = 0;
static int cog2 = 0;

static inline DiskFormatDesc *active_disk_obj(void)
{
    return drive_two? &disk2 : &disk1;
}

bool drive_spinning(void)
{
    return motor_on;
}

int active_disk(void)
{
    return drive_two? 2 : 1;
}

static void init(void)
{
    if (initialized) return;
    initialized = true;

    disk1 = disk_format_load(NULL);
    disk2 = disk_format_load(NULL);

    rombuf = load_rom("cards/disk2.rom", 256, false);

    if (cfg.disk) {
        disk1 = disk_format_load(cfg.disk);
    }
    if (cfg.disk2) {
        disk2 = disk_format_load(cfg.disk2);
    }
}

int eject_disk(int drive)
{
    if (motor_on && active_disk() == drive) {
        return -1;
    }

    init(); // to make sure

    if (drive == 1) {
        disk1.eject(&disk1);
        disk1 = disk_format_load(NULL);
    } else if (drive == 2) {
        disk2.eject(&disk2);
        disk2 = disk_format_load(NULL);
    }

    return 0;
}

PeriphDesc disk2card;
int insert_disk(int drive, const char *path)
{
    int err = eject_disk(drive);
    if (err != 0) return err;

    // Make sure we're inserted to slot 6
    // XXX should check for error/distinguish if we're already in that slot
    (void) periph_slot_reg(6, &disk2card);

    if (err) {
        return -1; // XXX should be a distinguishable err code
    }
    if (drive == 1) {
        disk1 = disk_format_load(path);
    } else if (drive == 2) {
        disk2 = disk_format_load(path);
    }

    return 0;
}

// NOTE: cog "left" and "right" refers only to the number line,
//       and not the physical cog or track head movement.
static inline bool cogleft(int *cog)
{
    int cl = ((*cog) + 3) % 4; // position to the "left" of cog
    return (!steppers[(*cog)] && steppers[cl]);
}

// NOTE: cog "left" and "right" refers only to the number line,
//       and not the physical cog or track head movement.
static inline bool cogright(int *cog)
{
    int cr = ((*cog) + 1) % 4; // position to the "right" of cog
    return (!steppers[(*cog)] && steppers[cr]);
}

static inline int *active_cog(void)
{
    return drive_two? &cog2 : &cog1;
}

static void adjust_track(void)
{
    DiskFormatDesc *disk = active_disk_obj();
    int *cog = active_cog();
    D2DBG("halftrack: ");
    if (cogleft(cog)) {
        *cog = ((*cog) + 3) % 4;
        if (disk->halftrack > 0) --disk->halftrack;
        D2DBG("dec to %d", disk->halftrack);
    } else if (cogright(cog)) {
        *cog = ((*cog) + 1) % 4;
        if (disk->halftrack < 69) ++disk->halftrack;
        D2DBG("inc to %d", disk->halftrack);
    } else {
        D2DBG("no change (%d)", disk->halftrack);
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
    DiskFormatDesc *disk = active_disk_obj();
    disk->spin(disk, false);
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
        {
            frame_timer_cancel(turn_off_motor);
            motor_on = true;
            DiskFormatDesc *disk = active_disk_obj();
            disk->spin(disk, true);
            event_fire_disk_active(drive_two? 2 : 1);
        }
            break;
        case 0x0A:
            if (motor_on && drive_two) {
                disk2.spin(&disk2, false);
                disk1.spin(&disk1, true);
            }
            drive_two = false;
            if (motor_on) {
                event_fire_disk_active(1);
            }
            break;
        case 0x0B:
            if (motor_on && !drive_two) {
                disk1.spin(&disk1, false);
                disk2.spin(&disk2, true);
            }
            drive_two = true;
            if (motor_on) {
                event_fire_disk_active(2);
            }
            break;
        case 0x0C:
        {
            DiskFormatDesc *disk = active_disk_obj();
            if (!motor_on) {
                // do nothing
            } else if (write_mode) {
                // XXX ignores timing
                disk->write_byte(disk, data_register);
                data_register = 0; // "shifted out".
            } else {
                // XXX any even-numbered switch can be used
                //  to read a byte. But for now we do so only
                //  through the sanctioned switch for that purpose.
                ret = data_register = disk->read_byte(disk);
            }
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
