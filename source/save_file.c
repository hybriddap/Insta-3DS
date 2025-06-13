#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <setjmp.h>
#include <3ds.h>
#include <sys/dirent.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <stdbool.h>

void rotate_ppm_left(const char* input, const char* output) {
    FILE* f = fopen(input, "rb");
    if (!f) {
        perror("Input open failed");
        return;
    }

    // Read header
    char magic[3];
    int width, height, maxval;
    fscanf(f, "%2s\n%d %d\n%d\n", magic, &width, &height, &maxval);

    if (strcmp(magic, "P6") != 0 || maxval != 255) {
        printf("Unsupported format\n");
        fclose(f);
        return;
    }

    int pixels = width * height;
    unsigned char* data = malloc(3 * pixels);
    fread(data, 1, 3 * pixels, f);
    fclose(f);

    // Allocate rotated buffer (90° left: size becomes height x width)
    unsigned char* rotated = malloc(3 * pixels);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int src_idx = (y * width + x) * 3;
            int dst_x = y;
            int dst_y = width - 1 - x;
            int dst_idx = (dst_y * height + dst_x) * 3;

            rotated[dst_idx] = data[src_idx];
            rotated[dst_idx + 1] = data[src_idx + 1];
            rotated[dst_idx + 2] = data[src_idx + 2];
        }
    }

    // Write rotated image
    FILE* out = fopen(output, "wb");
    fprintf(out, "P6\n%d %d\n255\n", height, width);  // flipped dims
    fwrite(rotated, 1, 3 * pixels, out);
    fclose(out);

    free(data);
    free(rotated);
}

void saveRGBToPPM(const char *filename, uint8_t *rgb, uint16_t width, uint16_t height) {
    FILE *f = fopen(filename, "wb");
    if (!f) { perror("fopen"); return; }

    //PPM header
    fprintf(f, "P6\n%u %u\n255\n", width, height);

    //RGB pixel data
    fwrite(rgb, 3, (size_t)width * height, f);

    fflush(f);
    fclose(f);
    rotate_ppm_left(filename,"output2.ppm");
}