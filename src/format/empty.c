#include "bobbin-internal.h"

void spin(DiskFormatDesc *d, bool b)
{
}

byte read_byte(DiskFormatDesc *d)
{
    // A real disk can never send a low byte.
    // But an empty disk must never send a legitimate byte.
    return 0x00;
}

void write_byte(DiskFormatDesc *d, byte b)
{
}

void eject(DiskFormatDesc *d)
{
}

DiskFormatDesc empty_disk_desc = {
    .spin = spin,
    .read_byte = read_byte,
    .write_byte = write_byte,
    .eject = eject,
};
