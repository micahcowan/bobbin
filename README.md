# BOBBIN (for Apples!)

**Bobbin** is a "highly hackable" Apple \]\[ emulator, aimed especially at improving productivity for devs of Apple \]\[ software by offering convenient terminal-based interface options, redirectable "standard input and output"-oriented options, and custom-scripted, on-the-fly adjustments to the emulated machine.

[Click below](https://youtu.be/zMlG5CRfRDA) to watch a demonstration of an early version (roughly two days into development) of the pipe-able, standard I/O emulator interface (dubbed the "simple" interface):

**Please note:** the video shows the use of command-line options `--simple` and `-m ][`. In the latest version of the **bobbin**, you must use `-m ][+` (or `-m plus`), and also add `--simple-input fgets`, to obtain approximately the same results as shown in the video. If you use `-m ][` as shown in the video (and if your shell even allows that), instead of the correct `-m ][+`, then **bobbin** will drop you directly into *Integer* (Woz) BASIC, instead of AppleSoft.
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
- Use **bobbin** as a compiler or assembler, by loading in your sources, running your favorite assembler (or what have you) in the emulated Apple, and then save the results back out
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
$ autoreconf --install
$ ./configure
$ make
```

You must have GNU autoconf and automake installed for the `autoreconf`
part to work
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

<!-- The Synopsis is used to generate bobbin --help text; -->
<!-- be careful about editing it! -->
### Synopsis

**bobbin --help**<br />
**bobbin --simple -m** *machine* \[ *options* \]<br />
**bobbin -m** *machine* \[ *options* \] **&lt;** *file*

### Options

Note: **bobbin**'s option parser treats **--option** and **-option** the same. Options that take value arguments can either give the option in one argument, and the value in the next, or else it can be specified all in one, like **--option=value**. Consequently, **bobbin** does *not* suport stringing a series of short options together, like **-cfb** to mean **-c**, **-f**, **-b**.

<!-- Note: if you change anything in the Options section, it will AFFECT PROGRAM BEHAVIOR! -->
<!--       this README.md is parsed for some of the information about program options.     -->
<!--       Only the first paragraph of each option's description gets copied to --help     -->
<!--START-OPTIONS-->
#### Basic Options

##### -h, --help

Display basic information on how to use **bobbin**.

##### -v, --verbose

Increase **bobbin**'s informational output. Use again for even more.

Normally **bobbin** only produces information about program-ending errors, or warnings about configuration or situations that might lead to unexpected operation.

##### -q, --quiet

Don't produce warnings, only errors.

#### Essential Options

##### -m, --machine, --mach *arg*

The machine **bobbin** should emulate.

This is currently a required argument, to tell **bobbin** what type of Apple \]\[ it should be. It *does* have a default value (emulating an enahnced IIe, or `//e`), but that machine is not yet supported, and so you must choose something else&mdash;either `original` or `plus` (or their equivalents). See [Choosing what type of Apple to emulate](#choosing-what-type-of-apple--to-emulate), below.

Valid values are:
<!--MACHINES-->
 - `original`. Aliases: `\]\[`, `II`, `two`, `woz`, `int`, `integer`.<br />
   Selects an original, non-autostart Apple \]\[ with Integer BASIC loaded. The `simple` interface boots directly into Integer BASIC (unlike the real machine); all other interfaces boot directly to the system monitor (actual machine behavior).
 - `plus`. Aliases: `+`, `][+`, `II+`, `twoplus`, `autostart`, `applesoft`, `asoft`.<br />
   Selects an Apple \]\[+ (or, equivalently an original Apple \]\[ with the autostart, AppleSoft BASIC firmware). Boots into AppleSoft.

Planned future values:
 - `twoey`. Aliases: `][e`, `IIe`.<br />
   An unenhanced Apple \]\[e.
 - `enhanced`. Alias: `//e`.<br />
   The enhanced Apple //e. This will be the default `--machine` value, in future.
<!--/MACHINES-->

##### --if, --interface, --iface *arg*

Select the user interface.

This is currently a required argument, unless **bobbin**'s input is from a file or pipe rather than a terminal. The default value, `tty`, has not yet been implemented. Currently only `simple` is supported. You may also specify `--simple`, instead of `--if simple`. See the section on [the "simple" interface](#the-simple-interface), below, for quite a lot of detail.

##### --simple

Alias for `--interface=simple`.

#### Machine configuration options

##### --no-turbo

Run at normal Apple \]\[ speed, instead of running as fast as possible.

##### --no-rom

Refrain from loading a ROM file into the upper address space.

When this option is specified, `--load` *arg* must be used as well (else there is no code to be run), and the `-m`/`--machine` option is no longer required.

Note that this option makes it possible to programatically change the contents of the `NMI`, `INT`, and `RESET` vectors at the extreme end of addressable space.

##### --load *arg*

Load the specified binary file into RAM.

When this option is specified, the given file will be loaded into the emulated machine RAM. It will be loaded starting at the memory location given to the `--load-at` argument, or `$0000` if that option was not provided.

Note that, if the loaded file overlaps ROM space (`$D000` thru `$FFFF`), the RAM in that space will not be accessible, unless the `--no-rom` option was given, or appropriate soft switches are used to grant access to RAM at that space (not yet implemented). The same is true if `--ram` is used to limit the amount of available RAM below the end of the loaded contents (the file can't be loaded into RAM that isn't there!). If a file is loaded across the region reserved for soft switches and slotted peripherals, `$C000` thru `$CFFF`, it is diverted to an alternate RAM bank (the so-called "first 4k bank") at `$D000` thru `$DFFF`.

A file's contents are mapped into RAM pretty much as one would expect: if a file is loaded into the start of RAM, at `$0000`, then all the locations in the file will correspond to the same loctions in RAM, *with the following exceptions*:

- Anything past position `$FFFF` in the file will be mapped into Auxiliary RAM, wrapping around again at memory location `$0000` of auxiliary RAM. Thus, file position `$1000` corresponds to location `$1000` in the "main" RAM, while file position `$11000` corresponds to location `$1000` in the "auxiliary" RAM bank.
- File locations `$C000` thru `$CFFF`, and `$1C000` thru `$1CFFF`, are *not* mapped into their corrsponding locations in addressable memory as described in the previous bullet point, since RAM is never mapped to those locations (they are reserved for slotted peripheral firmware). They are instead mapped to the "alternate" 4k RAM banks ("main RAM bank 1" and "auxilliary RAM bank 2", respectively), which are accessed at region `$D000` thru `$DFFF` (but *only* when appropriate soft switches have been activated).

If the file's contents would exceed the end of even auxilliary memory (128k), **bobbin** will exit with an error.

##### --load-at, --load-loc  *arg*

RAM location to load file from `--load`.

The *arg* must be a hexadecimal 16-bit value, optionally preceded by `$` or `0x`. If instruction evaluation begins while PC holds this value, the emulator will exit with a status of zero (success), and the message `.-= !!! REPORT SUCCESS !!! =-.`.

##### --start-at, --start-loc

Specify an initial start position in place of what's in the reset vector.

##### --delay-until-pc, --delay-until *arg*

Delays the effects of `--load` (RAM) and `--start-at`, until PC is *arg*.

The primary intent of this option is to allow the Apple \]\[ to run through its usual boot-up code, and then load new contents into memory and jump to a new, post-boot-sequence "start". This makes it easy to boot the emulator into your favorite code, without involving a disk-load.

**Bobbin**'s developer likes to use the value `FD75`, which jumps to your code as soon as BASIC (whether AppleSoft or Integer) tries to prompt the user for input.

##### --ram *arg*

Select how much RAM is installed on the machine, in kilobytes.

The default is the maximum possible amount normally possible (without third-party additions or modifications) on the specified machine&mdash;64k for the \]\[ or \]\[+, 128k on the others. The first 16k over 48k (totalling 64k) will be used as memory in an emulated "Language Card" (memory that could be used in case of the BASIC ROM firmware, to load an alternative "built-in" language to the one installed. (*Note: Language Card emulation is not yet implemented.*) If you specify 128k, then the top 64k of that will be used for so-called *auxiliary memory*, used for things like 80-column text support, "doubled" low-res and high-res graphics, and the ProDOS `/RAM` disk.

Acceptable values are: 4, 8, 12, 16, 20, 24, 32, 36, 48, 64, or 128. 28, 40, and 44 will also be permitted, but a warning will be issued as these were not normally possible configurations for an Apple \]\[. Above 48, only 64 or 128 are allowed. Note that while they are accepted, currently no values about 48 have any impact on the emulation, as neither the language card nor auxiliary memory are implemented yet.

#### "Simple" interface options

##### --remain

Go interactive after input is exhausted.

This option applies when **bobbin**'s input has been redirected, say from a file or pipe, and is not connected to the terminal. **Bobbin** ordinarly would quit once that input is exhausted, but if the `--remain` option is specified and **bobbin** is on a terminal, it will reconnect input through the terminal and continue executing with the `simple` interface.

##### --simple-input *arg*

`apple` (default), or `fgets`.

See [](#choosing-your-flavor-of-input-mode-with---simple-input) below, for copious details about this option. The TL;DR of it is that `fgets` uses your terminal's (primitive) line-input processing when **bobbin** detects that the Apple \]\[ wants to read a line of input (as opposed to a single character), and `apple` (the default) defers to the emulated Apple computer's (equally primitive, yet distinctly different) line-input processing. 

This option has no effect while **bobbin** is not reading from a terminal.

#### Diagnostics and Testing Options

##### --die-on-brk

Exit emulation with an error, if a BRK or illegal opcode is encountered.

##### --trace-to *m*\[!*n*\]

Trace N instructions before/including M (default N = 256).

This feature is intended for use with the `--trap-failure` feature. If the failure trao is triggered, it will output the instruction number where it did. You can then plug that number into this option, followed by an exclamation point and the number of instructions *before* the number you gave, that should be logged to the trace file (`trace.log` by default; adjustable with `--trace-file`). As an example, if the failure traps at instruction *12564*, then `--trap-failure 12564!100` would start logging instructions, register values, and (explicit) memory accesses starting at instruction *12465*. If instruction *12564* is reached, the trace logging ends. If instruction *12564* does not trigger the failure trap, then execution will still continue, but no more information will be output to the trace-log file.

This feature is used by **bobbin**'s build system to debug the emulator's correctness. It could also be used for Apple developers to examine unexpected behavior in their software via automated tests.

##### --trace-file *arg*

Trace log file to use instead of `trace.log`.

##### --trap-failure *arg*

Exit emulator with an error if execution reaches this location.

The *arg* must be a hexadecimal 16-bit value, optionally preceded by `$` or `0x`. If instruction evaluation begins while PC holds this value, the emulator will exit with the message `*** REPORT ERROR ***`, report the current instruction number and CPU status, and exit with a non-zero status. It will also interpret the value of the byte at memory location `$200` as "the current case number", and report that as well (this behavior is used in the testing of **bobbin**'s emulation accuracy).

Together with `--trap-success`, this option can be used to run automated tests on your Apple \]\[ code, since it will cause the emulator to exit with a suitable exit status.

##### --trap-success *arg*

Exit emulator, reporting success, if execution reaches this location.

The *arg* must be a hexadecimal 16-bit value, optionally preceded by `$` or `0x`. If instruction evaluation begins while PC holds this value, the emulator will exit with a status of zero (success), and the message `.-= !!! REPORT SUCCESS !!! =-.`.

Together with `--trap-success`, this option can be used to run automated tests on your Apple \]\[ code, since it will cause the emulator to exit with a suitable exit status.

<!--END-OPTIONS-->
### Choosing what type of Apple \]\[ to emulate

The eventual plan is for **bobbin** to emulate an enhanced Apple //e as the default, but this is not yet supported. In order to prevent future confusion when the Apple //e does become a supported machine type, **bobbin** does not default to either of the machine types that *are* currently supported: you must explicitly select the machine type via the `-m` switch.

There are currently two supported machine types, an Apple \]\[, and an Apple \]\[+. Since an (early) Apple \]\[+ is exactly equivalent to the original Apple \]\[ with the "autostart" ROM with AppleSoft (floating-point) BASIC built-in, the term "original Apple ][" is used here to mean that the original, "non-autostart" ROM that starts directly in the monitor program, and which included Integer BASIC and the original mini-assembler (including "trace" and "step" features), via entering `F666G` (not `!`) from the monitor. (Note that the "monitor" we are referring to here is a program that is built into all Apple \]\[ machines; we are not referring to a hardware display device in this context.) As a special feature of the `--simple` interface, **bobbin** does not actually drop you into the monitor with `-m \]\[ --simple`; instead it jumps you into Integer BASIC, as that's the more likely thing you might want to pipe a program into (you can still get to the monitor via `CALL-151`, of course!).

To start **bobbin** in AppleSoft BASIC, use `-m plus`, or one of its aliases (see [the description of the `--machine` option](#-m---machine---mach-arg), above).

To start **bobbin** in the system monitor, with Integer BASIC available via Ctrl-B, use `-m original`, or one of its aliases (see [the description of the `--machine` option](#-m---machine---mach-arg), above). Note that the `--simple` interface (the only interface currently available in **bobbin**) actually drops you directly into Integer BASIC, instead of the monitor. While this behavior is unrealistic to the original machine, it is thought to be more convenient for the purpose of redirected input (as you don't have to start your files with a Ctrl-B character).

Note that at least one shell—**zsh**, which is the default shell on Mac OS—does not accept `][` or `][+` unquoted; you would need to place them in single-quotes, or simply use one of the other aliases for the same machine.

## The "simple" interface

Just as with the machine type, a different default (called `tty`) is planned for what interface **bobbin** uses to display the emulation, and to accept user input. It is not necessary to specify the interface if **bobbin**'s standard input is being redirected from a file or a pipe, but it *is* necessary if **bobbin**'s program input is connected to a terminal. Even though at present there is only one supported interface, you must specify `--iface simple` (or just `--simple`) on the command-line for an interactive session with **bobbin**.

### Interactive "simple" interface

The most straightforward way to use the `simple` interface is interactively. If you run **bobbin** like:

```
$ ./bobbin -m plus --simple
```

You will immediately be dropped into an AppleSoft BASIC (`]`) prompt, and **bobbin** will inform you to type Ctrl-D at an input prompt when you wish to exit. This is a great mode for just messing around with BASIC on an Apple \]\[, and because the interface is in your terminal, it's a lightweight way to "check something real quick" without having to fire up a more graphics-oriented emulation program.

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

If you specify the `--remain` option, then **bobbin** will start in non-interactive mode, suppressing prompts, until all of its input has been consumed. It will then remain open, and switch to user input until the user types Ctrl-D (and any prompts the Apple emits will output successfully).

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

The default (`--simple-input apple`) is to pass every character through directly to the emulated Apple, and let it handle echoing the characters back, and backspacing over characters&mdash;what you see is fairly close to what you'd see on real Apple ][ hardware (in terms of input behavior, anyway). This mode provides the classic Apple \]\[ behavior in which backspacing over characters leaves them visible on the line, and the right-arrow key moves the cursor back over them.

On some terminals, the left-and-right arrow keys may not work. In such cases, Ctrl-U may be used as *right-arrow* instead. Ctrl-H is *left-arrow* (and is what Apple used as backspace). If you want to toggle between moving left and right while editing a line, Ctrl-H/Ctrl-U are conveniently close to one another; but if you just want to backspace over some characters, your keyboard's Backspace key is the better option. Note that Ctrl-U has an *entirely different* meaning under `--simple-input fgets`, on most Unix-style terminals (erases the current input line).

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

Our advice is to refrain from using the Esc key at line inputs. If you want to copy/re-enter a line, it's usually easier just to use your terminal's copy/paste feature!

The developer prefers `--simple-input apple` over `--simple-input fgets` for two reasons: (a) it is visually much closer to what the emulated Apple is actually seeing/processing, and (b) it keeps the interface experience perfectly consistent, in the event that a program is using its own bespoke line-input routine (which **bobbin** cannot detect). Since, in `apple` mode, no distinction is attempted between handling normal keypresses, versus handling a line of input, those two cases do not differ.

#### `--simple-input fgets`

*(Warning, the developer reserves the right to remove this mode in the future, unless someone informs him that they love and vastly prefer this option.)*

The other choice, is to let your terminal handle the line input, instead of letting the emulated Apple machine handle it. That's what `--simple-input fgets` does. In this mode, the terminal you are typing from is placed in what Unix terminal documentation likes to call "canonical mode": special characters are treated specially, and input is read as a full line, before the program under control (**bobbin**, in this case) is given a single character of it. This has the advantage of making the experience feel rather more like a "native command-line app" than it does in `apple` mode.

Note that this mode does *not* allow you to use your favorite emacs-style keys, just like you do at your shell prompt. It's more like (actually, exactly like) how things work when you type a line of input into the Unix `cat` program. You can type characters, you can backspace over them, you can Ctrl-W to delete the last word, and you can use Ctrl-U to erase and retype the entire current line. (Note: Ctrl-W and Ctrl-U do not appear to work on WSL (Windows Subsystem for Linux), which **bobbin**'s developer discovered to their annoyance whilst developing this program chiefly on Ubuntu-in-Windows).

None of the characters that are typed into the line are sent to the emulated Apple, until the entire line has been typed, and sealed with a ~~kiss~~ carriage return.

While `fgets` mode cannot properly be described as "comfortable" to a Unix user, it may well be that it is at least *more* comfortable than the Apple \]\['s approach to line input, which is often regarded as surprising and burdensome—though the primary cause of that is that in some Apple machines there is a "Delete" key where one might expect a "Backspace" to be, and if one uses that key in hopes that it does in fact "Backspace", they are sorely confused. (An Apple's "Backspace" key is the same key as "left arrow", and is usually at the bottom of the keyboard rather than the top.) This point of confusion does not apply to **bobbin**, because it makes sure to translate the Delete character (that your terminal's Backspace key probably generates) into the backspace (left-arrow) character before sending it to the Apple.

However, one remaining point of confusion that persists in **bobbin**'s `--simple-input apple` mode, is that control characters that have been typed will not echo to the screen, and yet they have still been input, and will produce `SYNTAX ERRORS` or the like. This is less likely to happen with `fgets` mode.

Of course, for a user to be fully as comfortable with inputting lines in **bobbin** as they are at their shell prompt, one would need to use a fully-featured line editor, such as GNU's `readline` facility. Perhaps one day **bobbin** will have a `--simple-input readline` feature? (That ooccurrance would probably hasten the obsolescence of this `fgets` feature).

When the line is done being typed, and is ready to send as input to the emulated Apple \]\[, **bobbin** turns on "output suppression" while the Apple is busy reading the characters. Once the Apple has slurped up the final carriage return from the line input, **bobbin** suppresses the very next character that's output (normally that same carriage return), and then re-enables output. This step is necessary because without the output suppression, the Apple would immediately and redundantly echo the line back out at you, because it has no way to know those characters are already appearing on your screen.

Note that the characters you type in `fgets` mode are not the actual characters the Apple \]\[ will see. Any lowercase characters are converted to uppercase, because an unmodified Apple \]\[ did not have lowercase characters, nor any means to type them. This probably won't surprise you, but what *may* surprise you is that some of the punctuation you type may be similarly transformed, as well. The original Apple \]\[ did not have the punctuation characters `{|}~` and \`; these are swapped out for `@[\\]` and `^`. Note that these same transformations are performed on piped input as well, and in `--simple-mode apple` processing (where it is immediately visible when entered).

#### Non-interactive line input

When **bobbin** detects that a line of input is being fetched, it suppresses output while that line is being read (just as described at the end of [the `--simple-input fgets` section](#--simple-input-fgets) just above. This is to support a familiar interface similar to other standard I/O tools: input comes into a program, output comes out. If output weren't suppressed during line input, the Apple would echo it back in the output. If you pipe a thousand-line BASIC program into **bobbin**, you'll probably be annoyed if you have to wade through those thousand lines you already had, just to get to the "good part" (the BASIC program's output).

For the same reasons, during non-terminal input, the `simple` interface also suppresses prompt characters and preceding newlines, that come just before line input is requested. Just as you don't want to see a thousand lines of BASIC code you just sent get thrown back at your face, you also don't want to see the thousand prompt characters that were issued, either. This "prompt suppression" is not performed during the other (selectable) input modes.

There is no option to control the input-mode used for non-interactive input. The developer's in-my-head name for it is "pipe" mode.

#### How the `--simple` interface distinguishes line-input from keypresses

**Bobbin** watches the Apple \]\['s 6502 CPU register called the PC (program counter) register, which tracks the location in memory where the Apple is currently executing code. If it reaches a place in the firmware that **bobbin** knows corresponds to a particular place in the Apple \]\[ firmware "monitor" program known as `GETLN`, then the specialized "line input" behavior is triggered. The locations used to match against the PC to determine if `GETLN` is running, are `$FD75` (`NXTCHAR`), `$FD67` (`GETLNZ`), and `$FD6A` (`GETLN`). The latter two locations are only used in non-interactive ("pipe") mode, to suppress prompt-printing, while the first location is used both to suppress output in non-interactive mode, and also to trigger "fgets" line input mode (when `--simple-input fgets` is specified). If the non-autostart, Integer BASIC firmware is loaded, an additional location is used for prompt-detection on the program counter: `$E006`.

Character output is detected when the program counter reaches `$FDF0` (`COUT1`). When output is not being suppressed, this triggers the character found in the accumulator register to be printed to **bobbin**'s output stream (after suitable transformation, from Apple \]\['s character encoding scheme to ASCII). Output suppression *in no way* affects whether, or in what way, a character is printed *interanlly* to the Apple \]\['s video memory.

While one might be tempted to think that character input probably mirrors how character output is done&mdash;matching the program counter against a memory location like `$FD1B` (`KEYIN`), it is actually detected in a completely different way: by trapping memory reads from special memory locations `$C00x` and `$C01x` (where `x` here represents any hexadecimal nybble value). The former location is used by an Apple \]\[ program to read whether a character is available to be read, and what its value is, while the `$C01x` memory is used just to tell the Apple \]\[ key-handling hardware that we've accepted the keypress, and that it shouldn't set the "key-is-ready" bit at `$c00x` until the user presses another key. **Bobbin** needs to use this method for providing user input, rather than checking for entry into a subroutine, because many, many programs use that method directly to read keypresses, and if **bobbin** didn't handle them, those programs might loop forever, always waiting for a keypress that **bobbin** would never know to deliver.

#### Additional details of bobbin's I/O processing

If **bobbin** receives a `SIGINT` signal (often triggered by a user typing Ctrl-C), **bobbin** treats it exactly as a normal Ctrl-C keypress, except that if there is some input that **bobbin** has received but which hasn't yet been processed by the program running on the Apple \]\[, the Ctrl-C may jump to the front of the line, ahead of other characters that had actually been typed before it. Note that it is possible to configure a terminal to use some other keypress instead of Ctrl-C for sending `SIGINT`; in this case **bobbin** will still treat that character as if it had been a Ctrl-C.

If **bobbin** receives a second `SIGINT` signal before the emulated Apple has had a chance to consume it as a keypress; or if Ctrl-C is pressed twice in a row without any intervening characters (even if the first Ctrl-C was in fact processed successfully as input to the emulated Apple), then **bobbin** will open a debugger interface.

If **bobbin** is processing non-interactive input while a `SIGINT` is received, and the `--remain` option is active, it will do three things in succession: (1) it will send a Ctrl-C to the program on the Apple \]\[ (in BASIC, this usually has the effect of causing a program `BREAK`); (2) it will discard any pipe or redirected input that was still waiting to be processed by the Apple \]\['s program, and (3) it will immediately enter "interactive" mode, and process line inputs according to whatever setting of `--simple-input` may be active.

In the `--simple` interface, if a Ctrl-D is input by the user, then it indicates that **bobbin** should exit (**bobbin** may see a Ctrl-D entered directly, or it may "percieve" it via a return from `read()` that indicated no bytes remained to be read). **Bobbin** does not exit *immediately* upon receipt of a Ctrl-D. Instead, it waits until the program in the Apple \]\[ has caught up with the user input, up to the point when the Ctrl-D was typed, and issues a carriage return character. Only when the Apple \]\[ program indicates "consumption" of that carriage return, does **bobbin** then exit. This is to make the behavior consistent with other programs at a terminal.
