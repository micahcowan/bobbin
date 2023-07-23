#!/usr/bin/python

from common import *

@bobbin('-m plus --simple -q')
def asoft_basic_input(p):
    global BOBBIN
    repl = REPLWrapper(p, "\r\n]", None)
    multi_commands(repl, [
        ('5 rem `{|}~',             '5 REM @[\\]^\r\n'),
        ('10 input a$',             '10 INPUT A$\r\n'),
        ('20 for i=1 to 10',        '20 FOR I=1 TO 10\r\n'),
        ('30 print spc(i);a$',      '30 PRINT SPC(I);A$\r\n'),
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
    return True
    #return p.send("""\
