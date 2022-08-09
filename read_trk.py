#!/usr/bin/env python3
import sys
def main():
    f = open(sys.argv[1], "rb")
    g = open(f"{sys.argv[1]}.nib", "wb")
    c = f.read()
    # print(len(c))
    n = 12
    if c[:n] in c[n+1:]:
        print("OK")
        i = n + c[n+1:].index(c[:n]) + 1
        g.write(c[:i])
    else:
        print("6400")
        g.write(c[:6400])

if __name__ == '__main__':
    main()

