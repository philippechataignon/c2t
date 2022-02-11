#define LZ4_MEMORY_USAGE 20
#include <stdio.h>   // For printf()
#include <string.h>  // For memcmp()
#include <stdlib.h>  // For exit()
#include <errno.h>     // This is all that is required to expose the prototypes for basic compression and decompression.
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

    /* compress */
    FILE* const inpFp = fopen(inpFilename, "rb");
    if (inpFp == NULL) {
        fprintf(stderr, "Error %d (%s): %s\n", errno, inpFilename, strerror(errno));
        return errno;
    }

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
