## Introduction

`c2t` is a command line tool that can convert binary code/data
for use with the Apple-1 and II (II, II+, //e) cassette interface.

`c2t` offers high-speed option (-f) as well as the native cassette
interface ROM routines.

On alsamixer, put master/headphones/PCM: 100%

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
File in intel hex format, normal speed

```
PC command:
c2t ~/asm6502/memtest2.hex | ap

Apple command:
] CALL -151
* 280.2FCR
```
------------------------------------------------------------------------------
File in intel hex format, fast speed

```
PC command:
% ./c2t ../asm6502/intbasic.hex -f | ap

Apple command:
] CALL -151
* 280.2FFR 280G

PC output memory range and entry point to go:
* A000.B424
* A000G
```
With fast speed (-f), Apple ][ commands are always the same: `280.2FFR 280G`.
Because dynamic load and self.midification, program at $280 can't be reuse.

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
