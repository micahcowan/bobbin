  10  DATA  "zero","one","two","three","four","five","six","seven"
  20  DATA  "eight","nine","ten","eleven","twelve","thirteen"
  30  DATA  "fourteen","fifteen","sixteen","seventeen","eighteen"
  40  DATA  "nineteen","twenty","thirty","forty","fifty","sixty"
  50  DATA  "seventy","eighty","ninety"
  80  SPEED= 32: HOME : ONERR  GOTO 5000
 100 N% = 1
 200  IF N% = 0 THEN  PRINT "zero";: GOTO 500
 210 INDIC$ = " thousand":PLACE = 1000:MOD = 1: GOSUB 2000
 250 INDIC$ = " hundred":PLACE = 100:MOD = 10: GOSUB 2000
 280 INDIC$ = "":PLACE = 1:MOD = 100: GOSUB 2000
 500 N% = N% + 1
 550  PRINT 
 560  GOTO 200
2000  REM This part of the program knows how to count from zero to ninety-nine
2005  REM and to apply this to arbitrary places in a number
2100  REM First, get the number we actually want to speak about
2110 M% = N% / PLACE
2130 M% = M% -  INT (M% / MOD) * MOD
2200  REM   Discard if we're zero (if the ACTUAL number is zero, we handled)
2210  IF M% = 0 THEN  RETURN 
2220  IF M% > 19 THEN  GOTO 4000
3000  REM Print a number between zero and nineteen
3090  RESTORE 
3100  FOR I = 0 TO M%: READ NAME$: NEXT I
3200  PRINT NAME$;INDIC$;" ";
3800  RETURN 
4000  REM Print a number between twenty and ninety-nine
4010 O% = M% -  INT (M% / 10) * 10: REM Ones place digit
4020 T% = M% / 10: REM Tens place digit
4050  REM  Read past the <19 names, then find the tens name
4060  RESTORE : FOR I = 0 TO (19 + T% - 1): READ NAME$: NEXT I
4070  PRINT NAME$;
4100  IF O% <  > 0 THEN  GOTO 4500
4200  REM No ones place
4210  PRINT INDIC$;" ";
4220  RETURN 
4500  REM Done with tens place; handle ones place
4510  PRINT "-";
4530 M% = O%: GOTO 3000
5000  SPEED= 255: STOP 
