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
    and output PCM samples 48000 / S16_LE
    for use with the Apple II cassette interface.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define VERSION "1.1.0"

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

void writebyte(unsigned char byte, int freq0, int freq1, int rate, int bits) {
    unsigned char j;
    for(j = 0; j < 8; j++) {
        if(byte & 0x80)
            appendtone(freq1, rate, 0, 2, bits);
        else
            appendtone(freq0, rate, 0, 2, bits);
        byte <<= 1;
    }
}

void usage()
{
    fprintf(stderr,"\nVersion %s\n\n",VERSION);
    fprintf(stderr,"c2t [-a] [-f] [-n] [-8] [-r rate] [-s start] infile.bin\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "-8: 8 bits, default 16 bits\n");
    fprintf(stderr, "-a: applesoft binary\n");
    fprintf(stderr, "-f: fast load (need load8000)\n");
    fprintf(stderr, "-n: dry run\n");
    fprintf(stderr, "-r: rate 48000/44100/22050/11025/8000\n");
    fprintf(stderr, "-s: start of program : gives monitor command\n");
}

void header(int rate, int bits, int fast) {
    if (fast) {
        appendtone(2000,rate,0,500, bits);
    } else {
        appendtone(770 ,rate,4,0, bits);
        appendtone(2500,rate,0,1, bits);
        appendtone(2000,rate,0,1, bits);
    }
}

int main(int argc, char **argv)
{
    FILE *ifp;
    int j;
    int c;
    int length;
    int freq0=2000, freq1=1000;
    int rate=48000;
    int bits=16;
    int applesoft=0;
    int fake=0;
    int fast=0;
    char* data;
    int start = -1;
    int display_chksum = 0;
    char* infilename;
    unsigned char checksum = 0xff;
    opterr = 1;
    while((c = getopt(argc, argv, "acfhns:r:8")) != -1) {
        switch(c) {
            case 'h':        // version
                usage();
                return 0;
                break;
            case 'r':
                rate = atoi(optarg);
                break;
            case 's':
                start = strtol(optarg, NULL, 16);
                break;
            case 'f':
                freq0 = 12000;
                freq1 = 6000;
                fast = 1;
                break;
            case 'a':
                applesoft = 1;
                break;
            case '8':
                bits = 8;
                break;
            case 'n':
                fake = 1;
                break;
            case 'c':
                display_chksum = 1;
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

    if (infilename[0] == '-') {
        ifp = stdin;
    } else if ((ifp = fopen(infilename, "rb")) == NULL) {
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

    if (fake) {
        free(data);
        return 0;
    }

    if (applesoft) {
        header(rate, bits, fast);
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
    header(rate, bits, fast);
    checksum = 0xff;
    for(j=0; j<length; j++) {
        writebyte(data[j], freq0, freq1, rate, bits);
        checksum ^= data[j];
    }
    free(data);
    if (display_chksum) {
        fprintf(stderr, "Checksum: %X\n", checksum);
    }
    writebyte(checksum, freq0, freq1, rate, bits);
    if (fast) {
        appendtone(770, rate, 0, 16, bits);
    } else {
        appendtone(1000, rate, 0, 2, bits);
    }
    return 0;
}
