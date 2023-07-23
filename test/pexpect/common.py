#!/usr/bin/python

from __future__ import print_function
from pexpect import EOF, TIMEOUT
import re
import sys

global tests
tests = []

global BOBBIN
BOBBIN = '../../src/bobbin'

def want_got(want, got):
    if want != got:
        fail("\nWanted:\n%s\n\nGot:\n%s\n" % (want,got))
    return True

def fail(s):
    raise Exception("FAILED: %s" % s)

# fn decorator to choose bobbin args and register the test
def bobbin(*args):
    if len(args) == 0:
        args = ''
    else:
        (args,) = args
    def wrapper(fn):
        tests.append(fn)
        fn.bobbin_args = args
        return fn
    return wrapper

if __name__ == '__main__':
    print("Do not run common.py, it is a library.")
    sys.exit(0)
