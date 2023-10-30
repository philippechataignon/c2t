100  DIM A%(1000)
105 F% = 3
106 rem -28416 = $9100
110  CALL  - 28416,F%,A%
120 for I = 0 to 9
121 ? i, a%(i)
122 next i
130 for I = 990 to 1000
131 ? i, a%(i)
132 next i
140 N% = 0
150  PRINT "ARRAY READY"
106 rem -28672 = $9000
200  CALL  - 28672,A%,N%
300  PRINT N%
