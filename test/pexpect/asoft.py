#!/usr/bin/python

from common import *

import re

@bobbin('-m plus --simple -q')
def asoft_basic_input(p):
    global BOBBIN
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('5 rem `{|}~',             '5 REM @[\\]^\r\n'),
        ('10 indecisive\b\b\b\b\b\b\b\bput a$', '10 INDECISIVE\b\b\b\b\b\b\b\bPUT A$\r\n'),
        ('20 for i=1 to 10',        '20 FOR I=1 TO 10\r\n'),
        ('30 ? spc(i);a$',          '30 ? SPC(I);A$\r\n'),
        ('40 next',                 '40 NEXT\r\n'),
        ('list', """\
LIST\r
\r
5  REM  @[\\]^\r
10  INPUT A$\r
20  FOR I = 1 TO 10\r
30  PRINT  SPC( I);A$\r
40  NEXT \r
"""),
    ])
    p.sendline('run')
    p.expect('\r\n?')
    want_got('RUN', p.before)
    commandck(repl,'bobbin rulez!!!',"""\
?BOBBIN RULEZ!!!\r
 BOBBIN RULEZ!!!\r
  BOBBIN RULEZ!!!\r
   BOBBIN RULEZ!!!\r
    BOBBIN RULEZ!!!\r
     BOBBIN RULEZ!!!\r
      BOBBIN RULEZ!!!\r
       BOBBIN RULEZ!!!\r
        BOBBIN RULEZ!!!\r
         BOBBIN RULEZ!!!\r
          BOBBIN RULEZ!!!\r
""")
    p.send("\x04") # EOF char
    p.expect(EOF)
    return True

@bobbin('-m plus --simple -q')
def asoft_basic_get(p):
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('10 get a$',                     '10 GET A$\r\n'),
        ('20 if a$ = "q" then print:end', '20 IF A$ = "Q" THEN PRINT:END\r\n'),
        ('30 print " - ";a$;" - ";',      '30 PRINT " - ";A$;" - ";\r\n'),
        ('40 go to 10',                   '40 GO TO 10\r\n'),
        ('list', """\
LIST\r
\r
10  GET A$\r
20  IF A$ = "Q" THEN  PRINT : END \r
\r
30  PRINT " - ";A$;" - ";\r
40  GOTO 10\r
""")
    ])
    p.sendline('RUN')
    p.send("BOBB")
    p.expect('RUN\r\n - B -  - O -  - B -  - B - ')
    p.send('\b')
    p.expect(' - \b - ')
    want_got('', p.before)
    p.send('in rulez')
    p.expect(' - I -  - N -  -   -  - R -  - U -  - L -  - E -  - Z - ')
    want_got('',p.before)
    p.send('Q')
    p.expect('\r\n]')
    want_got('\r\n',p.before)
    p.send('\x04') # EOF char
    p.expect(EOF)
    want_got('\r\n',p.before)
    return True

@bobbin('-m plus --simple -q')
def asoft_basic_infloop(p):
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('10 ? "Bobbin Rulez!"',        '10 ? "BOBBIN RULEZ!"\r\n'),
        ('20 GOTO 10',                  '20 GOTO 10\r\n')
    ])
    p.sendline('RUN')
    p.expect(TIMEOUT) # give it a chance to run for a bit
    if re.search('(BOBBIN RULEZ!\r\n){100}', p.before) is None:
        fail('Insufficient rulitude')
    p.sendintr()
    # We should see at least one more BOBBIN RULEZ (odds are, quite a few more)
    p.expect("\r\nBOBBIN RULEZ!\r\n")
    # Make sure we see a BREAK IN msg
    p.expect("\r\nBREAK IN ")
    p.send('\x04') # EOF char
    p.expect(EOF)
    if re.match('[12]0\r\n[]]\r\n$', p.before) is None:
        want_got('10\r\n]\r\n', p.before)
        fail('No match') # failsafe

    return True
