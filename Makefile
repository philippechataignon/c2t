CFLAGS=-O2 -Wall
LDLIBS=-llz4

all: c2t lz4_compress

c2t:	c2t.o ihex/libkk_ihex.a
lz4_compress: lz4_compress.o

ihex/libkk_ihex.a:
	CC=gcc make -C ./ihex libkk_ihex.a

clean:
	rm c2t c2t.o
