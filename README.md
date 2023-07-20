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
- The language card is not currently emulated, nor are the vast majority of software switches/special memory locations. It's just (the documented ops for) a 6502 CPU, RAM, and the firmware ROM.
- The BASIC `SAVE` command is useless (for now), and `LOAD` will proceed to hang the emulation, requiring you to force-quit via Ctrl-C. Same for the equivalent monitor commands, `R` and `W`,
- No disk emulation yet. That will obviously be very handy, and is a planned feature.

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

## Using Bobbin

### Choosing what type of Apple \]\[ to emulate

The eventual plan is for **bobbin** to emulate an enhanced Apple //e as the default, but this is not yet supported. In order to prevent future confusion when the Apple //e does become a supported machine type, **bobbin** does not default to either of the machine types that *are* currently supported: you must explicitly select the machine type via the `-m` switch.

There are currently two supported machine types, an Apple \]\[, and an Apple \]\[+. Since an (early) Apple \]\[+ is exactly equivalent to the original Apple \]\[ with the "autostart" ROM with AppleSoft (floating-point) BASIC built-in, the term "original Apple ][" is used here to mean that the original, "non-autostart" ROM that starts directly in the monitor program, and which included Integer BASIC and the original mini-assembler (including "trace" and "step" features), via entering `F666G` (not `!`) from the monitor. (Note that the "monitor" we are referring to here is a program that is built into all Apple \]\[ machines; we are not referring to a hardware display device in this context.)

To start **bobbin** in AppleSoft BASIC, use `-m plus`. Equivalent aliases to `plus` include: `+`, `][+`, `II+`, `twoplus`, and `autostart`.

To start **bobbin** in the system monitor, with Integer BASIC available via Ctrl-B, use `-m original`. Equivalent aliases to `original` include: `][`, `II`, `two`, `woz`, `int`, and `integer`.

Note that at least one shell—**zsh**, which is the default shell on Mac OS—does not accept `][` or `][+` unquoted; you would need to place them in single-quotes, or simply use one of the other aliases for the same machine.

## The "simple" interface

Just as with the machine type, a different default (called `tty`) is planned for what interface **bobbin** uses to display the emulation, and to accept user input. It is not necessary to specify the interface if **bobbin**'s standard input is being redirected from a file or a pipe, but it *is* necessary if **bobbin**'s program input is connected to a terminal. Even though at present there is only one supported interface, you must specify `--iface simple` on the command-line for an interactive session with **bobbin**.

### Interactive "simple" interface

The most straightforward way to use the `simple` interface is interactively. If you run **bobbin** like:

```
$ ./bobbin -m plus --iface simple
```

You will immediately be dropped into an AppleSoft BASIC (`]`) prompt, and **bobbin** will inform you to type Ctrl-D at an input prompt when you wish to exit. This is a great mode for just messing around with BASIC on an Apple \]\[, and because the interface is in your terminal, it's a lightweight way to "check something real wuick" without having to fire up a more graphics-oriented emulation program.

### Non-interactive, input-redirected "simple" interface

You can also just pipe some input into **bobbin**. **bobbin** will execute the commands found in your input, and then exit.

Example:
```
$ cat ../examples/amazing-run.bas | ./bobbin -m plus
./bobbin: Looking for ROM named "apple2plus.rom" in /usr/share/bobbin/roms...
./bobbin: Looking for ROM named "apple2plus.rom" in ./roms...
./bobbin: FOUND ROM file "./roms/apple2plus.rom".
                           AMAZING PROGRAM
              CREATIVE COMPUTING  MORRISTOWN, NEW JERSEY




WHAT ARE YOUR WIDTH AND LENGTH



.--.--.--.--.--.--.--.--.--.  .
I           I                 I
:--:--:  :--:  :  :  :  :--:--.
I     I        I  I  I     I  I
:  :  :--:--:--:--:  :  :  :  .
I  I  I        I     I  I  I  I
:  :  :  :--:  :  :--:  :  :  .
I  I  I     I  I  I     I     I
:  :--:  :  :  :  :--:--:--:  .
I     I  I  I     I        I  I
:--:  :  :  :--:--:  :--:--:  .
I        I  I           I     I
:  :--:--:--:  :--:--:  :  :--.
I        I     I     I  I  I  I
:--:--:  :  :--:  :  :  :  :  .
I  I     I     I  I  I     I  I
:  :  :--:--:  :  :  :--:  :  .
I  I     I  I     I  I  I     I
:  :--:  :  :  :--:  :  :--:--.
I           I  I              I
:--:--:--:--:--:  :--:--:--:--.
```

This mode is very useful for testing that a program you're writing is working as expected. It could easily form the basis of a suite of regression tests.

Note that all the `]` prompts printed by the emulated Apple `\]\[+ have been suppressed, so as not to clutter up the output.

### Mixed-interactivity via `--remain`

If you specify the `--remain-after-pipe` option (or just `--remain`), then **bobbin** will start in non-interactive mode, suppressing prompts, until all of its input has been consumed. It will then remain open, and switch to user input until the user types Ctrl-D (and any prompts the Apple emits will output successfully).

Example (note: `amazing.bas`, not `amazing-run.bas` as in the previous example):
```
$ cat ../examples/amazing.bas | ./bobbin -m plus --remain
./bobbin: Looking for ROM named "apple2plus.rom" in /usr/share/bobbin/roms...
./bobbin: Looking for ROM named "apple2plus.rom" in ./roms...
./bobbin: FOUND ROM file "./roms/apple2plus.rom".

[Bobbin "simple" interactive mode.
 Ctrl-D at input to exit.]

]
```

...wait, what happened?! Why is it just the normal AppleSoft prompt again, with no output?

Well, it's true that nothing visible has happened yet. But type `LIST` and hit Enter, and you'll see that all of the program code for a maze-generating program is fully loaded into the Apple's memory. Try typing `RUN` (and Enter), followed by `10,10` (and Enter), and see what kind of maze pops up! Try it again several times&mdash;just be sure you never type the `10,10` at the `]` AppleSoft prompt, only when asked for width and length, or you'll have accidentally entered garbage into the program code, and it will no longer be able to generate mazes.

This mode is handy for trying out/experimenting with your Apple programs.
