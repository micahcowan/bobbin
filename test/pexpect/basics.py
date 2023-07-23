#!/usr/bin/python

from common import *

@bobbin
def no_args(p):
    p.expect(EOF)
    global BOBBIN
    wanted = (
"""\
%(bobbin)s: Default machine type is \"//e\", but that type is not actually\r
%(bobbin)s: supported in this development version.\r
%(bobbin)s: Try invoking with -m ][+ or -m plus\r
%(bobbin)s: Exiting (2).\r
""" % {"bobbin": BOBBIN})
    if wanted != p.before.decode('ascii'):
        fail("wanted: %s\n\nGot:    %s\n" % (wanted,p.before))
    else:
        return True

#@bobbin('
