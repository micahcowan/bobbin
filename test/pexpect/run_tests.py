from basics import *
import pexpect
import os
import re

global tests
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

os.environ['BOBBIN_ROMDIR'] = '../../src/roms'

spaces = re.compile("_")

print()
for test in tests:
    name = test.__name__
    name = spaces.sub(" ", name)
    print('%-30s' % name, end='')
    p = pexpect.spawn('%s %s' % (BOBBIN, test.bobbin_args))
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
