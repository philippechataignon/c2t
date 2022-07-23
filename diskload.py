#!/usr/bin/env python3
'''
xmodem send (M0 .. M9)
xmodem send S0
      .
      .
xmodem send S9
'''

import sys
import io
import os.path
import serial
from xmodem import XMODEM
import lz4.block

def fill(b):
    lb = (len(b) // 256 + 1) * 256
    return (b + 256 * b'\x00')[:lb]

def main():
    if len(sys.argv) != 2:
        print("Entrer dsk filename")
        return
    fn = sys.argv[1]
    if os.path.getsize(fn) != 143360:
        print("Filesize must be 143360")
        return
    f = open(sys.argv[1], "rb")

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
    print([hex(len(cc)) for cc in lc])
    param = b"".join([len(cc).to_bytes(2, byteorder="little") for cc in lc])

    ser = serial.Serial('/dev/ttyUSB0', baudrate=19200)
    def putc(data, timeout=1):
        return ser.write(data)
    def getc(size, timeout=1):
        return ser.read(size) or None
    modem = XMODEM(getc, putc)

    print("Send param")
    modem.send(io.BytesIO(param))
    for numseg, seg in enumerate(lc):
        print(f"Send segment #{numseg}")
        modem.send(io.BytesIO(seg))

if __name__ == '__main__':
    main()
