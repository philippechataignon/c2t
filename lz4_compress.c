#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <lz4hc.h>

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
    const int src_size = fread(data, 1, 65536, inpFp);
    fclose(inpFp);
    const int max_dst_size = LZ4_compressBound(src_size);
    char* compressed_data = (char*)malloc((size_t)max_dst_size);
    const int compressed_data_size = LZ4_compress_HC(data, compressed_data, src_size, max_dst_size, LZ4HC_CLEVEL_MAX);
    FILE* const outFp = fopen(lz4Filename, "wb");
    const int out_size = fwrite(compressed_data, 1, compressed_data_size, outFp);
    fprintf(stderr, "Compress (max: $%x): %s %d->%d\n", max_dst_size, lz4Filename, src_size, out_size);
    fclose(outFp);
    return 0;
}
