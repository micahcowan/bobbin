from debug import *
from basics import *
from asoft import *
from cpu65c02 import *
from cpu_verification import *
from charset import *
from mousetext import *
from tty_tests import *
import pexpect
import os
import re

global tests
global only_test
global status

verbose = False
status = 0

if '-v' in sys.argv:
    verbose = True

# Print a green [PASS]
def ppass():
    print("[\033[1;32mPASS\033[m]")

# Print a red [FAIL]
def pfail():
    print("[\033[31mFAIL\033[m]")
    global status
    status = 1

spaces = re.compile("_")

print()
if len(only_test) != 0:
    tests = only_test
for test in tests:
    name = test.__name__
    name = spaces.sub(" ", name)
    m = test.__module__
    print('%-40s' % (m + ": " + name), end='')
    p = pexpect.spawn('%s %s' % (BOBBIN, test.bobbin_args), echo=False, encoding='ascii')
    p.timeout = 1
    ret = False
    try:
        ret = test(p)
        if ret == True:
            ppass()
        else:
            pfail()
    except Exception as e:
        pfail()
        if (verbose):
            print('----- %s FAILURE -----' % test.__name__)
            print(type(e))
            print(str(e))
            print('-----')

sys.exit(status)
