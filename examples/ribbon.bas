  10 I = 0:M = 100:M2 = 18
  11 MSG$ = "ribbon words"
  12 L = 2:R = 25:R2 = 8
  20 A = L + R + R2 +  INT (R *  SIN (6.28 * (I / M)) + R2 *  SIN (6.28 * (I / M2)))
  30  PRINT  SPC( A);: PRINT MSG$
  40 I = I + 1
  50  GOTO 20
