// save_file.h
#ifndef SAVE_FILE_H
#define SAVE_FILE_H

//includes
#include <3ds.h>

//variables


//methods
void saveRGBToPPM(const char *filename, uint8_t *rgb, uint16_t width, uint16_t height);
void read_from_file(char* serverAddress, char* token);

#endif