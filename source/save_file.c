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
    //Write first ppm file that will be rotated eventually
    FILE *f = fopen(filename, "wb");
    if (!f) { perror("fopen"); return; }

    //PPM header
    fprintf(f, "P6\n%u %u\n255\n", width, height);

    //RGB pixel data
    fwrite(rgb, 3, (size_t)width * height, f);

    fflush(f);
    fclose(f);


    //Need to rotate for some reason it saves rotated 90 degrees otherwise
    rotate_ppm_left(filename,"output.ppm");
}

void create_save_file()
{
    FILE *fptr;

    fptr = fopen("insta_3ds_data.txt", "w");

    fprintf(fptr, "http://daptests.xyz:5001/");
    fprintf(fptr, "\nToken");

    // Close the file
    fclose(fptr);
}

void update_file(char* serverAddress, char* token)
{
    FILE *fptr;
    fptr = fopen("insta_3ds_data.txt", "w");
    
    fprintf(fptr, serverAddress);
    fprintf(fptr, "\n");
    fprintf(fptr, token);

    // Close the file
    fclose(fptr);

    printf("\nUpdated Server Address: %s\n",serverAddress);
	printf("Updated Token: %s\n\n",token);
}

void read_from_file(char* serverAddress, char* token)
{
    // read mode.
    FILE* file = fopen("insta_3ds_data.txt", "r");
    // Buffer to store each line of the file.
    char line[512];
    int i=0;

    // Check if the file was opened successfully.
    if (file != NULL) {
        printf("Found save file.\n");
        // Read each line from the file and store it in the
        // 'line' buffer.
        while (fgets(line, sizeof(line), file)) {
            //Get rid of trailing \n
            int len = strlen(line);
            if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';

            //Actual logic
            if(i==0)
                strcpy(serverAddress, line);
            else if(i==1)
                strcpy(token, line);
            else
                break;  //only 2 lines
            i++;
        }

        // Close the file stream once all lines have been
        // read.
        fclose(file);
    }
    else {
        // Print an error message to the standard error
        // stream if the file cannot be opened.
        printf("No save file found. Creating new save file.\n");
        create_save_file();
        read_from_file(serverAddress,token); //read it again
        return; //return this node
    }
    printf("\nCurrent Server Address: %s\n",serverAddress);
	printf("Current Token: %s\n\n",token);
}