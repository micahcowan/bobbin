#!/usr/bin/python

from common import *

# The following function generates a test fro a specific illegal 6502 opcode
def make_illegal_op_test(op):
    # The test we're creating around `op`:
    def the_test(p):
        p.send('CALL -151\r'); # Enter the monitor
        p.send('0:EA EA EA EA EA EA\r'); # Preseed with NOPs
        p.send('0:%02X\r' % op); # Write the illegal op
        p.send('0G\r'); # Execute the illegal op
        got = p.expect([EOF, TIMEOUT])

        if got == 1:
            fail("Unexpected timeout! (should either die, or exit on trap.)")
        status = p.wait()
        # We're EXPECTING a FAILING exit status.
        # 
        # If the illegal opcode was (correctly) treated as BRK, then
        # --die-on-brk should have caused it to, well, die.
        # If the opcode was interpreted and successfully executed,
        # execution will reach PC=$0003, and trigger the --trap-success,
        # ending with a 0 exit status (which is a test FAIL for us).
        if status != 0 and "ILLEGAL OP" in p.before:
            # Failed successfully!
            True
        elif status == 0 and "!!! REPORT SUCCESS !!!" in p.before:

            fail('Illegal opcode was executed!')
        else:
            fail("UNKNOWN FINAL CONDITION. Got text that didn't match exit status.")
        return True
    
    # Rename the test function
    the_test.__name__ = 'illegal_op_%02X' % op
    # Register the test with a bobbin invocation
    the_test = bobbin('--simple -m plus --die-on-brk --trap-success 3')(the_test)
    the_test = only(the_test)

# Create tests for each illegal op that should act like BRK:
for n in [0x04, 0x0C, 0x12, 0x14, 0x1C, 0x32, 0x34, 0x3C, 0x52, 0x5A,
         0x64, 0x72, 0x74, 0x7A, 0x7C, 0x80, 0x89, 0x92, 0x9C, 0x9E,
         0xB2, 0xDA, 0xF2, 0xFA]:
    make_illegal_op_test(n)

# TODO: Create tests for $1A, $3A
