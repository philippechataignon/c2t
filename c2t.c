/*
c2t, Code to Tape|Text, Version 0.997, Wed Sep 27 15:27:56 GMT 2017

Parts copyright (c) 2011-2017 All Rights Reserved, Egan Ford (egan@sense.net)

THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Built on work by:
	* Mike Willegal (http://www.willegal.net/appleii/toaiff.c)
	* Paul Bourke (http://paulbourke.net/dataformats/audio/, AIFF and WAVE output code)
	* Malcolm Slaney and Ken Turkowski (Integer to IEEE 80-bit float code)
	* Lance Leventhal and Winthrop Saville (6502 Assembly Language Subroutines, CRC 6502 code)
    * Piotr Fusik (http://atariarea.krap.pl/x-asm/inflate.html, inflate 6502 code)
    * Rich Geldreich (http://code.google.com/p/miniz/, deflate C code)
    * Mike Chambers (http://rubbermallet.org/fake6502.c, 6502 simulator)

License:
	*  Do what you like, remember to credit all sources when using.

Description:
	This small utility will read Apple II binary
	and output PCM samples 44100 / S16_LE
	for use with the Apple II cassette interface.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define VERSION "Version 1.0.1"

void appendtone(int freq, int rate, double time, double cycles, int *offset)
{
    long n = time * rate;
	long i;
	int j;
    int bits = 16;

	if(freq && cycles)
		n= cycles * rate / freq;

	if(n == 0)
		n=cycles;

	for (i = 0; i < n; i++) {
        int period = (rate / freq) / 2;
        int value = (2 * i * freq / rate + *offset ) % 2;
		if (bits == 16) {
			int v = value ? 0x6666 : -0x6666;
			putchar((v & 0x00ff));
			putchar((v & 0xff00) >> 8);
		} else {
			unsigned char v = (unsigned char)0xcc * value + 0x1a; // $80 +- $66 = 80% 7F
			putchar(v);
		}
	}

	if(cycles - (int)cycles == 0.5) {
		*offset = (*offset == 0);
    }
}

void writebyte(unsigned char x, int freq0, int freq1, int rate, int* poffset) {
	unsigned char j;
	for(j = 0; j < 8; j++) {
		if(x & 0x80)
			appendtone(freq1,rate,0,1,poffset);
		else
			appendtone(freq0,rate,0,1,poffset);
		x <<= 1;
	}
}

void usage()
{
	fprintf(stderr,"c2t [-v] [-r rate] [-b bits] infile.bin\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "-r: rate 44100/11025, -b: bits 8 or 16 -v: version");
}

int main(int argc, char **argv)
{
	FILE *ifp;
    int offset = 0;
	int i, j;
	int c;
    int length;
    int freq0=2000, freq1=1000;
    int rate=44100;
    int bits=16;
    char* data;
    char* infilename;
    unsigned char checksum = 0xff;
	opterr = 1;
	while((c = getopt(argc, argv, "sr:b:v")) != -1) {
		switch(c) {
			case 'v':		// version
				fprintf(stderr,"\n%s\n\n",VERSION);
				return 2;
				break;
            case 'r':
                rate = atoi(optarg);
                break;
            case 'b':
                bits = atoi(optarg);
                break;
		}
    }

    if (optind >= argc) {
		usage();
		return 1;
    }

    infilename = argv[optind];

	if((data = malloc(64*1024 + 1)) == NULL) {
		fprintf(stderr,"could not allocate 64KiB data\n");
		return 3;
	}

	if ((ifp = fopen(infilename, "rb")) == NULL) {
		fprintf(stderr,"Cannot read: %s\n\n",infilename);
		return 4;
	}

    length = fread(data, 1, 65536, ifp);
	fprintf(stderr, "%d bytes read from %s\n", length, infilename);
	fclose(ifp);

	appendtone(770 ,rate,4.0,0  ,&offset);
	appendtone(2500,rate,0  ,0.5,&offset);
	appendtone(2000,rate,0  ,0.5,&offset);
	checksum = 0xff;
    for(j=0; j<length; j++) {
        writebyte(data[j], freq0, freq1, rate, &offset);
        checksum ^= data[j];
    }
    writebyte(checksum, freq0, freq1, rate, &offset);
    appendtone(1000,rate,0,1,&offset);
    free(data);
    return 0;
}
