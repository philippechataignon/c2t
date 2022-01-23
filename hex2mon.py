#!/usr/bin/env python3
from intelhex import IntelHex
def fmt(x):
    return hex(x)[2:].upper()

def hex2mon(infile, line=16):
    ih = IntelHex(infile)
    for s in ih.segments():
        for a in range(s[0], s[1], line):
            print(fmt(a), ":", end="", sep="")
            for sr in range(a, min(a+line, s[1])):
                print(fmt(ih[sr]), end=" ")
            print()

def hex2c(infile, line=16):
    ih = IntelHex(infile)
    for s in ih.segments():
        for a in range(s[0], s[1], line):
            for sr in range(a, min(a+line, s[1])):
                print("0x%02X" % ih[sr], end=",")
            print()

if __name__ == '__main__':
    import sys
    hex2mon(sys.argv[1])
    hex2c(sys.argv[1])
