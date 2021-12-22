CFLAGS=-O2
LIBS=-lm

c2t:	c2t.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm c2t c2t.o
