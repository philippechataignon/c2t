/*
 * simple_buffer.c
 * Copyright  : Kyle Harper
 * License    : Follows same licensing as the lz4.c/lz4.h program at any given time.  Currently, BSD 2.
 * Description: Example program to demonstrate the basic usage of the compress/decompress functions within lz4.c/lz4.h.
 *              The functions you'll likely want are LZ4_compress_default and LZ4_decompress_safe.
 *              Both of these are documented in the lz4.h header file; I recommend reading them.
 */

/* Dependencies */
#include <stdio.h>   // For printf()
#include <string.h>  // For memcmp()
#include <stdlib.h>  // For exit()
#include <lz4.h>     // This is all that is required to expose the prototypes for basic compression and decompression.

char data[65536];
int main(int argc, const char **argv) {
    char inpFilename[256] = { 0 };
    char lz4Filename[256] = { 0 };

    if (argc < 2) {
        printf("Please specify input filename\n");
        return 0;
    }

    snprintf(inpFilename, 256, "%s", argv[1]);
    snprintf(lz4Filename, 256, "%s.lz4", argv[1]);

    printf("inp = [%s]\n", inpFilename);
    printf("lz4 = [%s]\n", lz4Filename);

    /* compress */
    FILE* const inpFp = fopen(inpFilename, "rb");
    FILE* const outFp = fopen(lz4Filename, "wb");

    const int src_size = fread(data, 1, 65536, inpFp);
    const int max_dst_size = LZ4_compressBound(src_size);
    // We will use that size for our destination boundary when allocating space.
    char* compressed_data = (char*)malloc((size_t)max_dst_size);
    const int compressed_data_size = LZ4_compress_default(data, compressed_data, src_size, max_dst_size);
    const int out_size = fwrite(compressed_data, 1, compressed_data_size, outFp);
    printf("compress : %s -> %s %d->%d\n", inpFilename, lz4Filename, src_size, out_size);
    fclose(outFp);
    fclose(inpFp);
    return 0;
}
