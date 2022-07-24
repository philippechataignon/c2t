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
import lz4.block
from xmodem import XMODEM

def main():
    ser = serial.Serial('/dev/ttyUSB0', baudrate=19200)
    def putc(data, timeout=1):
        return ser.write(data)
    def getc(size, timeout=1):
        return ser.read(size) or None
    modem = XMODEM(getc, putc)
    buffaddr = 0x1000
    dsksize = 143360

    if len(sys.argv) != 2:
        print("Entrer dsk filename")
        return
    fn = sys.argv[1]
    if os.path.getsize(fn) != dsksize:
        print("Filesize must be 143360")
        return
    with open(sys.argv[1], "rb") as f:
        a = f.read()
        l = [a[s:s + dsksize // 10] for s in range(0, dsksize, dsksize // 10)]
        lc = [
            lz4.block.compress(
                c,
                mode = "high_compression",
                compression = 12,
                store_size = False,
            ) for c in l
        ]
        endaddr = [buffaddr + len(cc) for cc in lc]
        print([hex(cc) for cc in endaddr])
        param = b"".join([cc.to_bytes(2, byteorder="little") for cc in endaddr])

    print("Send param")
    modem.send(io.BytesIO(param))
    for numseg, seg in enumerate(lc):
        print(f"Send segment #{numseg}")
        modem.send(io.BytesIO(seg))

if __name__ == '__main__':
    main()
