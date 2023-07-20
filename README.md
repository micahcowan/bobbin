# BOBBIN (for Apples!)

**Bobbin** is a "highly hackable" Apple \]\[ emulator, aimed especially at improving productivity for devs of Apple \]\[ software by offering convenient terminal-based interface options, redirectable "standard input and output"-oriented options, and custom-scripted, on-the-fly adjustments to the emulated machine.

[Click below](https://youtu.be/zMlG5CRfRDA) to watch a demonstration of an early version (roughly two days into development) of the pipe-able, standard I/O emulator interface (dubbed the "simple" interface):

**Please note:** the video shows the use of command-line options `--simple` and `-m ][`. In the latest version of the **bobbin**, you must use `-m ][+` (or `-m plus`), and also add `--simple-input canonical`, to obtain approximately the same results as shown in the video. If you use `-m ][` as shown in the video (and if your shell even allows that), instead of the correct `-m ][+`, then **bobbin** will drop you directly into the monitor program (from which you can enter Integer ("Woz") BASIC, by typing Ctrl-B followed by Enter, but from which you have no access to AppleSoft).
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
$ ./bobbin -m plus --simple
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

Just as with the machine type, a different default (called `tty`) is planned for what interface **bobbin** uses to display the emulation, and to accept user input. It is not necessary to specify the interface if **bobbin**'s standard input is being redirected from a file or a pipe, but it *is* necessary if **bobbin**'s program input is connected to a terminal. Even though at present there is only one supported interface, you must specify `--iface simple` (or just `--simple`) on the command-line for an interactive session with **bobbin**.

### Interactive "simple" interface

The most straightforward way to use the `simple` interface is interactively. If you run **bobbin** like:

```
$ ./bobbin -m plus --simple
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

Well, it's true that nothing visible has happened yet. But type `LIST` and hit Enter, and you'll see that all of the program code for a maze-generating program is fully loaded into the Apple's memory. Try typing `RUN` (and Enter), followed by `10,10` (and Enter), and see what kind of maze pops up! Try it again several times—just be sure you never type the `10,10` at the `]` AppleSoft prompt, only when asked for width and length, or you'll have accidentally entered garbage into the program code, and it will no longer be able to generate mazes.

This mode is handy for trying out/experimenting with your Apple programs.

### Choosing your flavor of input mode with `--simple-input`

There are currently two different "input modes" available for the `--simple` interface to use when it detects that a line of input is being requested during an interactive user session, selected by the option `--simple-input`. This option has zero effect on any behavior while **bobbin** is reading input from a pipe or a file—or anything that isn't a terminal. It also has no effect on how input is handled when a program running on the Apple is reading a single keypress—nor when an unrecognized routine is being used to fetch an input line. This is because **bobbin** cheats to detect "line" input, by knowing where in the firmware the line-input routine lives—if that's not the routine being used to fetch input, **bobbin** can't tell the difference between fetching a keypress or fetching a whole line of input.

#### `--simple-input apple`

The default (`--simple-input apple`) is to pass every character through directly to the emulated apple, and let it handle echoing the characters back, and backspacing over characters. This mode provides the classic Apple \]\[ behavior in which backspacing over characters leaves them visible on the line, and Ctrl-U moves the cursor back over them (Ctrl-U is what the right-arrow key generates on an Apple \]\[; the `--simple` interface in `--simple-input apple` mode doesn't know how to convert a right-arrow key on your terminal (which it may not even support) to a right-arrow key on the Apple, but if you type a Ctrl-U it will be passed through, and the Apple will recognize that as "right-arrow key" just the same). Note that Ctrl-U has an *entirely different* meaning under `--simple-input canonical`, on most Unix-style terminals.

The `apple` input mode does *not* grant you access to moving the cursor arround the screen using the Escape key in combination with other keys... or rather, it *does*, but you won't *see* it, because that requires full screen emulation (like the `--tty` interface will do). So, if you type something like:

```
]PRINT "HEY THERE"
HEY THERE

]
```

And if you then type Esc D Esc D and then type Ctrl-U a few times, you won't see the cursor move at all (unlike on an Apple screen), but you *will* see the letters from a couple lines above start to echo at the current cursor position:

```
]PRINT "HEY THERE"
HEY THERE

]EY THE
```

Our advice is to refrain from using the Esc key at line inputs.

The developer prefers `--simple-input apple` over `--simple-input canonical` for two reasons: (a) it is visually much closer to what the emulated Apple is actually seeing/processing, and (b) it keeps the interface experience perfectly consistent, in the event that a program is using it's own bespoke line-input routine (which **bobbin** cannot detect). Since, in `apple` mode, no distinction is attempted between handling normal keypresses, versus handling a line of input, those two cases do not differ.

#### `--simple-input canonical`

*(Warning, the developer reserves the right to remove this mode in the future, unless someone informs him that they love and vastly prefer this option.)*

The other choice, is to let your terminal handle the line input, instead of letting the emulated Apple machine handle it. That's what `--simple-input canonical` does. In this mode, the terminal you are typing from is placed in what Unix terminal documentation likes to call "canonical mode": special characters are treated specially, and input is read as a full line, before the program under control (**bobbin**, in this case) is given a single character of it. This has the advantage of making the experience feel rather more like a "native command-line app" than it does in `apple` mode.

Note that this mode does *not* allow you to use your favorite emacs-style keys, just like you do at your shell prompt. It's more like (actually, exactly like) how things work when you type a line of input into the Unix `cat` program. You can type characters, you can backspace over them, you can Ctrl-W to delete the last word, and you can use Ctrl-U to erase and retype the entire current line. (Note: Ctrl-W and Ctrl-U do not appear to work on WSL (Windows Subsystem for Linux), which **bobbin**'s developer discovered to their annoyance whilst developing this program chiefly on Ubuntu-in-Windows).

None of the characters that are typed into the line are sent to the emulated Apple, until the entire line has been typed, and sealed with a ~~kiss~~ carriage return.

While "canonical mode" cannot properly be described as "comfortable" to a Unix user, it may well be that it is at least *more* comfortable than the Apple \]\['s approach to line input, which is often regarded as surprising and burdensome—though the primary cause of that is that in some Apple machines there is a "Delete" key where one might expect a "Backspace" to be, and if one uses that key in hopes that it does in fact "Backspace", they are sorely confused. (An Apple's "Backspace" key is the same key as "left arrow", and is usually at the bottom of the keyboard rather than the top.) This point of confusion does not apply to **bobbin**, because it makes sure to translate the Delete character (that your terminal's Backspace key probably generates) into the backspace (left-arrow) character before sending it to the Apple.

Of course, for a user to be fully as comfortable with inputting lines in **bobbin** as they are at their shell prompt, one would need to use a fully-featured line editor, such as GNU's `getline` facility. Perhaps one day **bobbin** will have a `--simple-input getline` feature? (That ooccurrance would probably hasten the obsolescence of this `canonical` feature).
