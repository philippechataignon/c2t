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
	This small utility will read Apple I/II binary and
	monitor text files and output Apple I or II AIFF and WAV
	audio files for use with the Apple I and II cassette
	interface.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#define ABS(x) (((x) < 0) ? -(x) : (x))

#define VERSION "Version 0.997a"

void appendtone(double **sound, long *length, int freq, int rate, double time, double cycles, int *offset, int square)
{
    long n = time * rate;
	long i;
	int j;

	if(freq && cycles)
		n= cycles * rate / freq;

	if(n == 0)
		n=cycles;

	if(square) {
		for (i = 0; i < n; i++) {
            int period = (rate / freq) / 2;
	        (*sound)[*length + i] = (2 * i * freq / rate + *offset ) % 2;
		}
	} else {
        for(i = 0; i < n; i++) {
	        (*sound)[*length + i] = sin(2 * M_PI * i * freq / rate + *offset * M_PI);
        }
    }

	if(cycles - (int)cycles == 0.5) {
		*offset = (*offset == 0);
    }

	*length += n;
}

void writebyte(unsigned char x, double** poutput, long* poutputlength, int freq0, int freq1, int rate, int* poffset, int square) {
	unsigned char j;
	for(j = 0; j < 8; j++) {
		if(x & 0x80)
			appendtone(poutput,poutputlength,freq1,rate,0,1,poffset,square);
		else
			appendtone(poutput,poutputlength,freq0,rate,0,1,poffset,square);
		x <<= 1;
	}
}

void usage()
{
	fprintf(stderr,"c2t [-v] [-s] [-r rate] [-b bits] infile.bin\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "-s: square wave (default: sin), -r: rate 44100/11025, -b: bits 8 or 16 -v: version");
}

void Write_WAVE(double *samples, long nsamples, int nfreq, int bits)
{
    FILE* fptr = stdout;
    double amp = 0.90;
	unsigned short v;
	int i;
	unsigned long totalsize, bytespersec;
	double themin, themax, scale, themid;

	// Write the form chunk
	fprintf(fptr, "RIFF");
	totalsize = (bits / 8) * nsamples + 36;
	fputc((totalsize & 0x000000ff), fptr);	// File size
	fputc((totalsize & 0x0000ff00) >> 8, fptr);
	fputc((totalsize & 0x00ff0000) >> 16, fptr);
	fputc((totalsize & 0xff000000) >> 24, fptr);
	fprintf(fptr, "WAVE");
	fprintf(fptr, "fmt ");		// fmt_ chunk
	fputc(16, fptr);			// Chunk size
	fputc(0, fptr);
	fputc(0, fptr);
	fputc(0, fptr);
	fputc(1, fptr);				// Format tag - uncompressed
	fputc(0, fptr);
	fputc(1, fptr);				// Channels
	fputc(0, fptr);
	fputc((nfreq & 0x000000ff), fptr);	// Sample frequency (Hz)
	fputc((nfreq & 0x0000ff00) >> 8, fptr);
	fputc((nfreq & 0x00ff0000) >> 16, fptr);
	fputc((nfreq & 0xff000000) >> 24, fptr);
	bytespersec = (bits / 8) * nfreq;
	fputc((bytespersec & 0x000000ff), fptr);	// Average bytes per second
	fputc((bytespersec & 0x0000ff00) >> 8, fptr);
	fputc((bytespersec & 0x00ff0000) >> 16, fptr);
	fputc((bytespersec & 0xff000000) >> 24, fptr);
	fputc((bits / 8), fptr);		// Block alignment
	fputc(0, fptr);
	fputc(bits, fptr);			// Bits per sample
	fputc(0, fptr);
	fprintf(fptr, "data");
	totalsize = (bits / 8) * nsamples;
	fputc((totalsize & 0x000000ff), fptr);	// Data size
	fputc((totalsize & 0x0000ff00) >> 8, fptr);
	fputc((totalsize & 0x00ff0000) >> 16, fptr);
	fputc((totalsize & 0xff000000) >> 24, fptr);

	// Find the range
	themin = samples[0];
	themax = themin;
	for (i = 1; i < nsamples; i++) {
		if (samples[i] > themax)
			themax = samples[i];
		if (samples[i] < themin)
			themin = samples[i];
	}
	if (themin >= themax) {
		themin -= 1;
		themax += 1;
	}
	themid = (themin + themax) / 2;
	themin -= themid;
	themax -= themid;
	if (ABS(themin) > ABS(themax))
		themax = ABS(themin);
	scale = amp * ((bits == 16) ? 32760 : 124) / (themax);

	// Write the data
	for (i = 0; i < nsamples; i++) {
		if (bits == 16) {
			v = (unsigned short) (scale * (samples[i] - themid));
			fputc((v & 0x00ff), fptr);
			fputc((v & 0xff00) >> 8, fptr);
		} else {
			v = (unsigned char) (scale * (samples[i] - themid));
			fputc(v + 0x80, fptr);
		}
	}
}

int main(int argc, char **argv)
{
	FILE *ifp;
	FILE *ofp;

    double *output = (double*) malloc(10000000);
    long outputlength=0;
    int offset = 0;
	int i, j;
	int c;
    int length;
    int freq0=2000, freq1=1000;
    int rate=44100;
    int bits=16;
    int square=0;
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
            case 's':
                square = 1;
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

	ofp=stdout;

	appendtone(&output,&outputlength,770 ,rate,4.0,0  ,&offset,square);
	appendtone(&output,&outputlength,2500,rate,0  ,0.5,&offset,square);
	appendtone(&output,&outputlength,2000,rate,0  ,0.5,&offset,square);
	checksum = 0xff;
    for(j=0; j<length; j++) {
        writebyte(data[j], &output, &outputlength, freq0, freq1, rate, &offset, square);
        checksum ^= data[j];
    }
    writebyte(checksum, &output, &outputlength, freq0, freq1, rate, &offset, square);
    appendtone(&output,&outputlength,1000,rate,0,1,&offset,square);
    Write_WAVE(output,outputlength,rate,bits);
}
