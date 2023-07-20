# BOBBIN (for Apples!)

**Bobbin** is a "highly hackable" Apple \]\[ emulator, aimed especially at improving productivity for devs of Apple \]\[ software by offering convenient terminal-based interface options, redirectable "standard input and output"-oriented options, and custom-scripted, on-the-fly adjustments to the emulated machine.

[Click below](https://youtu.be/zMlG5CRfRDA) to watch a demonstration of an early version (roughly two days into development) of the pipe-able, standard I/O emulator interface (dubbed the "simple" interface):

**Please note:** the video shows the use of command-line options `--simple` and `-m ][`. There is no `--simple` option: use `--iface simple` in its place. The correct value of `-m` to use **bobbin** as shown, should be `-m ][+`, or `-m plus`. `-m ][` (or `-m two`, `-m original`) will drop you directly into the monitor program (from which you can enter Integer ("Woz") BASIC, by typing Ctrl-B followed by Enter).
<br />

[![a2vimode showcase video](https://img.youtube.com/vi/zMlG5CRfRDA/0.jpg)](https://youtu.be/zMlG5CRfRDA)

## Overview

### Current features

- Emualates an Apple \]\[+ (well really just a 6502, some RAM and the Apple ][ firmware)
- Simplistic text-entry interface, roughly equivalent to using an Apple ][ via serial connection
- Can accept redirected input (Integer BASIC program, AppleSoft program, or hex entry via monitor)
- Not rate-limited, so runs in "turbo mode"

### Planned features

- Full Apple \]\[ text page contents emulated to the tty (ncurses-like interface, as the default) (anyone up for Apple \]\[-over-telnet?)
- Watch a specified binary or disk-image file for changes, and automatically reload/run the emulation (for constant, instantaneous feedback during Apple \]\[ program development)
- Run in "turbo mode" until boot-up is finished, or a certain point in a program is reached. then switch to 1MHz mode to continue running
- Full graphics (not to the terminal) and sound emulation, of course
- Emulate an enhanced Apple //e by default
- Scriptable, on-the-fly modifications (via Lua?) to the emulated address space and registers, in response to memory reads, PC value, external triggers...
- Remote commands via Unix-socket or network connection

### Known issues

- No way to send Ctrl-RESET (coming soon)
- No way to use the open-apple or closed-apple keys (coming soon, but it'll likely have to be awkward)
- Currently always runs in "turbo mode", many times faster than a "real" Apple ][ machine. Rate-limiting will be added soon.

## Building Bobbin / Getting Started

```
$ cd src
$ make
```

Bobbin has no external dependencies as of right now, though this will change in the near future.
Once it has built successfully, try:

```
$ ./bobbin -m plus --iface simple
./bobbin: Looking for ROM named "apple2plus.rom" in /usr/share/bobbin/roms...
./bobbin: Looking for ROM named "apple2plus.rom" in ./roms...
./bobbin: FOUND ROM file "./roms/apple2plus.rom".
[Bobbin "simple" interactive mode.
 Ctrl-D at input to exit.]


]W$="WORLD"

]PRINT "HELLO, ";W$
HELLO, WORLD

]^D
```
