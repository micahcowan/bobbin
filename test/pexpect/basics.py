#!/usr/bin/python

from common import *

@bobbin()
def no_args(p):
    p.expect(EOF)
    global BOBBIN
    return want_got(
"""\
%(bobbin)s: Default machine type is "//e", but that type is not actually\r
%(bobbin)s: supported in this development version.\r
%(bobbin)s: Try invoking with -m ][+ or -m plus\r
%(bobbin)s: Exiting (2).\r
""" % {"bobbin": BOBBIN}, p.before)

@bobbin('-m plus')
def m_plus_only(p):
    p.expect(EOF)
    global BOBBIN
    return want_got(
"""\
%(bobbin)s: default interface is "tty" when stdin is a tty; but that\r
%(bobbin)s: interface is not yet implemented in this development version.\r
%(bobbin)s: Try invoking with --simple.\r
%(bobbin)s: Exiting (2).\r
""" % {"bobbin": BOBBIN}, p.before)

@bobbin('-m plus --simple')
def m_plus_simple(p):
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    return want_got(
"""\
\r
[Bobbin "simple" interactive mode.\r
 Ctrl-D at input to exit.\r
 Ctrl-C *TWICE* to enter debugger.]\r
\r
\r
]""", p.before)

@bobbin('-m plus --simple -q')
def m_plus_simple_q(p):
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    return want_got("\r\n\r\n]", p.before)

@bobbin('-m plus --simple -v')
def m_plus_simple_v(p):
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    global BOBBIN
    return want_got(
"""\
%(bobbin)s: FOUND ROM file "../../src/roms/apple2plus.rom".\r
%(bobbin)s: ROM file checksum is good:\r
%(bobbin)s:   378ba00c86a64cca49cedaca7de8d5d351983ebc295d9d11e0752febfc346249\r
\r
[Bobbin "simple" interactive mode.\r
 Ctrl-D at input to exit.\r
 Ctrl-C *TWICE* to enter debugger.]\r
\r
\r
]""" % {"bobbin": BOBBIN}, p.before)

@bobbin('-m plus --simple -v -v')
def m_plus_simple_v_v(p):
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    global BOBBIN
    return want_got(
"""\
%(bobbin)s: Looking for ROM named "apple2plus.rom" in ../../src/roms...\r
%(bobbin)s: FOUND ROM file "../../src/roms/apple2plus.rom".\r
%(bobbin)s: ROM file checksum is good:\r
%(bobbin)s:   378ba00c86a64cca49cedaca7de8d5d351983ebc295d9d11e0752febfc346249\r
\r
[Bobbin "simple" interactive mode.\r
 Ctrl-D at input to exit.\r
 Ctrl-C *TWICE* to enter debugger.]\r
\r
\r
]""" % {"bobbin": BOBBIN}, p.before)
