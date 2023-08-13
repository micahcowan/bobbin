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
    before = p.before
    before = re.sub('[^\n]*bobbin: FOUND ROM file "[^"\n]*/roms/apple2plus\.rom"\.\r\n', '- ELIDED -\r\n', before)
    return want_got(
"""\
%(bobbin)s: Searching for ROM file apple2plus.rom...\r
- ELIDED -\r
%(bobbin)s: ROM file checksum is good:\r
%(bobbin)s:   fc3e9d41e9428534a883df5aa10eb55b73ea53d2fcbb3ee4f39bed1b07a82905\r
\r
[Bobbin "simple" interactive mode.\r
 Ctrl-D at input to exit.\r
 Ctrl-C *TWICE* to enter debugger.]\r
\r
\r
]""" % {"bobbin": BOBBIN}, before)

@bobbin('-m plus --simple -v -v')
def m_plus_simple_v_v(p):
    got = p.expect([EOF, TIMEOUT])
    if got != 1:
        fail("got EOF")
    global BOBBIN
    before = p.before
    before = re.sub('[^\n]*bobbin: Looking for ROM named "apple2plus.rom" in [^\n]*/roms...\r\n', '- ELIDED -\r\n', before)
    before = re.sub('[^\n]*bobbin: FOUND ROM file "[^"\n]*/roms/apple2plus\.rom"\.\r\n', '- ELIDED -\r\n', before)
    return want_got(
"""\
%(bobbin)s: Searching for ROM file apple2plus.rom...\r
- ELIDED -\r
- ELIDED -\r
%(bobbin)s: ROM file checksum is good:\r
%(bobbin)s:   fc3e9d41e9428534a883df5aa10eb55b73ea53d2fcbb3ee4f39bed1b07a82905\r
\r
[Bobbin "simple" interactive mode.\r
 Ctrl-D at input to exit.\r
 Ctrl-C *TWICE* to enter debugger.]\r
\r
\r
]""" % {"bobbin": BOBBIN}, before)
