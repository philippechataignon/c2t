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

void appendtone(int freq, int rate, int time, int cycles, int bits)
{
    static int offset = 0;

    unsigned long n = time > 0 ? time * rate : freq > 0 ? (cycles * rate) / (2 * freq) : cycles;

	unsigned long i;
	for (i = 0; i < n; i++) {
        int value = ((2 * i * freq) / rate + offset ) % 2;
		if (bits == 16) {
			int v = value ? 0x6666 : -0x6666;
			putchar(v & 0xff);
			putchar(v >> 8 & 0xff);
		} else {
			unsigned char v = (unsigned char)0xcc * value + 0x1a; // $80 +- $66 = 80% 7F
			putchar(v);
		}
	}

	if (cycles % 2) {
		offset = (offset == 0);
    }
}

void writebyte(unsigned char x, int freq0, int freq1, int rate, int bits) {
	unsigned char j;
	for(j = 0; j < 8; j++) {
		if(x & 0x80)
			appendtone(freq1, rate, 0, 2, bits);
		else
			appendtone(freq0, rate, 0, 2, bits);
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
	int i, j;
	int c;
    int length;
    int freq0=2000, freq1=1000;
    int rate=44100;
    int bits=16;
    int applesoft=0;
    char* data;
    int start = -1;
    char* infilename;
    unsigned char checksum = 0xff;
	opterr = 1;
	while((c = getopt(argc, argv, "ads:r:b:v8")) != -1) {
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
            case 's':
                start = strtol(optarg, NULL, 16);
                break;
            case 'd':
                freq0 = 6000;
                freq1 = 12000;
                break;
            case 'a':
                applesoft = 1;
                break;
            case '8':
                bits = 8;
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
	fclose(ifp);

    if (start >= 0) {
	    fprintf(stderr, "%d bytes read from %s\n", length, infilename);
	    fprintf(stderr, "] CALL -151\n");
	    fprintf(stderr, "* %X.%XR\n", start, start + length - 1);
    }

    if (applesoft) {
	    appendtone(770 ,rate,4,0, bits);
	    appendtone(2500,rate,0,1, bits);
	    appendtone(2000,rate,0,1, bits);
	    checksum = 0xff;
        unsigned char tmp;
        tmp = (length - 1) & 0x000000ff;
        checksum ^= tmp;
        writebyte(tmp, freq0, freq1, rate, bits);
        tmp = ((length - 1) & 0x0000ff00) >> 8;
        checksum ^= tmp;
        writebyte(tmp, freq0, freq1, rate, bits);
        tmp = 0x55;
        checksum ^= tmp;
        writebyte(tmp, freq0, freq1, rate, bits);
        writebyte(checksum, freq0, freq1, rate, bits);
        appendtone(1000,rate,0,2, bits);
    }
	appendtone(770 ,rate,4,0, bits);
	appendtone(2500,rate,0,1, bits);
	appendtone(2000,rate,0,1, bits);
	checksum = 0xff;
    for(j=0; j<length; j++) {
        writebyte(data[j], freq0, freq1, rate, bits);
        checksum ^= data[j];
    }
    writebyte(checksum, freq0, freq1, rate, bits);
    appendtone(1000,rate,0,2, bits);
    free(data);
    return 0;
}
