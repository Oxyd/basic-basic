005 REM Factorial calculation in classical Basic
010 PRINT "Enter a natural number "
020 INPUT n
030 IF n < 0 THEN 120
040 LET f = 1
050 LET i = n
060 IF i = 0 THEN 100
070 LET f = f * i
080 LET i = i - 1
090 GOTO 60
100 PRINT n, "! = ", f
110 STOP
120 PRINT n, " is not a natural number"
