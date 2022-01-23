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
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "ihex/kk_ihex_read.h"

#define VERSION "1.2.0"
#define AUTODETECT_ADDRESS UINT16_MAX

static uint8_t data[65536];
static uint32_t file_position = 0L;
static uint16_t address_offset = 0UL;
static uint16_t ptr = 0;

uint16_t start_load8000 = 0x260;
uint8_t load8000[] = {
    0xA5, 0xFA, 0x8D, 0x8B, 0x02, 0xA5, 0xFB, 0x8D,
    0x8C, 0x02, 0xA2, 0x00, 0x2C, 0x60, 0xC0, 0x10,
    0xFB, 0xA9, 0x01, 0xA0, 0x00, 0x2C, 0x60, 0xC0,
    0x30, 0xFB, 0xC8, 0x2C, 0x60, 0xC0, 0x10, 0xFA,
    0xC0, 0x40, 0xB0, 0x17, 0xC0, 0x14, 0xB0, 0xEB,
    0xC0, 0x07, 0x3E, 0x00, 0x00, 0xA0, 0x00, 0x0A,
    0xD0, 0xE1, 0xE8, 0xD0, 0xDC, 0xEE, 0x8C, 0x02,
    0x4C, 0x71, 0x02, 0x8A, 0x18, 0x6D, 0x8B, 0x02,
    0x8D, 0x8B, 0x02, 0x90, 0x03, 0xEE, 0x8C, 0x02,
    0xA9, 0x00, 0x85, 0xEB, 0xA5, 0xFB, 0x85, 0xEC,
    0xA9, 0xFF, 0xA4, 0xFA, 0x51, 0xEB, 0xA6, 0xEC,
    0xE4, 0xFD, 0xB0, 0x08, 0xC8, 0xD0, 0xF5, 0xE6,
    0xEC, 0x4C, 0xB4, 0x02, 0xC8, 0xF0, 0x06, 0xC4,
    0xFC, 0x90, 0xE9, 0xF0, 0xE7, 0x85, 0xFE, 0xA0,
    0x01, 0x51, 0xFC, 0xD0, 0x03, 0x4C, 0xDF, 0x02,
    0xA9, 0xFA, 0xA0, 0x02, 0x4C, 0xE3, 0x02, 0xA9,
    0xF7, 0xA0, 0x02, 0x85, 0xEB, 0x84, 0xEC, 0xA0,
    0xFF, 0xC8, 0xB1, 0xEB, 0xF0, 0x08, 0x09, 0x80,
    0x20, 0xED, 0xFD, 0x4C, 0xE9, 0x02, 0x60, 0x4F,
    0x4B, 0x00, 0x4B, 0x4F, 0x00
};

