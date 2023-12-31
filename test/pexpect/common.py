#!/usr/bin/python

from __future__ import print_function
from pexpect import EOF, TIMEOUT
from pexpect.replwrap import REPLWrapper
import pexpect
import re
import sys
from difflib import unified_diff

global tests
tests = []
global only_test
only_test = []

global BOBBIN
BOBBIN = '../../src/bobbin'

def multi_commands(repl, cmdlist):
    for (cmd, want) in cmdlist:
        commandck(repl, cmd, want)
    return True

def commandck(repl, cmd, want):
    got = repl.run_command(cmd)
    return want_got(want, got)

def want_got(want, got):
    if want != got:
        fail("Got/want mismatch:\n%s\n" %
                ''.join(unified_diff(want.splitlines(True),got.splitlines(True))))
    return True

def fail(s):
    raise Exception("FAILED: %s" % s)

# fn decorator to select only this test to be run
def only(fn):
    only_test.append(fn)
    return fn

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
