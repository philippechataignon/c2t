/*
c2t, Code to Tape

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
    This small utility will read Apple II binary or intel hex files
    and output PCM samples 48000 / S16_LE
    for use with the Apple II cassette interface.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <lz4hc.h>

#include "ihex/kk_ihex_read.h"

#define VERSION "1.2.1"
#define AUTODETECT_ADDRESS UINT16_MAX

/* size of dsk = 16 * 35 *256 */
#define DSKSIZE 143360
#define NBSEG 10
#define SEGSIZE (DSKSIZE / NBSEG)
#define MAXSIZE 48*1024
#define BUFFADDR 0x1000

static uint8_t data[DSKSIZE];
static uint8_t zdata[MAXSIZE];
static uint8_t seglen[2 * NBSEG];
static uint32_t file_position = 0L;
static uint16_t address_offset = 0UL;
static uint16_t ptr = 0;

uint16_t entry_load8000 = 0x2A0;
uint16_t start_load8000 = 0x280;
uint8_t load8000[] = {
    0xA5, 0xFA, 0x8D, 0xC1, 0x02, 0x8D, 0xD2, 0x02,
    0xA5, 0xFB, 0x8D, 0xC2, 0x02, 0x8D, 0xD3, 0x02,
    0xA5, 0xFC, 0x8D, 0xF9, 0x02, 0xA5, 0xFD, 0x8D,
    0xFA, 0x02, 0x4C, 0xA0, 0x02, 0x00, 0x00, 0x00,
    0xA2, 0x00, 0x2C, 0x60, 0xC0, 0x10, 0xFB, 0xA9,
    0x01, 0xA0, 0x00, 0x2C, 0x60, 0xC0, 0x30, 0xFB,
    0xC8, 0x2C, 0x60, 0xC0, 0x10, 0xFA, 0xC0, 0x40,
    0xB0, 0x15, 0xC0, 0x14, 0xB0, 0xEB, 0xC0, 0x07,
    0x3E, 0x00, 0x00, 0x0A, 0xD0, 0xE3, 0xE8, 0xD0,
    0xDE, 0xEE, 0xC2, 0x02, 0x4C, 0xA7, 0x02, 0xA9,
    0xFF, 0x4D, 0x00, 0x00, 0xAA, 0xEE, 0xD2, 0x02,
    0xD0, 0x03, 0xEE, 0xD3, 0x02, 0xAD, 0xF9, 0x02,
    0xCD, 0xD2, 0x02, 0xAD, 0xFA, 0x02, 0xED, 0xD3,
    0x02, 0x8A, 0xB0, 0xE5, 0xF0, 0x0A, 0xA9, 0xCB,
    0x20, 0xED, 0xFD, 0xA9, 0xCF, 0x20, 0xED, 0xFD,
    0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void usage()
{
    fprintf(stderr, "\nVersion %s\n\n", VERSION);
    fprintf(stderr, "c2t [-a] [-f] [-n] [-r rate] [-s start] infile.hex\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "-a: applesoft binary (implies -b)\n");
    fprintf(stderr, "-b: binary file (intel hex by default)\n");
    fprintf(stderr, "-d: dsk mode, must be use with diskload\n");
    fprintf(stderr, "-f: fast load (need load8000)\n");
    fprintf(stderr, "-l: locked basic program (implies -a -b)\n");
    fprintf(stderr, "-m: fast load monitor mode, doesn't send load8000\n");
    fprintf(stderr, "-n: dry run\n");
    fprintf(stderr, "-r: rate 48000/44100/22050/11025/8000\n");
    fprintf(stderr, "-s: start of program : gives monitor command\n");
    fprintf(stderr, "-t: time in seconds to write tracks in dsk mode\n");
}

void appendtone(uint32_t freq, uint16_t rate, uint32_t time,
                uint32_t demi_cycles)
{
    uint32_t i;
    uint32_t n;
    static uint8_t offset = 0;
    if (time > 0) {
        demi_cycles += 2 * time * freq;
    }
    // rate / freq is # of values by cycle, rate / (2 * freq) is # of values by demi_cycle
    // warning : order is important in integer division: keep 2 in denominator
    if (freq > 0) {
        n = (demi_cycles * rate) / (2 * freq);
    } else {
        n = time * rate + demi_cycles / 2;
    }
    for (i = 0; i < n; i++) {
        uint8_t value = (2 * i * freq / rate + offset) % 2;
        uint8_t v = (int8_t) value == 0 ? 16 : 240;
        putchar(v);
    }
    // if demi_cycles is odd, remember if last call was low or high, else reset offset
    if (demi_cycles % 2) {
        offset = (offset == 0);
    } else {
        offset = 0;
    }
}

void writebyte(uint8_t byte, uint16_t rate, uint8_t fast)
{
    uint8_t j;
    uint32_t freq0 = fast ? 12000 : 2000;
    uint32_t freq1 = fast ? 6000 : 1000;
    for (j = 0; j < 8; j++) {
        if (byte & 0x80)
            appendtone(freq1, rate, 0, 2);
        else
            appendtone(freq0, rate, 0, 2);
        byte <<= 1;
    }
}

void buff2wav(uint8_t * data, uint32_t length, uint16_t rate, uint8_t fast)
{
    uint32_t i;
    // header
    if (fast) {
        appendtone(2000, rate, 0, 500);
    } else {
        appendtone(770, rate, 4, 0);
        appendtone(2500, rate, 0, 1);
        appendtone(2000, rate, 0, 1);
    }
    // data
    uint8_t checksum = 0xff;
    for (i = 0; i < length; i++) {
        writebyte(data[i], rate, fast);
        checksum ^= data[i];
    }
    // checksum
    writebyte(checksum, rate, fast);
    // ending
    if (fast) {
        appendtone(770, rate, 0, 16);
    } else {
        appendtone(1000, rate, 0, 2);
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
    char buf[256];
    char *infilename;
    int16_t c;
    int32_t start = -1;
    uint32_t length;
    uint16_t rate = 48000;
    uint8_t applesoft = 0;
    uint8_t binary = 0;
    uint8_t dsk = 0;
    uint8_t fake = 0;
    uint8_t fast = 0;
    uint8_t lock = 0;
    uint8_t monitor = 0;
    uint8_t wait_time = 15;
    address_offset = AUTODETECT_ADDRESS;
    opterr = 1;
    while ((c = getopt(argc, argv, "abdfhlmnr:s:t:")) != -1) {
        switch (c) {
        case 'a':
            applesoft = 1;
            binary = 1;
            fast = 0;
            break;
        case 'b':
            binary = 1;
            break;
        case 'd':
            dsk = 1;
            binary = 1;
            fast = 1;
            break;
        case 'f':
            fast = 1;
            break;
        case 'h':
            usage();
            return 0;
            break;
        case 'l':
            applesoft = 1;
            binary = 1;
            lock = 1;
            break;
        case 'm':
            monitor = 1;
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
        case 't':
            wait_time = atoi(optarg);
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
        fprintf(stderr, "Cannot read: %s\n", infilename);
        return 4;
    }

    if (binary) {
        if (dsk) {
            length = fread(data, 1, DSKSIZE, ifp);
            fprintf(stderr, "DSK mode\n");
            if (length != DSKSIZE) {
                fprintf(stderr, "Incorrect dsk file size : %s %d\n",
                        infilename, ferror(ifp));
                return 5;
            }
        } else {
            length = fread(data, 1, MAXSIZE, ifp);
        }
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
        int32_t end = start + length - 1;
        fprintf(stderr, "Memory range: %X.%X\n", start, end);
        fprintf(stderr, "] CALL -151\n");
        if (fast) {
            if (monitor) {
                end++;
                fprintf(stderr, "* FA:%X %X %X %X N %XG\n",
                        start & 0xff, start >> 8, end & 0xff, end >> 8,
                        start_load8000);
            } else {
                fprintf(stderr, "* %X.%XR %XG\n", start_load8000,
                        start_load8000 + (uint16_t) sizeof(load8000) - 1,
                        entry_load8000);
            }
        } else {
            fprintf(stderr, "* %X.%XR\n", start, end);
        }
    }

    if (fake) {
        return 0;
    }

    if (dsk) {
        uint8_t seg;
        // compress to get len
        for (seg = 0; seg <NBSEG; seg++) {
            const uint16_t l = BUFFADDR + LZ4_compress_HC(
                (char*)data + SEGSIZE * seg,
                (char*)zdata,
                SEGSIZE, MAXSIZE, LZ4HC_CLEVEL_MAX
            );
            seglen[9-seg] = l & 0xff;
            seglen[19-seg] = l >> 8;
        }
        // send param at low speed
        buff2wav((uint8_t*)seglen, sizeof(seglen), rate, 0);
        // send each segment
        for (seg = 0; seg <NBSEG; seg++) {
            const uint16_t zdata_length = LZ4_compress_HC(
                (char*)data + SEGSIZE * seg,
                (char*)zdata,
                SEGSIZE, MAXSIZE, LZ4HC_CLEVEL_MAX
            );
            fprintf(stderr, "Segment %d: %d\n", seg, zdata_length);
            buff2wav(zdata, zdata_length, rate, fast);
            appendtone(0, rate, wait_time, 0);
            // add silence between segments
        }
    } else {
        if (applesoft) {
            uint8_t array[3] = {
                (length - 1) & 0xff,
                (length - 1) >> 8 & 0xff,
                lock ? 0xD5 : 0x55
            };
            buff2wav(array, sizeof(array), rate, fast);
        }

        if (fast & !monitor) {
            uint32_t end = start + length;
            load8000[0x41] = start & 0xff;
            load8000[0x42] = start >> 8;
            load8000[0x52] = start & 0xff;
            load8000[0x53] = start >> 8;
            load8000[0x79] = end & 0xff;
            load8000[0x7A] = end >> 8;
            // load8000 is send at normal speed
            buff2wav(load8000, sizeof(load8000), rate, 0);
        }
        buff2wav(data, length, rate, fast);
    }
    return 0;
}
