#!/usr/bin/env python3
import sys
def main():
    f = open(sys.argv[1], "rb")
    g = open(f"{sys.argv[1]}.nib", "wb")
    print(sys.argv[1], end=" ")
    c = f.read()
    # print(len(c))
    n = 14
    pos = c[n:].find(c[:n])
    if 6000 <= pos:
        print("OK")
        g.write(c[:n+pos])
    else:
        print(f"6400 ({pos})")
        g.write(c[:6400])

if __name__ == '__main__':
    main()

