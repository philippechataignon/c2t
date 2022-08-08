#!/usr/bin/env python3
import sys
import io
import os
import os.path
import serial
from xmodem import XMODEM

def main():
    ser = serial.Serial('/dev/ttyUSB0', baudrate=19200)
    def putc(data, timeout=1):
        return ser.write(data)
    def getc(size, timeout=1):
        return ser.read(size) or None

    ack = b'\x06'
    esc = b'\x9B'
    nbtrk = 35

    if len(sys.argv) != 2:
        print("Entrer dsk filename")
        return
    modem = XMODEM(getc, putc)
    print(ser)

    dirname = sys.argv[1]
    os.mkdir(dirname)

    for trk in range(nbtrk):
        fn = f"{dirname}/{sys.argv[1]}_{trk:02d}.trk"
        print(f"Waiting track #{trk} for {fn}", file=sys.stderr)
        wait_esc = ser.read(1)
        if wait_esc == esc:
            ser.write(ack)
        else:
            print("Error: no ESC char received, exit", file=sys.stderr)
            return 1
        with open(fn, "ab") as stream:
            print(f"Get track #{trk}")
            modem.recv(stream, crc_mode=0)
    ser.close()

if __name__ == '__main__':
    main()
