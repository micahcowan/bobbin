# BOBBIN (for Apples!)

[Follow me at mastodon.social](https://mastodon.social/@micahcowan) for new feature announcements. Posts related to **bobbin** will begin with the tag "`BOBBIN: `".
<hr/>

**Bobbin** is a "highly hackable" Apple \]\[ emulator, aimed especially at improving productivity for devs of Apple \]\[ software by offering convenient terminal-based interface options, redirectable "standard input and output"-oriented options, and custom-scripted, on-the-fly adjustments to the emulated machine.

[Click below](https://youtu.be/zMlG5CRfRDA) to watch an out-dated demonstration of an early version (roughly two days into development) of the pipe-able, standard I/O emulator interface (dubbed the "simple" interface):

**Please note:** the video shows the use of command-line option `-m ][`. In the latest version of the **bobbin**, you must use `-m plus` (or `-m ][+`, but some shells don't allow those characters to appear unquoted), to obtain approximately the same results as shown in the video. If you use `-m ][` as shown in the video (and if your shell even allows that), instead of the correct `-m plus`, then **bobbin** will drop you directly into *Integer* (Woz) BASIC, instead of AppleSoft BASIC (and the examples from the video would no longer work).
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
- Supports running AppleSoft programs as standalone Unix programs, via `#!/path/to/bobbin --run-basic`

### Planned features

- `.woz` disk format, and 13-sector support
- Full graphics (not to the terminal) and sound emulation, of course
- Emulate an enhanced Apple //e by default
- Scriptable, on-the-fly modifications (via Lua?) to the emulated address space and registers, in response to memory reads, PC value, external triggers...
- Use **bobbin** as a command-line disk editor, using real DOS or ProDOS under emulation to change the contents of a disk image
- Use **bobbin** as a command-line compiler or assembler, by loading in your sources, running your favorite assembler (or what have you) in the emulated Apple, and then save the results back out

### Known issues

- No way to use the open-apple or closed-apple keys (coming soon, but it'll likely have to be awkward).
- If booted without a disk operating system, the BASIC `SAVE` command is useless (for now), and `LOAD` will proceed to hang the emulation, requiring you to send a reset (from the Ctrl-C Ctrl-C command interface). Same for the equivalent monitor commands, `R` and `W`.
- The Ctrl-C Ctrl-C command parser is crap and needs to be rewritten, and should probably use libreadline (or libedit).
- If the `:disk NUM load` command fails (even due to a typo), the emulator quits, instead of just printing an error message.

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
mcowan$ ./bobbin -m plus --simple --delay-until INPUT --load hello.bin --load-at 300 --jump-to 300

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

Increase **bobbin**'s informational output. Can specify multiple times.

Normally **bobbin** only produces information about program-ending errors, or warnings about configuration or situations that might lead to unexpected operation. Specifying -v once increases the number of informational messages, and a second -v makes things quite verbose. A third -v causes low-level debug information to be included.

##### -q, --quiet

Don't produce warnings, only errors.

##### -i, --input *arg*

Input filename. `-` means standard input (and is usually redundant).

##### -o, --output *arg*

Output filename. `-` means standard output (and is usually redundant).

##### --run-basic *arg*

Similar to *-i*, but more suitable for running BASIC progs via #!.

A `RUN` is appended after the file's contents are read. Leading comment
lines are ignored. All lines must start with a number.
Standard input can be read by the BASIC program. If the BASIC program
exits, so does **bobbin**.

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

If neither `--disk` nor `--disk2` are used, then a disk controller card is not included in the emulated machine, causing it to boot immediately into BASIC. This differs from many emulators, which include a disk controller card even when no disks are inserted, causing the boot to hang forever until Ctrl-RESET breaks out from the boot. You can still insert a disk later, via the `:disk` command in the command interface (Control-C twice), in which event a disk controller card will magically appear in the system!

The currently-supported disk format types are: `.nib`, `.dsk`, `.do`, and `.po`. No attempt is made at "detecting" the format of a `.dsk` file, it is always assumed to be DOS-ordered (rename it to `.po` if it's not). Only 5.25", 16-sector formats are supported at this time.

Any changes written to disk are synced to the underlying file when the disk-drive motor stops spinning. The current implementation syncs the entire file to disk, even if only a small portion was written. Write-protected disk image files are not yet supported.

##### --disk2 *arg*

Load the given disk file to drive 2.

##### --hdd *arg*

Load the given file as a SmartPort hard disk drive image.

May be specified up to four times.

The image must be a multiple of 512 bytes in size. The `--watch` option is not
honored for hard disk images.

#### Special options

##### --watch

Watch the `--load` and/or `--run-basic` files for changes; reboot with new version if it does.

If used in combination with `--delay-until-pc` (see below), `--watch` ensures that the machine is rebooted with, once again, a cleared (garbage-filled) RAM, and will wait, once again, for execution to reach the designated location, before reloading the RAM from `--load` (and jumping execution to a new spot, if `--start-loc` was specified (see below).

Future versions of **bobbin** will also allow `--watch` to reboot for disk image changes, in addition to the `--load` argument.

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

Suppress your terminal bell from playing when the Apple issues a beep.

Only meaningful in the `tty` interface (`simple` does not sound a bell).

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

- Anything past position `$FFFF` in the file will be mapped into Auxiliary RAM, wrapping around again at memory location `$0000` of auxiliary RAM. Thus, file position `$1000` corresponds to location `$1000` in the "main" RAM, while file position `$11000` corresponds to location `$1000` in the "auxiliary" RAM bank. Note that auxiliary RAM is not accessible within the emulation, if the selected machine type is original \]\[, or \]\[+.
- File locations `$C000` thru `$CFFF`, and `$1C000` thru `$1CFFF`, are *not* mapped into their corrsponding locations in addressable memory as described in the previous bullet point, since RAM is never mapped to those locations (they are reserved for slotted peripheral firmware). They are instead mapped to the "alternate" 4k RAM banks ("main RAM bank 1" and "auxiliary RAM bank 1", respectively), which are accessed at region `$D000` thru `$DFFF` (but *only* when appropriate soft switches have been activated).

If the file's contents would exceed the end of even auxiliary memory (128k), **bobbin** will exit with an error.

This option may be specified multiple times: later invocations will overwrite memory loaded from earlier ones (in the case of any overlap). You can also use intervening `--delay-until-pc` options, to run the emulator for a while with one file loaded into memory, and then when the program-counter register hits the delay-until point, load the next file.

##### --load-at, --load-loc  *arg*

RAM location to load file from `--load`.

The *arg* must be a hexadecimal 16-bit value, optionally preceded by `$` or `0x`.

##### --load-basic-bin *arg*

Load tokenized (binary) AppleSoft BASIC file at boot.

This option is effectively the same as `--load `*arg*` --load-at 801`, except that it does some additional "fixup" to connect it to the BASIC interpreter (to tell it where the program start and end are). If this option is not preceded by a  `--load`, `--load-at`, `--jump-to`, `--delay-until-pc`, or another `--load-basic-bin`, then it will automatically prefix itself with a `--delay-until-pc INPUT`, to ensure that the firmware finishes initializing before the basic program is loaded. If it *is* preceded by any of these options, then you may need to add the `--delay-until-pc INPUT` option yourself, depending on where execution will have reached before the `--load-basic-bin` option is honored.

##### --jump-to, --jump, --jmp *arg*

Jump the program counter to the location specified.

If a preceding `--delay-until-pc` was specified, waits until the program counter has reached the delay point normally, before jumping to this location.  Otherwise, the emulator will start at this location, instead of at the Apple II formware `RESET` routine.

##### --delay-until-pc, --delay-until, --delay-pc *arg*

Delays following `--load`, `--load-at`, or `--jump-to`, until PC is *arg*.

The primary intent of this option is to allow the Apple \]\[ to run through its usual boot-up code, and then load new contents into memory and jump to a new, post-boot-sequence "start". This makes it easy to boot the emulator into your favorite code, without involving a disk-load.

The special mnemonic *arg* value `INPUT` (case-insensitive) is translated to location `FD1B` (the monitor `KEYIN` routine for reading from the keyboard), which is frequently a very convenient value to use.

You can also specify this option multiple times: **bobbin** will wait until the first PC valye is reached, perform any successive `--load`, `--load-at`, or `--jump-to` up until the next `--delay-until`, wait again, rinse and repeat.

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

#### Diagnostics, Debugging, and Testing Options

##### --die-on-brk

Exit emulation with an error, if a BRK or illegal opcode is encountered.

##### --debug-on-brk

Enter Bobbin's debugger if a BRK is encountered (instead of the Monitor's).

##### --breakpoint, --bp *arg*

Set a debugger breakpoint (`simple` interface only).

##### --trace-to *m*\[:*n*\]

Trace N instructions before/including M (default N = 256).

This feature is intended for use with the `--trap-failure` feature. If the failure trao is triggered, it will output the instruction number where it did. You can then plug that number into this option, followed by a colon and the number of instructions *before* the number you gave, that should be logged to the trace file (`trace.log` by default; adjustable with `--trace-file`). As an example, if the failure traps at instruction *12564*, then `--trap-failure 12564:100` would start logging instructions, register values, and (explicit) memory accesses starting at instruction *12465*. If instruction *12564* is reached, the trace logging ends. If instruction *12564* does not trigger the failure trap, then execution will still continue, but no more information will be output to the trace-log file.

This feature is used by **bobbin**'s build system to debug the emulator's correctness. It could also be used for Apple developers to examine unexpected behavior in their software via automated tests.

Tracing can also be switched on and off, directly in the debugger, using the debugger's `tron` and `troff` commands. Tracing can fill up your disk space *very* rapidly, so it's best to use these commands surgically, and begin and end your traces to include just the bits that are relevant to your debugging session.

##### --trace-prodos-mli

Add additional information to the trace log for ProDOS MLI calls.

Note: this option is likely to be removed/replaced in the future. Whenever the instruction `JSR $BF00` is encountered, information is printed about the command being issued, and for some commands, the parameters given.

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

##### Non-interactive input

When **bobbin** detects that input is being fed to it from a file or a program (that is, not from a direct user at the terminal), the `--simple` interface suppresses locally-echoed characters and command-prompt characters  that the emulated Apple prints to the screen while input is being read. This is to support a familiar interface similar to other standard I/O tools: input comes into a program, output comes out. If output weren't suppressed during line input, the Apple would echo it back in the output. If you pipe a thousand-line BASIC program into **bobbin**, you'd probably be annoyed if you have to wade through those thousand lines you already had, just to get to the "good part" (the BASIC program's output).

##### Additional details of bobbin's I/O processing

If **bobbin** receives a `SIGINT` signal (often triggered by a user typing Ctrl-C), **bobbin** treats it exactly as a normal Ctrl-C keypress, except that if there is some input that **bobbin** has received but which hasn't yet been processed by the program running on the Apple \]\[, the Ctrl-C may jump to the front of the line, ahead of other characters that had actually been typed before it. Note that it is possible to configure a terminal to use some other keypress instead of Ctrl-C for sending `SIGINT`; in this case **bobbin** will still treat that character as if it had been a Ctrl-C.

If **bobbin** receives a second `SIGINT` signal before the emulated Apple has had a chance to consume it as a keypress; or if Ctrl-C is pressed twice in a row without any intervening characters (even if the first Ctrl-C was in fact processed successfully as input to the emulated Apple), then **bobbin** will open a debugger interface. This interface enables you, among other things, to type the "reset" key (performing either a "warm" or "cold" reset), to jump directly into the Apple monitor, or to step through the currently-running machine code.

If **bobbin** is processing non-interactive input while a `SIGINT` is received, and the `--remain` option is active, it will do three things in succession: (1) it will send a Ctrl-C to the program on the Apple \]\[ (in BASIC, this usually has the effect of causing a program `BREAK`); (2) it will discard any pipe or redirected input that was still waiting to be processed by the Apple \]\['s program, and (3) it will immediately enter "interactive" mode.

In the `--simple` interface, if a Ctrl-D is input by the user, then it indicates that **bobbin** should exit (**bobbin** may see a Ctrl-D entered directly, or it may "percieve" it via a return from `read()` that indicated no bytes remained to be read). **Bobbin** does not exit *immediately* upon receipt of a Ctrl-D. Instead, it waits until the program in the Apple \]\[ has caught up with the user input, up to the point when the Ctrl-D was typed, and issues a carriage return character. Only when the Apple \]\[ program indicates "consumption" of that carriage return, does **bobbin** then exit. This is to make the behavior consistent with other programs at a terminal.

On some terminals, the left-and-right arrow keys may not work (this happens when the actual terminal being used does not match the value of the `TERM` environment variable; or if the `termcap` or `terminfo` databases have incorrect escape sequences in the definition for that terminal). In such cases, Ctrl-U may be used as *right-arrow* instead. Ctrl-H is *left-arrow* (and is what Apple used as backspace). If you want to toggle between moving left and right while editing a line, Ctrl-H/Ctrl-U are conveniently close to one another; but if you just want to backspace over some characters, your keyboard's Backspace key is the better option.

The `--simple` interface does *not* grant you access to moving the cursor arround the screen using the Escape key in combination with other keys... or rather, it *does*, but you won't *see* it, because that requires full screen emulation (like the `--tty` interface will do). So, if you type something like:

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

Our advice is to refrain from using the Esc key at line inputs. If you want to copy/re-enter a line, it's usually easier just to use your terminal's copy/paste feature! ...Or you can use the (default) `tty` interface instead of `--simple`, so that you can actually see where the text cursor has moved to.


### Bobbin's built-in debugger

Typing Control-C twice in succession enters a command-input interface. The emulation is paused while the user is prompted for a command. A single Control-C will be passed through to the emulated Apple \]\[, (including the first of two Control-C's in succession). If you enter the command accident by accident when you really just wanted to send another Control-C (break) character to the emulated Apple, then type the command `^C` (the carat character followed by a capital C) and then the Enter key, and it will send the Control-C and continue emulation.

If you are using **bobbin**'s `tty` interface, the video display will vanish when you enter the debugger, and content that was visible before you launched **bobbin** may become visible again. When the debugger is exited (except by `q`, the "quit bobbin" command), the video display will be restored once again.

To exit the debugger interface and return to emulation, type `c` followed by the Enter key.

#### Essential commands

The following non-debugging-oriented commands are available:

**h**, **help**. Lists the essential commands and a brief description of each. Currently does not list the actual debugging commands.

**q**, **quit**. Quits the emulator.

**r**, **w**. Sends a "soft" reset signal to the CPU.

**rr**. Sends a "hard" reset signal signal to the CPU. This is currently achieved by explicitly invalidating the "powered up" checksum byte (at `$3F4`) for the user "reset vector" in memory (at `$3F2` and `$3F3`), which the firmware will then see as its cue to run a full reboot sequence.

**m**. Simulates a `BRK`, to print register values and enter the monitor (note: for some firmware ROMs, if the user `BRK` vector (at `$3FE` and `$3FF`) is set up to do something other than enter the monitor, then **m** won't actually enter the monitor&mdash;future versions of **bobbin** will take better steps to ensure we actually enter the monitor, regardless of what's in the user `BRK` vector.

**disk *NUM* eject**. Ejects any disk in drive *NUM* (where *NUM* is either 1 or 2). Can only be used if that drive is not currently spinning (which might indicate that changes have not yet been saved to the underlying image file).

**disk *NUM* load *FILE***. Loads the specified *FILE* into drive number *NUM*. **Please note:** this command is very fragile at the moment, and typing the wrong path (one that doesn't exist), or a path to a file that isn't writeable, will currenty cause the emulator to *quit*. This is a known issue that will be fixed in version 0.5 of **bobbin**. As with **disk eject**, this command may not be used when that drive's motor is spinning (there is a visual indicator for this in the `tty` interface, but not in the `simple` interface).

If you fire up **bobbin** without any disks initially, the emulated Apple \]\[ machine will not be configured with a disk-controller card (which causes it to boot up to BASIC instantly, instead of hanging indefinitely waiting for a disk to be inserted). If you then use the breakout **disk load** command to load a disk image file, a disk controller will automagically appear at slot 6, as if it had been there from the start. If you were then to eject that disk, and use the **rr** command (or `PR#6` at the BASIC prompt), then the system will be rebooted with an (empty) disk controller still active, and *then* you will see the familiar hang at the `APPLE ][` message on the top of the screen (send a regular soft reset (**r** or **w** at the command-input interface), to break into BASIC).

**save-ram *FILE*** (*not* documented in-program!). Use this command to dump current RAM contents into the named file (overwriting it, if it exists). The file size will be 128k (even if the emulated machine doesn't support that much RAM, or if RAM was foreshortened via the `--ram` option). "Language card" bank one (`$D000` when bank one is switched in) will be at file offset 0xC000 thru 0xCFFF, and auxiliary memory bank one (`$D000` when the **ALTZP** soft switch is on and bank one is switched in) will be at file offset 0x1C000.

#### Understanding the debugger display

The following debug-oriented commands are also available.

When you enter the debugger, you'll see a message like the following:

```
*** Welcome to Bobbin Debugger ***
  SPC = next intr, c = leave debugger (continue execution), m = Apple II monitor
  q = quit bobbin, r or w = warm reset, rr = cold reset
-----
ACC: A0  X: 00  Y: 05  SP: F4          [N]   V   [U]  [B]   D    I    Z    C
STK: $1F4:  26  17  FB  (A0)  37  FD  77  FD  D2  E3  D6  E2  |  FF
0300:   B1 28       LDA ($28),y      28: D0 07   07D5:  A0 A0 A0 A0 A0
>
```

As the introduction states, you can type `c` to get back out of the debugger. You can type `r` or `w` to perform a "warm reset" (same as Ctrl-RESET on a real machine), or type `rr` to perform a "hard reset" (same as Open-Apple Ctrl-RESET on a real machine). Bobbin doesn't currently emulate the Open-Apple key, so it just directly invalidates the "powered-up"/warm-reset verification byte before issuing the reset.

Taking a closer look at this portion of output from the debugger:

```
ACC: A0  X: 00  Y: 05  SP: F4          [N]   V   [U]  [B]   D    I    Z    C
STK: $1F4:  26  17  FB  (A0)  37  FD  77  FD  D2  E3  D6  E2  |  FF
0300:   B1 28       LDA ($28),y      28: D0 07   07D5:  A0 A0 A0 A0 A0
```

You can see that the values of the accumulator (A or ACC), the index registers X and Y, and the Stack Pointer, are printed first. Following that is a breakout of the processor flags register. Any flags enclosed in `[`square braces`]` are "set" (bit value 1); any that aren't are "reset" (or "unset"&mdash;bit value 0).

Next, a slice of the contents of the stack are displayed. The value at the current Stack Pointer (in this example, `$F4`, so this is the value at location `$1F4`) is printed in parentheses. It is preceded by three bytes of preceding data, which might be garbage, or might be return addresses of recently-exited subroutines. It is followed by the values at increasing location, the items that have been "pushed" onto the stack, from most- to least-recently-pushed. If you see a `|` bar, it indicates wraparound between the value at `$1FF`, to the value at `$100`.

On the third line, we have the Program Counter value, followed by the opcode and any arguments at that location, a disassembly of those, and finally any memory locations of interest. Since the `LDA ($28),y` instruction first looks at the 16-bit word starting at location `$28`, and then adds the value from the Y index register to that before reading a value from the resulting memory location, we first see the two bytes located at `$28`&mdash;`$D0` and `$07`, which (in the Apple \]\['s little-endian environment) represents memory location `$0705`&mdash;followed by the memory contents at `$07D5`&mdash;or, `$07D0` plus the contents of the Y register (5). The first of those values at `$07D5` are what will be stored in the accumulator by this `LDA` instruction.

You can step through successive lines of code by simply hitting return. Additional debugger commands are described below; note that there is currently no in-program documentation while running **bobbin** for these commands (will be added later).

#### Execution commands

***Enter*** As previously mentioned, a blank line steps to the next instruction.

**n** "Step over". If the current operation about to be performed is a `JSR`, then emulation is continued, breaking only when the stack is the same size again as it was before the `JSR`, or shorter (value in `SP` register is higher).

**rts** Returns from the current subroutine. Emulation is continued, breaking when the stack is two bytes shorter than it currently is (or shorter). Note that the name of this command is misleading: the break may not happen on an actual `RTS` instruction; it could just as easily break on a `TXS` operation (if that operation shortens the stack enough to trigger the break).

#### Breakpoint commands

**b 300** Sets a breakpoint at memory location `$300`. If the CPU is about to execute an instruction at this location, the breakpoint is triggered.

**w 300** Sets a watchpoint for memory location `$300`. If the value of the byte at `$300` changes from its current value, this watchpoint will be triggered. Note that *writing* a value to this location is not sufficient to trigger the break&mdash;the value must *change*.

Note that a **w** with no argument is a different command altogether (sends a "soft" reset signal to the CPU).

**disable 1** Disable breakpoint number 1. **Bobbin** doesn't currently provide a way to list the breakpoints and find their numbers (future feature), but when a breakpoint (or watchpoint) is reached, **bobbin** will state which breakpoint number has triggered.

**enable 1** Enable breakpoint number 1.

#### Monitor-like commands

The debugger has a few commands that are similar to those of Apple's system monitor. Note that they may not work exactly the same way, and not all commands are implemented. In particular there are no "write values to memory" (system monitor colon (`:`)), or "copy/move memory" (system monitor `M`) commands. Unlike in the system monitor, typing Enter again after a command won't repeat the previous command for the next memory range; it just steps to the next instruction. You must retype the full command to run it again.

**300** Display the byte at memory location `$300`.

**300.30F** Display the value at bytes `$300` through `$30F`.

**300L** Disassemble sixteen instructions, starting with the instruction at `$300`.

**300G** Jump execution to `$300` (and exit the debugger, returning to emulation).
