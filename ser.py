#!/usr/bin/env python3
import sys
import serial
def main():
    ack = b'\x06'
    ser = serial.Serial('/dev/ttyUSB0')  # open serial port
    ser.baudrate=19200
    ser.bytesize=8
    ser.rtscts=1
    ser.dsrdtr=1
    print(ser)         # check which port was really used
    c1 = ser.read(1)
    c2 = ser.read(1)
    c3 = ser.read(1)
    if c1 == b'\x19' and c2 == b'\x64' and c3 == b'\x00':
        print("Start OK", file=sys.stderr)
        ser.write(ack)

    with open(f"ser.out", "wb") as g:
        for i in range(5):
            print(f"Wait segment #{i+1}")
            for j in range(7):
                c = ser.read(0x1000)
                g.write(c)
                print(".", end="", file=sys.stderr, flush=True)
            print(file=sys.stderr)
            print(f"Get segment #{i+1}", file=sys.stderr)
            ser.write(ack)

    # final ack
    c1 = ser.read(1)
    if c1 == ack:
        print("OK, done.", file=sys.stderr)
        ser.write(ack)
    else:
        print("Error: no ack", file=sys.stderr)


    ser.close()

if __name__ == '__main__':
    main()
