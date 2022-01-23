## Introduction

`c2t` is a command line tool that can convert binary code/data
for use with the Apple-1 and II (II, II+, //e) cassette interface.

`c2t` offers high-speed option (-f) as well as the native cassette
interface ROM routines.

Written from https://github.com/datajerk/c2t.

## Options

Command:
```
c2t [-a] [-f] [-g] [-l] [-n] [-8] [-r rate] [-s start] infile.hex
```
Options:
```
-8: 8 bits (default 16 bits)
-a: applesoft binary (implies -b)
-b: binary file (intel hex by default)
-f: fast load (need load8000)
-g: get load8000
-l: locked basic program (implies -a -b)
-n: dry run
-r: rate 48000(default)/24000 for fast/non-fast, 8000 for non-fast
-s: manual start of program
```

## Quick Start

For all examples, `ap` is an alias for `aplay -r48000 -fS16_LE`

------------------------------------------------------------------------------
File in intel hex format, dry run to see Apple command

```
PC command:
% c2t -n ~/asm6502/memtest2.hex

PC output
] CALL -151
* 280.2FCR
```
------------------------------------------------------------------------------
File in intel hex format

```
PC command:
c2t ~/asm6502/memtest2.hex | ap

Apple command:
] CALL -151
* 280.2FCR
```
------------------------------------------------------------------------------
Tokenized Appelsoft source created with `bastoken.py`

```
PC command:
% cat hplot.bas
1000 hgr2
1010 for y = 0 to 191 step 3
1020   x = y * 280 / 192
1030   hplot 279 - x,0 to 0,y
1040   hplot x, 191 to 0,y
1050   hplot 279 - x,0 to 279,191-y
1060   hplot x, 191 to 279,191-y
2040 next y
2050 input a$
2060 text
% ./bastoken/bastoken.py hplot.bas > hplot.bin
% ./c2t -a -b hplot.bin| ap

Apple command:
] LOAD
```
------------------------------------------------------------------------------
Tokenized Appelsoft source - piped version

```
PC Command:
% ./bastoken/bastoken.py hplot.bas | ./c2t -a -b - | ap

Apple command:
] LOAD
```
------------------------------------------------------------------------------
Fast load with -g option: get load8000 with c2t

```
PC command:
% ./c2t ../asm6502/intbasic.hex -fg | ap

Apple command:
] CALL -151
* FA.FDR 260.2FCR 260G

PC output memory range and entry point to go:
* A000.B424
* A000G
```
------------------------------------------------------------------------------
Fast load with load8000 already loaded

```
PC command:
% ./c2t ../asm6502/intbasic.hex -f | ap

Apple command:
] CALL -151
* FA.FDR 260G
```
------------------------------------------------------------------------------
Manual `load8000.hex` loading

```
Apple command:
] CALL -151
* 260.2FCR

PC Command:
% ./c2t ~/asm6502/load8000.hex | ap

PC Command dry-run to get Apple command:
% ./c2t ~/asm6502/integer.hex -n
] CALL -151
* BD00.BF0ER 

Apple command: start/end addresses in $FA-$FB/$FC-$FD little-endian
] CALL -151
* FA:00 BD 0E BF
* 260G

PC Command:
% ./c2t -f ~/asm6502/integer.hex | ap
```
