#!/usr/bin/env python3
import sys
import io
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
    nbseg = 5

    if len(sys.argv) != 2:
        print("Entrer dsk filename")
        return
    fn = sys.argv[1]
    modem = XMODEM(getc, putc)
    print(ser)

    for i in range(nbseg):
        print("Waiting", file=sys.stderr)
        wait_esc = ser.read(1)
        if wait_esc == esc:
            # print("Get ESC char, send ACK", file=sys.stderr)
            ser.write(ack)
        else:
            print("Error: no ESC char received, exit", file=sys.stderr)
            return 1
        with open(fn, "ab") as stream:
            print(f"Get segment #{i}")
            modem.recv(stream, crc_mode=0)
    ser.close()

if __name__ == '__main__':
    main()
