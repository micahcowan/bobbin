#!/usr/bin/python

from common import *

@bobbin
def no_args(p):
    p.expect(
"""\
[^\n]*bobbin: Default machine type is \"//e\", but that type is not actually\r
[^\n]*bobbin: supported in this development version.\r
[^\n]*bobbin: Try invoking with -m ]\[\+ or -m plus\r
[^\n]*bobbin: Exiting \(2\)\\.\r
""")
    if len(p.before) != 0:
        fail("preceding output: %s" % p.before)
    #p.expect(
    got = p.expect(EOF)
    if (len(p.before) != 0):
        fail("succeeding output: %s" % p.before)
    return True

#@bobbin('
