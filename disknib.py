#!/usr/bin/env python3

import sys
import io
import os.path
import serial
import lz4.block
from xmodem import XMODEM

def main():
    numtrk = 35
    #ser = serial.Serial('/dev/ttyUSB0', baudrate=19200)
    #def putc(data, timeout=1):
    #    return ser.write(data)
    #def getc(size, timeout=1):
    #    return ser.read(size) or None
    #modem = XMODEM(getc, putc)
    buffaddr = 0x1000

    if len(sys.argv) != 2:
        print("Entrer trk directory")
        return 1

    dirname = sys.argv[1]
    if not os.path.isdir(dirname):
        print(f"Directory {dirname} doesn't exist")
        return 2

    lc = []
    endaddr = []
    for trk in range(numtrk):
        with open(f"{dirname}/{dirname}_{trk:02d}.trk", "rb") as f:
            a = f.read()
            n = 14
            pos = a[n:].find(a[:n])
            if 6000 <= pos:
                a = a[:n+pos]
            else:
                a = a[:6400]
            a = a + b'\x00'
            c = lz4.block.compress(
                a,
                mode = "high_compression",
                compression = 12,
                store_size = False,
            )
            lc.append(c)
            endaddr.append(buffaddr + len(c))

    param = b"".join([cc.to_bytes(2, byteorder="little") for cc in endaddr])
    print([hex(x) for x in endaddr])

    print("Send param")
    modem.send(io.BytesIO(param))
    for numtrk, trk in enumerate(lc):
        print(f"Send track #{numtrk}")
        modem.send(io.BytesIO(trk))

if __name__ == '__main__':
    main()
