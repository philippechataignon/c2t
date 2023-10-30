100  DIM A%(1000)
105 rem -28416 = $9100
110  CALL  - 28416,A%
120 for I = 0 to 9
130 PRINT i, a%(i)
140 next i
150 for I = 990 to 1000
160 PRINT i, a%(i)
170 next i
