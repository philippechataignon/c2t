#!/usr/bin/env python3
import lz4.block

def fill(b):
    lb = (len(b) // 256 + 1) * 256
    return (b + 256 * b'\x00')[:lb]

def main():
    f = open("dos33.dsk", "rb")
    a = f.read()
    l = [ a[s:s+14336] for s in range(0, 143360, 14336) ]
    lc = [
        lz4.block.compress(
            c,
            mode = "high_compression",
            compression = 12,
            store_size = False,
        ) for c in l
    ]
    lc = [fill(c) for c in lc]
    print([hex(len(cc)>>8) for cc in lc])

if __name__ == '__main__':
    main()

