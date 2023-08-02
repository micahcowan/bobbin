#!/usr/bin/python

from common import *

@bobbin('-m plus --simple')
def ctrl_c_in_dbg(p):
    p.expect("\r\n]")
    p.sendintr()
    p.expect(TIMEOUT)
    p.sendintr()
    p.expect("\r\n\\*\\*\\* Welcome to Bobbin Debugger \\*\\*\\*\r\n")
    p.expect("\r\n>")
    p.sendintr()
    p.expect("\r\n\\(Interrupt received\\.")
    p.expect("\r\n>")
    p.sendline("c")
    p.expect("\r\nContinuing...\r\n")
    p.expect(TIMEOUT)
    want_got("", p.before)
    p.sendline("")
    p.sendline("PRINT\"HELLO\"")
    p.expect("\r\nHELLO\r\n\r\n]")
    return True
