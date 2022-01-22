CFLAGS=-O2 -Wall
LDLIBS=

c2t:	c2t.o ihex/libkk_ihex.a

ihex/libkk_ihex.a:
	CC=gcc make -C ./ihex libkk_ihex.a

clean:
	rm c2t c2t.o
