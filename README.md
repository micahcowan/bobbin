# BOBBIN (for Apples!)

**Bobbin** is a "highly hackable" Apple \]\[ emulator, aimed especially at improving productivity for devs of Apple \]\[ software by offering convenient terminal-based interface options, redirectable "standard input and output"-oriented options, and custom-scripted, on-the-fly adjustments to the emulated machine.

[Click below](https://youtu.be/zMlG5CRfRDA) to watch a demonstration of an early version (roughly two days into development) of the pipe-able, standard I/O emulator interface (dubbed the "simple" interface):

**Please note:** the video shows the use of command-line options `--simple` and `-m ][`. In the latest version of the **bobbin**, you must use `-m ][+` (or `-m plus`), and also add `--simple-input fgets`, to obtain approximately the same results as shown in the video. If you use `-m ][` as shown in the video (and if your shell even allows that), instead of the correct `-m ][+`, then **bobbin** will drop you directly into *Integer* (Woz) BASIC, instead of AppleSoft.
<br />

[![bobbin showcase video](https://img.youtube.com/vi/zMlG5CRfRDA/0.jpg)](https://youtu.be/zMlG5CRfRDA)

And here's another video (clickable) that showcases additional features in a more recent version of **bobbin** (~two weeks of work):

[![bobbin part 2](https://img.youtube.com/vi/yIJ4bPOrrko/0.jpg)](https://www.youtube.com/watch?v=yIJ4bPOrrko)

## Overview

### Current features

- Emualates an Apple \]\[, \]\[+, or (unenhanced) \]\[e (with 80-column support)
- Two available interfaces, both for running within a Unix-style terminal program
 - Simplistic text-entry interface, roughly equivalent to using an Apple ][ via serial connection
 - Complete screen-contents emulation via the curses library (anyone up for Apple \]\[-over-telnet?)
- Can accept redirected input (Integer BASIC program, AppleSoft program, or hex entry via monitor)
- Can automatically watch a binary file for changes, and reload itself instantly when it's updated
- Can delay loading/running a binary file until the system has completed the basic boot-up
- Can be used to tokenize or detokenize AppleSoft BASIC programs

### Planned features

- `.woz` disk format, and 13-sector support
- Hard-drive image support for ProDOS
- Full graphics (not to the terminal) and sound emulation, of course
- Emulate an enhanced Apple //e by default
- Scriptable, on-the-fly modifications (via Lua?) to the emulated address space and registers, in response to memory reads, PC value, external triggers...
- Use **bobbin** as a command-line disk editor, using real DOS or ProDOS under emulation to change the contents of a disk image
- Use **bobbin** as a command-line compiler or assembler, by loading in your sources, running your favorite assembler (or what have you) in the emulated Apple, and then save the results back out

### Known issues

- No way to use the open-apple or closed-apple keys (coming soon, but it'll likely have to be awkward)
- If booted without a disk operating system, the BASIC `SAVE` command is useless (for now), and `LOAD` will proceed to hang the emulation, requiring you to force-quit via Ctrl-C. Same for the equivalent monitor commands, `R` and `W`,

## Building Bobbin

```
$ autoreconf --install # Only do this if ./configure doesn't exist (non-release sources)
$ ./configure
$ make
```

### Build dependencies

Bobbin is written in SUSv4 / POSIX-compliant C (with some reasonable additional assumptions, such as an ASCII-compatible environment). It assumes it is running on a SUSv4-compliant Unix-like environment. Even if you are running on an operating system that is not, itself, Unix-like, there are likely Unix-like environments that it can host. The developer uses **bobbin** primarily on a Windows 11 system, hosting Ubuntu via [WSL](https://learn.microsoft.com/en-us/windows/wsl/install).

You need to have GNU autoconf and automake installed, to run the `autoreconf` command shown above. This step is *not* necessary for the release source packages (the ones that are *not* named "Source Code", even though they do in fact consist of just the source code, with the `./configure` script included (no need for the GNU autotools).

In addition, you will need the *development files* for a Unix **curses** or **nurses** implementation. This means header files (`<curses.h>`), and library-file symlinks suitable for development. Your system's packaging system may call it something like **libncurses-dev**, or **ncurses-devel**.

The `--watch` feature currently requires Linux, and its **inotify** facility.

To run the included (as-yet incomplete) tests, you must also have available:
 - the ca65 assembler and ld65 linker, from [the cc65 project](https://cc65.github.io/).
 - Python 3, and the Python **pexpect** module.

## Bobbin Usage Examples

### Simple boot-up

Once it has built successfully, try:

```
$ ./bobbin -m iie
```

(To exit, type Control-C twice in a row, which will bring up a command interface, then type `q` followed by the Enter key.)

### Run Bobbin in line-oriented ("simple") mode

```
$ ./bobbin -m plus --simple

[Bobbin "simple" interactive mode.
 Ctrl-D at input to exit.
 Ctrl-C *TWICE* to enter debugger.]


]10 FOR I=1 TO 5 RULEZ!

]20 PRINT SPC(I);"BOBBIN RULEZ!"

]30 NEXT I

]RUN
 BOBBIN RULEZ!
  BOBBIN RULEZ!
   BOBBIN RULEZ!
    BOBBIN RULEZ!
     BOBBIN RULEZ!

]^D
$
```

### Run a BASIC program and save its output

```
$ cat > input
10 FOR I=1 TO 3
20 ? SPC(I);"HIYA!"
30 NEXT I
RUN
^D
$ ./bobbin -m plus < input > output # OR: -i input -o output
$ cat output
 HIYA!
  HIYA!
   HIYA!

$
```

### Read commands / BASIC programs from a file, and then remain running

```
$ ./bobbin -m plus -i input --remain

[Bobbin "simple" interactive mode.
 Ctrl-D at input to exit.
 Ctrl-C *TWICE* to enter debugger.]
 HIYA!
  HIYA!
   HIYA!

]LIST

10  FOR I = 1 TO 3
20  PRINT  SPC( I);"HIYA!"
30  NEXT I

]^D
$
```

Also try `--remain-tty` instead, for the same thing but with full text-screen emulation.

### Tokenize/detokenize BASIC programs

```
$ cat input
10 FOR I=1 TO 3
20 ? SPC(I);"HIYA!"
30 NEXT I
$ ./bobbin --tokenize < input > output
./bobbin: Tokenized data written to standard output.
$ hexdump -C output
00000000  0c 08 0a 00 81 49 d0 31  c1 33 00 1d 08 14 00 ba  |.....I.1.3......|
00000010  c3 49 29 3b 22 48 49 59  41 21 22 00 24 08 1e 00  |.I);"HIYA!".$...|
00000020  82 49 00 00 00                                    |.I...|
00000025
$ file output
output: Applesoft BASIC program data, first line number 10
$ ./bobbin --detokenize < output
10  FOR I = 1 TO 3
20  PRINT  SPC( I);"HIYA!"
30  NEXT I
$
```

### Loading a program into memory

#### Load tokenized AppleSoft to be available in the emulator

```
$ ./bobbin --tokenize < input > tokenized
$ ./bobbin --load-basic-bin tokenized -m plus
```
(try typing `LIST` or `RUN`)

#### Load a binary file

```
$ hexdump -C hello.bin
00000000  a2 00 bd 0e 03 f0 06 20  ed fd e8 d0 f5 60 c2 cf  |....... .....`..|
00000010  c2 c2 c9 ce a1 8d 00 00                           |........|
00000018
$ ./bobbin -m plus --simple --load hello.bin --load-at 300

[Bobbin "simple" interactive mode.
 Ctrl-D at input to exit.
 Ctrl-C *TWICE* to enter debugger.]

]CALL 768
BOBBIN!

]
$
```

#### Load and automatically run a binary file

```
$ hexdump -C hello.bin
00000000  a2 00 bd 10 03 f0 06 20  ed fd e8 d0 f5 4c 03 e0  |....... .....L..|
00000010  c2 cf c2 c2 c9 ce a1 8d  00                       |.........|
00000019
mcowan$ ./bobbin -m plus --simple --load hello.bin --load-at 300 --start-at 300 --delay-until INPUT

[Bobbin "simple" interactive mode.
 Ctrl-D at input to exit.
 Ctrl-C *TWICE* to enter debugger.]

]BOBBIN!

]^D
```

## Command Line and Options

<!-- The Synopsis is used to generate bobbin --help text; -->
<!-- be careful about editing it! -->
### Synopsis

**bobbin --help**<br />
**bobbin -m** *machine* \[ *options* \]<br />

### Options

Note: **bobbin**'s option parser treats **--option** and **-option** the same. Options that take value arguments can either give the option in one argument, and the value in the next, or else it can be specified all in one, like **--option=value**. Consequently, **bobbin** does *not* suport stringing a series of short options together, like **-cfb** to mean **-c**, **-f**, **-b**.

<!-- Note: if you change anything in the Options section, it will AFFECT PROGRAM BEHAVIOR! -->
<!--       this README.md is parsed for some of the information about program options.     -->
<!--       Only the first paragraph of each option's description gets copied to --help     -->
<!--START-OPTIONS-->
#### Basic Options

##### -V, --version

Display version info and exit.

##### -h, --help

Display basic information on how to use **bobbin**, and exit.

##### -v, --verbose

Increase **bobbin**'s informational output. Use again for even more.

Normally **bobbin** only produces information about program-ending errors, or warnings about configuration or situations that might lead to unexpected operation.

##### -q, --quiet

Don't produce warnings, only errors.

##### -i, --input *arg*

Input filename. `-` means standard input (and is usually redundant).

##### -o, --output *arg*

Output filename. `-` means standard output (and is usually redundant).

#### Essential Options

##### -m, --machine, --mach *arg*

The machine **bobbin** should emulate.

This is currently a required argument, to tell **bobbin** what type of Apple \]\[ it should be. It *does* have a default value (emulating an enahnced IIe, or `//e`), but that machine is not yet supported, and so you must choose something else&mdash;either `original` or `plus` (or their equivalents). See [Choosing what type of Apple to emulate](#choosing-what-type-of-apple--to-emulate), below.

Valid values are:
<!--MACHINES-->
 - `original`. Aliases: `][`, `II`, `two`, `woz`, `int`, `integer`.<br />
   Selects an original, non-autostart Apple \]\[ with Integer BASIC loaded. The `simple` interface boots directly into Integer BASIC (unlike the real machine); all other interfaces boot directly to the system monitor (actual machine behavior).
 - `plus`. Aliases: `+`, `][+`, `II+`, `twoplus`, `autostart`, `applesoft`, `asoft`.<br />
   Selects an Apple \]\[+ (or, equivalently an original Apple \]\[ with the autostart, AppleSoft BASIC firmware). Boots into AppleSoft.
 - `twoey`. Aliases: `][e`, `IIe`.<br />
   An unenhanced Apple \]\[e.

Note: some shells (notably zsh) will not allow you to type `][` as a bare string in a command invocation, you most wrap it with single-or-double-quotes (in which case it's probably easier to use one of the other aliases instead.)

Planned future values:
 - `enhanced`. Alias: `//e`.<br />
   The enhanced Apple //e. This will be the default `--machine` value, in future.
<!--/MACHINES-->

Note that while the Apple \]\[e provides a somewhat more powerful environment, Apple \]\[+ may be a more convenient choice in some cases, as neither \]\[+ nor \]\[e accept BASIC commands in lowercase, but since Apple \]\[e actually *has* lowercase characters, automatic conversion to uppercase is not performed on input, whereas on Apple \]\[ and \]\[+ (which have no lowercase), all input is automatically converted to uppercase.

##### --if, --interface, --iface *arg*

Select the user interface.

If standard input is from a terminal, then the default value is `tty`, which provides a full, in-terminal display of the emulated Apple \]\['s screen contents; if standard input is coming from something else (file or pipe), the `simple` interface, which uses a line-oriented I/O interface, is used instead.

##### --simple

Alias for `--interface=simple`.

##### --disk, --disk1 *arg*

Load the given disk file to drive 1.

If neither `--disk` nor `--disk2` are used, then a disk controller card is not included in the emulated machine, causing it to boot immediately into BASIC. This differs from many emulators, which include a disk controller card even when no disks are inserted, causing the boot to hang forever until Ctrl-RESET breaks out from the boot.

##### --disk2 *arg*

Load the given disk file to drive 2.

#### Special options

##### --watch

Watch the `--load` file for changes; reboot with new version if it does.

If used in combination with `--delay-until-pc` (see below), `--watch` ensures that the machine is rebooted with, once again, a cleared (garbage-filled) RAM, and will wait, once again, for execution to reach the designated location, before reloading the RAM from `--load` (and jumping execution to a new spot, if `--start-loc` was specified (see below).

(This feature currently only works via the Linux inotify API. A fallback method is planned, for when inotify is not available.)

##### --tokenize

Reads in a AppleSoft BASIC listing, outputs AppleSoft tokenized binary.

Expects AppleSoft BASIC on the standard input (or whatever you specified with `-i`), and will output the tokenized binary version. **Bobbin** tokenizes the output by running the input through an emulated Apple machine, and then when input has ended, saving the program at `$801` to the output file (specified with `-o`).

If a line that doesn't begin with a number is entered, or AppleSoft gives an error on line input, **bobbin** will exit wtih an error.

**Bobbin** will refuse to run with this option if it detects that output is directed to a tty.

##### --detokenize

Reads in a tokenized BASIC binary, and outputs the program listing.

Like `--tokenize`, uses an emulated Apple to detokenize the file, and then runs the `LIST` command to get text back out of it, except that the `LIST` output is modified so that long lines aren't broken up into multiple.

#### Machine configuration options

##### --no-bell

Suppress your terminal bell from playing when the Apple issues a beep. Only meaningful in the `tty` interface (`simple` does not sound a bell).

##### --turbo, --no-turbo

Run as fast as possible - don't throttle speed to 1.023 MHz.

This is the default when the interface is `simple`, and you may use `--no-turbo` to disable it in that mode. By default, the `tty` interface runs at (approximately) normal Apple \]\[ speed.

##### --no-lang-card

Disable the language card.

##### --rom-file *arg*

Use the specified file as the firmware ROM.

The file size must exactly match the expected size for the emulated machine. For `original` and `plus`, that means exactly 12kib. For `twoey` it means 16kib, and for `enhanced` it's 32kib.

##### --no-rom

Refrain from loading a ROM file into the upper address space.

When this option is specified, `--load` *arg* must be used as well (else there is no code to be run), and the `-m`/`--machine` option is no longer required.

Note that this option makes it possible to programatically change the contents of the `NMI`, `INT`, and `RESET` vectors at the extreme end of addressable space.

##### --load *arg*

Load the specified binary file into RAM.

When this option is specified, the given file will be loaded into the emulated machine RAM. It will be loaded starting at the memory location given to the `--load-at` argument, or `$0000` if that option was not provided.

Note that, if the loaded file overlaps ROM space (`$D000` thru `$FFFF`), the RAM in that space will not be accessible, unless the `--no-rom` option was given, or appropriate soft switches are used to grant access to RAM at that space (not yet implemented). The same is true if `--ram` is used to limit the amount of available RAM below the end of the loaded contents (the file can't be loaded into RAM that isn't there!). If a file is loaded across the region reserved for soft switches and slotted peripherals, `$C000` thru `$CFFF`, it is diverted to an alternate RAM bank (the so-called "first 4k bank") at `$D000` thru `$DFFF`.

A file's contents are mapped into RAM pretty much as one would expect: if a file is loaded into the start of RAM, at `$0000`, then all the locations in the file will correspond to the same loctions in RAM, *with the following exceptions*:

- Anything past position `$FFFF` in the file will be mapped into Auxiliary RAM, wrapping around again at memory location `$0000` of auxiliary RAM. Thus, file position `$1000` corresponds to location `$1000` in the "main" RAM, while file position `$11000` corresponds to location `$1000` in the "auxiliary" RAM bank. Note that auxilliary RAM is not accessible within the emulation, if the selected machine type is original \]\[, or \]\[+.
- File locations `$C000` thru `$CFFF`, and `$1C000` thru `$1CFFF`, are *not* mapped into their corrsponding locations in addressable memory as described in the previous bullet point, since RAM is never mapped to those locations (they are reserved for slotted peripheral firmware). They are instead mapped to the "alternate" 4k RAM banks ("main RAM bank 1" and "auxilliary RAM bank 2", respectively), which are accessed at region `$D000` thru `$DFFF` (but *only* when appropriate soft switches have been activated).

If the file's contents would exceed the end of even auxilliary memory (128k), **bobbin** will exit with an error.

##### --load-at, --load-loc  *arg*

RAM location to load file from `--load`.

The *arg* must be a hexadecimal 16-bit value, optionally preceded by `$` or `0x`.

##### --load-basic-bin *arg*

Load tokenized (binary) AppleSoft BASIC file at boot.

This option is effectively the same as `--load `*arg*` --load-at 801 --delay-until INPUT`, except that it does some additional "fixup" to connect it to the BASIC interpreter (to tell it where the program start and end are).

##### --start-at, --start-loc

Specify an initial start position in place of what's in the reset vector.

##### --delay-until-pc, --delay-until *arg*

Delays the effects of `--load` (RAM) and `--start-at`, until PC is *arg*.

The primary intent of this option is to allow the Apple \]\[ to run through its usual boot-up code, and then load new contents into memory and jump to a new, post-boot-sequence "start". This makes it easy to boot the emulator into your favorite code, without involving a disk-load.

The special mnemonic *arg* value `INPUT` (case-insensitive) is translated to location `FD1B` (the monitor `KEYIN` routine for reading from the keyboard), which is frequently a very convenient value to use.

##### --ram *arg*

Select how much RAM is installed on the machine, in kilobytes.

The default is the maximum possible amount normally possible (without third-party additions or modifications) on the specified machine&mdash;64k for the \]\[ or \]\[+, 128k on the others. The first 16k over 48k (totalling 64k) will be used as memory in an emulated "Language Card" (memory that could be used in case of the BASIC ROM firmware, to load an alternative "built-in" language to the one installed. If you specify 128k, then the top 64k of that will be used for so-called *auxiliary memory*, used for things like 80-column text support, "doubled" low-res and high-res graphics, and the ProDOS `/RAM` disk.

Acceptable values are: 4, 8, 12, 16, 20, 24, 32, 36, 48, 64, or 128. The values 28, 40, and 44 will also be permitted, but a warning will be issued as these were not normally possible configurations for an Apple \]\[. Above 48, only 64 or 128 are allowed.

#### "Simple" interface options

##### --remain

Go interactive after input is exhausted.

This option applies when **bobbin**'s input has been redirected, say from a file or pipe, and is not connected to the terminal. **Bobbin** ordinarly would quit once that input is exhausted, but if the `--remain` option is specified and **bobbin** is on a terminal, it will reconnect input through the terminal and continue executing with the `simple` interface.

##### --remain-tty

Go interactive after input is exhausted, *and* switch to `tty` interface.

Same as `--remain`, except that after input has been exhausted, the display is switched to the full Apple \]\[ display emulation (the `tty` interface).

##### --simple-input *arg*

`apple` (default), or `fgets`.

See [](#choosing-your-flavor-of-input-mode-with---simple-input) below, for copious details about this option. The TL;DR of it is that `fgets` uses your terminal's (primitive) line-input processing when **bobbin** detects that the Apple \]\[ wants to read a line of input (as opposed to a single character), and `apple` (the default) defers to the emulated Apple computer's (equally primitive, yet distinctly different) line-input processing. 

This option has no effect while **bobbin** is not reading from a terminal.

#### Diagnostics, Debugging, and Testing Options

##### --die-on-brk

Exit emulation with an error, if a BRK or illegal opcode is encountered.

##### --breakpoint, --bp *arg*

Set a debugger breakpoint (`simple` interface only).

##### --trace-to *m*\[:*n*\]

Trace N instructions before/including M (default N = 256).

This feature is intended for use with the `--trap-failure` feature. If the failure trao is triggered, it will output the instruction number where it did. You can then plug that number into this option, followed by a colon and the number of instructions *before* the number you gave, that should be logged to the trace file (`trace.log` by default; adjustable with `--trace-file`). As an example, if the failure traps at instruction *12564*, then `--trap-failure 12564:100` would start logging instructions, register values, and (explicit) memory accesses starting at instruction *12465*. If instruction *12564* is reached, the trace logging ends. If instruction *12564* does not trigger the failure trap, then execution will still continue, but no more information will be output to the trace-log file.

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

##### --trap-print *arg*

Print the ASCII char in ACC and return, if execution reaches this location.

Unlike the other two `--trap-*` options, this does not cause **bobbin** to exit; only to print a character. Only works in the `simple` interface.

<!--END-OPTIONS-->
### Choosing what type of Apple \]\[ to emulate

The eventual plan is for **bobbin** to emulate an enhanced Apple //e as the default, but this is not yet supported. In order to prevent future confusion when the Apple //e does become a supported machine type, **bobbin** does not default to amy of the machine types that *are* currently supported: you must explicitly select the machine type via the `-m` switch.

The machine types that are currently supported are: an Apple \]\[, an Apple \]\[+, or an unenhanced Apple \]\[e. Since an (early) Apple \]\[+ is exactly equivalent to the original Apple \]\[ with the "autostart" ROM with AppleSoft (floating-point) BASIC built-in, the term "original Apple ][" is used here to mean that the original, "non-autostart" ROM that starts directly in the monitor program, and which included Integer BASIC, the ROM known as "Programmer's Aid #1", and the original mini-assembler that included the later-removed "trace" and "step" features. (The mini-assembler is entered by typing `F666G` (not `!`) from the monitor, and exited via `RESET`&mdash;note that the "monitor" we are referring to here is a program that is built into the Apple \]\[ ROM; we are not referring to a hardware display device in this context.) As a special feature of the `--simple` interface, **bobbin** does not actually drop you into the monitor with `-m \]\[ --simple`; instead it jumps you into Integer BASIC, as that's the more likely thing you might want to pipe a program into (you can still get to the monitor via `CALL-151`, of course!).

Note, too, that the Apple \]\[ and Apple \]\[+ did not have lowercase characters, and so **bobbin** automatically translates lowercase typing into uppercase, for these two machines. However, the Apple \]\[e and later *does* support lowercase typing, so the translation is not performed. Even though the unenhanced Apple \]\[e supports lowercase characters, it will not recognize lowercase BASIC commands (unlike the Enhanced Apple //e, which does). For this reason, if you are primarily running *bobbin* to type in BASIC programs, we recommend you emulate a \]\[+ rather than a \]\[e, unless your program needs to print or accept lowercase characters (or there is some other reason you need an Apple \]\[e).

To start **bobbin** in AppleSoft BASIC, emulating an Apple \]\[+, use `-m plus`, or one of its aliases (see [the description of the `--machine` option](#-m---machine---mach-arg), above).

To start **bobbin** in the system monitor, with Integer BASIC available via Ctrl-B, use `-m original`, or one of its aliases (see [the description of the `--machine` option](#-m---machine---mach-arg), above). Note that the `--simple` interface (the only interface currently available in **bobbin**) actually drops you directly into Integer BASIC, instead of the monitor. While this behavior is unrealistic to the original machine, it is thought to be more convenient for the purpose of redirected input (as you don't have to start your files with a Ctrl-B character).

And finally, to start **bobbin** as an Apple \]\[e, use `-m twoey` or `-m iie` (or another alias, as given above).

## User Interfaces (--iface)

### The "tty" interface

The `tty` interface is used by default when you start **bobbin** on the command-line&mdash;unless standard input is being redirected from a file or pipe (or is otherwise not connected to a terminal). It provides a display of the actual emulated machine's screen contents, inside your terminal, using the standard Unix **curses** (or **ncurses**) library.

While in the `tty` interface, you can type Ctrl-C twice in a row to enter command-entry mode for **bobbin**. If you need to quit **bobbin**, you can type `q` followed by Enter from the command-entry mode. Type `help` and Enter to see some other commands you can use. The `help` message will only stay on your screen for ten seconds&mdash;but if you re-enter command mode again while the `help` text is still on the screen, it will stay there until you leave command mode again (by typing another command, or simply Enter by itself).

Similarly, any warning messages will only display for two seconds before disappearing&mdash;if you need more time to read a message on the screen, type Control-C twice to enter command mode and give yourself more time to read. At some future point we will probably keep a backlog of such messages, and provide a command for viewing it. In the current version of **bobbin**, however, once a message disappears it cannot be reviewed.

The `tty` is a few steps closer to "realistic" than the strictly line-oriented `simple` interface, and is adequate for most strictly-textual software that runs on 8-bit Apple machines. However, there are important drawbacks and caveats:

 - In 40-column mode, the screen has a strange aspect-ratio on most terminals. This is because typical terminal emulators use fonts that look good at 80 columns by 25 lines, and so are half the width needed to make 40 columns look "full".
 - Only ASCII (and Unicode) characters are available. This means that the various "mousetext" characters available from the enhanced Apple //e and onwards, are not available for display on a Unix terminal. Actually, Unicode *does* include the Apple mousetext characters as part of the standard, but as of now they are not widely implemented, so no attempt is currently being made for **bobbin** to use them at the terminal interface.
 - This interface (as well as the `simple` one) will remain unable to support the often-important "any key" functionality of the keyboard strobe software switch. present in the Apple \]\[e. The reason being that Unix-style terminals do not commonly have a way to detect if a key is currently being pressed, so we don't have this information to pass along to the Apple \]\[e. Therefore, if a piece of software (usually a game), perhaps after detecting that it's running in an "Apple \]\[e", depends on this functionality, it will not behave correctly under the `tty` interface. Games intended to work on earlier Apple models are fine, since they don't expect to be able to detect whether a key is down (typically, older games feature movement that continues until a different key is pressed, rather than checking for a key to be released).

### The "simple" interface

The `simple` interface is accessed via the `--iface simple` (or just `--simple`) option, and also is the default when standard input is not from the terminal (e.g., file or pipe). It provides a line-oriented interface, and is largely similar to the experience of using an Apple \]\[ over a serial connection: cursor position is ignored, and characters are printed sequentially. The cursor may be backed up if the user, or the emulated program, types a backspace character, but the cursor will never travel upwards on the screen.

There are two "input modes" for the `simple` interface (`apple` and `fgets`), which select how input behaves when **bobbin** detects that the emulated program desires a full line of input. See below for more detail.

#### Interactive "simple" interface

The most straightforward way to use the `simple` interface is interactively. If you run **bobbin** like:

```
$ ./bobbin -m plus --simple
```

You will immediately be dropped into an AppleSoft BASIC (`]`) prompt, and **bobbin** will inform you to type Ctrl-D at an input prompt when you wish to exit. This is a great mode for just messing around with BASIC on an Apple \]\[, or to "check something real quick" about how things work with an Apple \]\[ machine.

#### Non-interactive, input-redirected "simple" interface

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

#### Mixed-interactivity via `--remain`

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

There is also the `--remain-tty` option, which does the same thing as `--remain`, except after reading your BASIC program or commands at input, it switches to the full "screen emulation" provided by the `tty` interface (instead of remaining at the line-oriented `simple` interface).

#### Choosing your flavor of input mode with `--simple-input`

There are currently two different "input modes" available for the `--simple` interface to use when it detects that a line of input is being requested during an interactive user session, selected by the option `--simple-input`. This option has zero effect on any behavior while **bobbin** is reading input from a pipe or a file—or anything that isn't a terminal. It also has no effect on how input is handled when a program running on the Apple is reading a single keypress—nor when an unrecognized routine is being used to fetch an input line. This is because **bobbin** cheats to detect "line" input, by knowing where in the firmware the line-input routine lives—if that's not the routine being used to fetch input, **bobbin** can't tell the difference between fetching a keypress or fetching a whole line of input.

##### `--simple-input apple`

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

##### `--simple-input fgets`

*(Warning, the developer reserves the right to remove this mode in the future, unless someone informs him that they love and vastly prefer this option.)*

The other choice, is to let your terminal handle the line input, instead of letting the emulated Apple machine handle it. That's what `--simple-input fgets` does. In this mode, the terminal you are typing from is placed in what Unix terminal documentation likes to call "canonical mode": special characters are treated specially, and input is read as a full line, before the program under control (**bobbin**, in this case) is given a single character of it. This has the advantage of making the experience feel rather more like a "native command-line app" than it does in `apple` mode.

Note that this mode does *not* allow you to use your favorite emacs-style keys, just like you do at your shell prompt. It's more like (actually, exactly like) how things work when you type a line of input into the Unix `cat` program. You can type characters, you can backspace over them, you can Ctrl-W to delete the last word, and you can use Ctrl-U to erase and retype the entire current line. (Note: Ctrl-W and Ctrl-U do not appear to work on WSL (Windows Subsystem for Linux), which **bobbin**'s developer discovered to their annoyance whilst developing this program chiefly on Ubuntu-in-Windows).

None of the characters that are typed into the line are sent to the emulated Apple, until the entire line has been typed, and sealed with a ~~kiss~~ carriage return.

While `fgets` mode cannot properly be described as "comfortable" to a Unix user, it may well be that it is at least *more* comfortable than the Apple \]\['s approach to line input, which is often regarded as surprising and burdensome—though the primary cause of that is that in some Apple machines there is a "Delete" key where one might expect a "Backspace" to be, and if one uses that key in hopes that it does in fact "Backspace", they are sorely confused. (An Apple's "Backspace" key is the same key as "left arrow", and is usually at the bottom of the keyboard rather than the top.) This point of confusion does not apply to **bobbin**, because it makes sure to translate the Delete character (that your terminal's Backspace key probably generates) into the backspace (left-arrow) character before sending it to the Apple.

However, one remaining point of confusion that persists in **bobbin**'s `--simple-input apple` mode, is that control characters that have been typed will not echo to the screen, and yet they have still been input, and will produce `SYNTAX ERRORS` or the like. This is less likely to happen with `fgets` mode.

Of course, for a user to be fully as comfortable with inputting lines in **bobbin** as they are at their shell prompt, one would need to use a fully-featured line editor, such as GNU's `readline` facility. Perhaps one day **bobbin** will have a `--simple-input readline` feature? (That ooccurrance would probably hasten the obsolescence of this `fgets` feature).

When the line is done being typed, and is ready to send as input to the emulated Apple \]\[, **bobbin** turns on "output suppression" while the Apple is busy reading the characters. Once the Apple has slurped up the final carriage return from the line input, **bobbin** suppresses the very next character that's output (normally that same carriage return), and then re-enables output. This step is necessary because without the output suppression, the Apple would immediately and redundantly echo the line back out at you, because it has no way to know those characters are already appearing on your screen.

Note that the characters you type in `fgets` mode are not the actual characters the Apple \]\[ will see. Any lowercase characters are converted to uppercase, because an unmodified Apple \]\[ did not have lowercase characters, nor any means to type them. This probably won't surprise you, but what *may* surprise you is that some of the punctuation you type may be similarly transformed, as well. The original Apple \]\[ did not have the punctuation characters `{|}~` and \`; these are swapped out for `@[\\]` and `^`. Note that these same transformations are performed on piped input as well, and in `--simple-mode apple` processing (where it is immediately visible when entered).

##### Non-interactive line input

When **bobbin** detects that a line of input is being fetched, it suppresses output while that line is being read (just as described at the end of [the `--simple-input fgets` section](#--simple-input-fgets) just above. This is to support a familiar interface similar to other standard I/O tools: input comes into a program, output comes out. If output weren't suppressed during line input, the Apple would echo it back in the output. If you pipe a thousand-line BASIC program into **bobbin**, you'll probably be annoyed if you have to wade through those thousand lines you already had, just to get to the "good part" (the BASIC program's output).

For the same reasons, during non-terminal input, the `simple` interface also suppresses prompt characters and preceding newlines, that come just before line input is requested. Just as you don't want to see a thousand lines of BASIC code you just sent get thrown back at your face, you also don't want to see the thousand prompt characters that were issued, either. This "prompt suppression" is not performed during the other (selectable) input modes.

There is no option to control the input-mode used for non-interactive input. The developer's in-my-head name for it is "pipe" mode.

##### How the `--simple` interface distinguishes line-input from keypresses

**Bobbin** watches the Apple \]\['s 6502 CPU register called the PC (program counter) register, which tracks the location in memory where the Apple is currently executing code. If it reaches a place in the firmware that **bobbin** knows corresponds to a particular place in the Apple \]\[ firmware "monitor" program known as `GETLN`, then the specialized "line input" behavior is triggered. The locations used to match against the PC to determine if `GETLN` is running, are `$FD75` (`NXTCHAR`), `$FD67` (`GETLNZ`), and `$FD6A` (`GETLN`). The latter two locations are only used in non-interactive ("pipe") mode, to suppress prompt-printing, while the first location is used both to suppress output in non-interactive mode, and also to trigger "fgets" line input mode (when `--simple-input fgets` is specified). If the non-autostart, Integer BASIC firmware is loaded, an additional location is used for prompt-detection on the program counter: `$E006`.

Character output is detected when the program counter reaches `$FDF0` (`COUT1`). When output is not being suppressed, this triggers the character found in the accumulator register to be printed to **bobbin**'s output stream (after suitable transformation, from Apple \]\['s character encoding scheme to ASCII). Output suppression *in no way* affects whether, or in what way, a character is printed *interanlly* to the Apple \]\['s video memory.

While one might be tempted to think that character input probably mirrors how character output is done&mdash;matching the program counter against a memory location like `$FD1B` (`KEYIN`), it is actually detected in a completely different way: by trapping memory reads from special memory locations `$C00x` and `$C01x` (where `x` here represents any hexadecimal nybble value). The former location is used by an Apple \]\[ program to read whether a character is available to be read, and what its value is, while the `$C01x` memory is used just to tell the Apple \]\[ key-handling hardware that we've accepted the keypress, and that it shouldn't set the "key-is-ready" bit at `$c00x` until the user presses another key. **Bobbin** needs to use this method for providing user input, rather than checking for entry into a subroutine, because many, many programs use that method directly to read keypresses, and if **bobbin** didn't handle them, those programs might loop forever, always waiting for a keypress that **bobbin** would never know to deliver.

##### Additional details of bobbin's I/O processing

If **bobbin** receives a `SIGINT` signal (often triggered by a user typing Ctrl-C), **bobbin** treats it exactly as a normal Ctrl-C keypress, except that if there is some input that **bobbin** has received but which hasn't yet been processed by the program running on the Apple \]\[, the Ctrl-C may jump to the front of the line, ahead of other characters that had actually been typed before it. Note that it is possible to configure a terminal to use some other keypress instead of Ctrl-C for sending `SIGINT`; in this case **bobbin** will still treat that character as if it had been a Ctrl-C.

If **bobbin** receives a second `SIGINT` signal before the emulated Apple has had a chance to consume it as a keypress; or if Ctrl-C is pressed twice in a row without any intervening characters (even if the first Ctrl-C was in fact processed successfully as input to the emulated Apple), then **bobbin** will open a debugger interface. This interface enables you, among other things, to type the "reset" key (performing either a "warm" or "cold" reset), to jump directly into the Apple monitor, or to step through the currently-running machine code.

If **bobbin** is processing non-interactive input while a `SIGINT` is received, and the `--remain` option is active, it will do three things in succession: (1) it will send a Ctrl-C to the program on the Apple \]\[ (in BASIC, this usually has the effect of causing a program `BREAK`); (2) it will discard any pipe or redirected input that was still waiting to be processed by the Apple \]\['s program, and (3) it will immediately enter "interactive" mode, and process line inputs according to whatever setting of `--simple-input` may be active.

In the `--simple` interface, if a Ctrl-D is input by the user, then it indicates that **bobbin** should exit (**bobbin** may see a Ctrl-D entered directly, or it may "percieve" it via a return from `read()` that indicated no bytes remained to be read). **Bobbin** does not exit *immediately* upon receipt of a Ctrl-D. Instead, it waits until the program in the Apple \]\[ has caught up with the user input, up to the point when the Ctrl-D was typed, and issues a carriage return character. Only when the Apple \]\[ program indicates "consumption" of that carriage return, does **bobbin** then exit. This is to make the behavior consistent with other programs at a terminal.

### Bobbin's built-in debugger

The debugger is currently only available from the `simple` interface, though availability in some form is planned for the `tty` interface as well. It is entered from `simple` by typing Ctrl-C twice in succession (**note:** doing this in the `tty` interface gives you a command interface, but not the full debugger interface).

When you enter the debugger, you'll see a message like the following:

```
*** Welcome to Bobbin Debugger ***
  SPC = next intr, c = leave debugger (continue execution), m = Apple II monitor
  q = quit bobbin, r or w = warm reset, rr = cold reset
-----
ACC: A0  X: 00  Y: 05  SP: F6          [N]   V   [U]   B    D    I    Z    C
STK: $1F6:  (FE)  3A  F5  84  FF  F8  E6  00  85  E8  |  FF  FF  00  00
0300:   B1 28       LDA ($28),y      28: D0 07   07D5:  A0 A0 A0 A0 A0
>
```

As the introduction states, you can type `c` to get back out of the debugger. You can type `r` or `w` to perform a "warm reset" (same as Ctrl-RESET on a real machine), or type `rr` to perform a "hard reset" (same as Open-Apple Ctrl-RESET on a real machine). Bobbin doesn't currently emulate the Open-Apple key, so it just directly invalidates the "powered-up"/warm-reset verification byte before issuing the reset.

You can also jump to the system monitor by typing `m`. In order to encourage the monitor to print the value of the PC and the other registers on entry, `m` actually simulates the `BRK` signal (which occurs when the 6502 `BRK` instruction (`$00`) is executed).

Taking a closer look at this portion of output from the debugger:

```
ACC: A0  X: 00  Y: 05  SP: F6          [N]   V   [U]   B    D    I    Z    C
STK: $1F6:  (FE)  3A  F5  84  FF  F8  E6  00  85  E8  |  FF  FF  00  00
0300:   B1 28       LDA ($28),y      28: D0 07   07D5:  A0 A0 A0 A0 A0
```

You can see that the values of the accumulator (A or ACC), the index registers X and Y, and the Stack Pointer, are printed first. Following that is a breakout of the processor flags register. Any flags enclosed in `[`square braces`]` are "set" (bit value 1); any that aren't are "reset" (or "unset"&mdash;bit value 0).

Next, a slice of the contents of the stack are displayed. The value at the current Stack Pointer (in this example, `$F6`, so this is the value at location `$1F6`) is printed first, in parentheses to indicate that this value doesn't have meaning right now. It is followed by the values at increasing location, the items that have been "pushed" onto the stack, from most- to least-recently-pushed. If you see a `|` bar, it indicates wraparound between the value at `$1FF`, to the value at `$100`.

On the third line, we have the Program Counter value, followed by the opcode and any arguments at that location, a disassembly of those, and finally any memory locations of interest. Since the `LDA ($28),y` instruction first looks at the 16-bit word starting at location `$28`, and then adds the value from the Y index register to that before reading a value from the resulting memory location, we first see the two bytes located at `$28`&mdash;`$D0` and `$07`, which (in the Apple \]\['s little-endian environment) represents memory location `$0705`&mdash;followed by the memory contents at `$07D5`&mdash;or, `$07D0` plus the contents of the Y register (5). The first of those values at `$07D5` are what will be stored in the accumulator by this `LDA` instruction.

You can step through successive lines of code by simply hitting return. As the debugger is still in early stages of development (like the rest of **bobbin**, there isn't much else to do in the debugger right now.
