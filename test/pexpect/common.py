#!/usr/bin/python

from __future__ import print_function
from pexpect import EOF
import re
import sys

global tests
tests = []

def fail(s):
    raise Exception("FAILED: %s" % s)

def bobbin(fn, *args):
    if len(args) == 0:
        args = ''
    else:
        args = (args,)
    fn.bobbin_args = args
    tests.append(fn)
    return fn

if __name__ == '__main__':
    print("Do not run common.py, it is a library.")
    sys.exit(0)
