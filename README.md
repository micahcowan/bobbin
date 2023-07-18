# BOBBIN (for Apples!)

an Apple \]\[ emulator targeted at devs of Apple \]\[ software, especially devs on Unixy systems who like to use the terminal

**NOTE**: a 12kb Apple \]\[+ ROM file named `apple2.rom` must exist in the working directory, or **bobbin** won't run.

## Current characteristics (many temporary)

- Emualates an Apple \]\[+ (well really just a 6502, some RAM and the Apple ][ firmware)
- Simplistic text-entry interface, roughly equivalent to using an Apple ][ via serial connection
- Can accept redirected input (AppleSoft program, or hex entry via monitor)
- No way to send Ctrl-RESET
- Not rate-limited, so runs in "turbo mode"
- I/O is automatically converted to uppercase

## Features planned

- Full Apple \]\[ text page contents emulated to the tty (ncurses-like interface, as the default) (anyone up for Apple \]\[-over-telnet?)
- Watch a specified binary or disk-image file for changes, and automatically reload/run the emulation (for constant, instantaneous feedback during Apple \]\[ program development)
- Run in "turbo mode" until boot-up is finished, or a certain point in a program is reached. then switch to 1MHz mode to continue running
- Full graphics (not to the terminal) and sound emulation, of course
- Emulate an enhanced Apple //e by default
- Scriptable, on-the-fly modifications (via Lua?) to the emulated address space and registers, in response to memory reads, PC value, external triggers...
- Remote commands via Unix-socket or network connection