void usage()
{
    fprintf(stderr, "\nVersion %s\n\n", VERSION);
    fprintf(stderr,
            "c2t [-a] [-f] [-n] [-8] [-r rate] [-s start] infile.hex\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "-8: 8 bits, default 16 bits\n");
    fprintf(stderr, "-a: applesoft binary (implies -b)\n");
    fprintf(stderr, "-b: binary file (intel hex by default)\n");
    fprintf(stderr, "-f: fast load (need load8000)\n");
    fprintf(stderr, "-g: get load8000\n");
    fprintf(stderr, "-l: locked basic program (implies -a -b)\n");
    fprintf(stderr, "-n: dry run\n");
    fprintf(stderr, "-r: rate 48000/44100/22050/11025/8000\n");
    fprintf(stderr, "-s: start of program : gives monitor command\n");
}

void appendtone(uint32_t freq, uint16_t rate, uint32_t time, uint32_t cycles,
                uint8_t bits)
{
    uint32_t i;
    static uint32_t offset = 0;
    uint32_t n =
        time > 0 ? time * rate : freq >
        0 ? (cycles * rate) / (2 * freq) : cycles;
    for (i = 0; i < n; i++) {
        int16_t value = ((2 * i * freq) / rate + offset) % 2;
        if (bits == 16) {
            int16_t v = value ? 0x6666 : -0x6666;
            putchar(v & 0xff);
            putchar(v >> 8 & 0xff);
        } else {
            uint8_t v = (int8_t) 0xcc * value + 0x1a;   // $80 +- $66 = 80% 7F
            putchar(v);
        }
    }

    if (cycles % 2) {
        offset = (offset == 0);
    }
}

void writebyte(uint8_t byte, uint16_t rate, uint8_t bits, uint8_t fast)
{
    uint8_t j;
    uint32_t freq0 = fast ? 12000 : 2000;
    uint32_t freq1 = fast ? 6000 : 1000;
    for (j = 0; j < 8; j++) {
        if (byte & 0x80)
            appendtone(freq1, rate, 0, 2, bits);
        else
            appendtone(freq0, rate, 0, 2, bits);
        byte <<= 1;
    }
}

void header(uint16_t rate, uint8_t bits, uint8_t fast)
{
    if (fast) {
        appendtone(2000, rate, 0, 500, bits);
    } else {
        appendtone(770, rate, 4, 0, bits);
        appendtone(2500, rate, 0, 1, bits);
        appendtone(2000, rate, 0, 1, bits);
    }
}

void buff2wav(uint8_t * data, uint32_t length, uint16_t rate, uint8_t bits,
              uint8_t fast)
{
    header(rate, bits, fast);
    uint32_t j;
    uint8_t checksum = 0xff;
    for (j = 0; j < length; j++) {
        writebyte(data[j], rate, bits, fast);
        checksum ^= data[j];
    }
    writebyte(checksum, rate, bits, fast);
    if (fast) {
        appendtone(770, rate, 0, 16, bits);
    } else {
        appendtone(1000, rate, 0, 2, bits);
    }
}

ihex_bool_t
ihex_data_read(struct ihex_state *ihex,
               ihex_record_type_t type, ihex_bool_t error)
{
    error = error || (ihex->length < ihex->line_length);

    if (type == IHEX_DATA_RECORD && !error) {
        uint32_t address = (uint32_t) IHEX_LINEAR_ADDRESS(ihex);
        if (address < address_offset) {
            if (address_offset == AUTODETECT_ADDRESS) {
                address_offset = address;
            }
        }
        address -= address_offset;
        if (address != file_position) {
            if (file_position < address) {
                do {
                    data[ptr++] = '\0';
                } while (++file_position < address);
            }
            file_position = address;
        }
        memcpy(data + ptr, ihex->data, ihex->length);
        ptr += ihex->length;
        file_position += ihex->length;
    }
    return !error;
}

int main(int argc, char *argv[])
{
    FILE *ifp;
    int16_t c;
    uint32_t length;
    uint16_t rate = 48000;
    uint8_t bits = 16;
    uint8_t applesoft = 0;
    uint8_t fake = 0;
    uint8_t fast = 0;
    uint8_t binary = 0;
    uint8_t lock = 0;
    uint8_t get_load = 0;
    int32_t start = -1;
    char *infilename;
    char buf[256];
    address_offset = AUTODETECT_ADDRESS;
    opterr = 1;
    while ((c = getopt(argc, argv, "abfghlnr:s:8")) != -1) {
        switch (c) {
        case 'a':
            applesoft = 1;
            binary = 1;
            break;
        case 'b':
            binary = 1;
            break;
        case 'f':
            fast = 1;
            break;
        case 'g':
            get_load = 1;
            break;
        case 'h':              // version
            usage();
            return 0;
            break;
        case 'l':
            applesoft = 1;
            binary = 1;
            lock = 1;
            break;
        case 'n':
            fake = 1;
            break;
        case 'r':
            rate = atoi(optarg);
            break;
        case 's':
            start = strtol(optarg, NULL, 16);
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

    if (infilename[0] == '-') {
        ifp = stdin;
    } else if ((ifp = fopen(infilename, "rb")) == NULL) {
        fprintf(stderr, "Cannot read: %s\n\n", infilename);
        return 4;
    }

    if (binary) {
        length = fread(data, 1, 65536, ifp);
    } else {
        struct ihex_state ihex;
        ihex_read_at_address(&ihex, 0);
        while (fgets(buf, sizeof(buf), ifp)) {
            ihex_read_bytes(&ihex, buf, (ihex_count_t) strlen(buf));
        }
        ihex_end_read(&ihex);
        start = address_offset;
        length = ptr;
    }
    fclose(ifp);

    if (start >= 0) {
        uint32_t end = start + length - 1;
        fprintf(stderr, "%d bytes read from %s\n", length, infilename);
        fprintf(stderr, "] CALL -151\n");
        if (fast) {
            if (get_load) {
                fprintf(stderr, "* FA.FDR %X.%lXR %XG\n", start_load8000,
                        start_load8000 + sizeof(load8000) - 1, start_load8000);
            } else {
                fprintf(stderr, "* FA.FDR %XG\n", start_load8000);
            }
            fprintf(stderr, "* %X.%X\n", start, end);
            fprintf(stderr, "* %XG\n", start);
        } else {
            fprintf(stderr, "* %X.%XR\n", start, end);
        }
    }

    if (fake) {
        return 0;
    }

    if (applesoft) {
        uint8_t array[3] = {
            (length - 1) & 0xff,
            (length - 1) >> 8 & 0xff,
            lock ? 0xD5 : 0x55
        };
        buff2wav(array, sizeof(array), rate, bits, fast);
    }

    if (fast) {
        uint32_t end = start + length - 1;
        uint8_t array[4] = {
            start & 0xff,
            start >> 8,
            end & 0xff,
            end >> 8
        };
        buff2wav(array, sizeof(array), rate, bits, 0);
        if (get_load) {
            buff2wav(load8000, sizeof(load8000), rate, bits, 0);
        }
    }
    buff2wav(data, length, rate, bits, fast);
    return 0;
}
