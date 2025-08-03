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

#include "video.h"
#include "save_file.h"
#include "http_requests.h"

static jmp_buf exitJmp;

inline void clearScreen(void) {
	u8 *frame = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	memset(frame, 0, 320 * 240 * 3);
}

void hang(char *message) {
	clearScreen();
	printf("%s", message);
	printf("Press start to exit");

	while (aptMainLoop()) {
		hidScanInput();

		u32 kHeld = hidKeysHeld();
		if (kHeld & KEY_START) longjmp(exitJmp, 1);
	}
}

void cleanup() {
	camExit();
	gfxExit();
	acExit();
}

void writePictureToFramebufferRGB565(void *fb, void *img, u16 x, u16 y, u16 width, u16 height) {
	u8 *fb_8 = (u8*) fb;
	u16 *img_16 = (u16*) img;
	int i, j, draw_x, draw_y;
	for(j = 0; j < height; j++) {
		for(i = 0; i < width; i++) {
			draw_y = y + height - j;
			draw_x = x + i;
			u32 v = (draw_y + draw_x * height) * 3;
			u16 data = img_16[j * width + i];
			uint8_t b = ((data >> 11) & 0x1F) << 3;
			uint8_t g = ((data >> 5) & 0x3F) << 2;
			uint8_t r = (data & 0x1F) << 3;
			fb_8[v] = r;
			fb_8[v+1] = g;
			fb_8[v+2] = b;
		}
	}
}

u8 * takePictureFromFramebufferRGB565(void *fb, void *img, u16 x, u16 y, u16 width, u16 height) {
	u8 *fb_8 = (u8*) fb;
	u16 *img_16 = (u16*) img;
	int i, j, draw_x, draw_y;
	for(j = 0; j < height; j++) {
		for(i = 0; i < width; i++) {
			draw_y = y + height - j;
			draw_x = x + i;
			u32 v = (draw_y + draw_x * height) * 3;
			u16 data = img_16[j * width + i];
			uint8_t b = ((data >> 11) & 0x1F) << 3;
			uint8_t g = ((data >> 5) & 0x3F) << 2;
			uint8_t r = (data & 0x1F) << 3;
			fb_8[v] = r;
			fb_8[v+1] = g;
			fb_8[v+2] = b;
		}
	}
    return fb_8;
}

void changeCams(bool useInnerCam){
	if(useInnerCam)
	{
		CAMU_Activate(SELECT_NONE);
		CAMU_Activate(SELECT_IN1);
	}
	else
	{
		CAMU_Activate(SELECT_NONE);
		CAMU_Activate(SELECT_OUT1_OUT2);
	}
	CAMU_PlayShutterSound(SHUTTER_SOUND_TYPE_MOVIE_END);
}

// TODO: Figure out how to use CAMU_GetStereoCameraCalibrationData
void takePicture3D(u8 *buf) {
	u32 bufSize;
	printf("CAMU_GetMaxBytes: 0x%08X\n", (unsigned int) CAMU_GetMaxBytes(&bufSize, WIDTH, HEIGHT));
	printf("CAMU_SetTransferBytes: 0x%08X\n", (unsigned int) CAMU_SetTransferBytes(PORT_BOTH, bufSize, WIDTH, HEIGHT));

	printf("CAMU_Activate: 0x%08X\n", (unsigned int) CAMU_Activate(SELECT_OUT1_OUT2));

	Handle camReceiveEvent = 0;
	Handle camReceiveEvent2 = 0;

	printf("CAMU_ClearBuffer: 0x%08X\n", (unsigned int) CAMU_ClearBuffer(PORT_BOTH));
	printf("CAMU_SynchronizeVsyncTiming: 0x%08X\n", (unsigned int) CAMU_SynchronizeVsyncTiming(SELECT_OUT1, SELECT_OUT2));

	printf("CAMU_StartCapture: 0x%08X\n", (unsigned int) CAMU_StartCapture(PORT_BOTH));

	printf("CAMU_SetReceiving: 0x%08X\n", (unsigned int) CAMU_SetReceiving(&camReceiveEvent, buf, PORT_CAM1, SCREEN_SIZE, (s16) bufSize));
	printf("CAMU_SetReceiving: 0x%08X\n", (unsigned int) CAMU_SetReceiving(&camReceiveEvent2, buf + SCREEN_SIZE, PORT_CAM2, SCREEN_SIZE, (s16) bufSize));
	printf("svcWaitSynchronization: 0x%08X\n", (unsigned int) svcWaitSynchronization(camReceiveEvent, WAIT_TIMEOUT));
	printf("svcWaitSynchronization: 0x%08X\n", (unsigned int) svcWaitSynchronization(camReceiveEvent2, WAIT_TIMEOUT));
	printf("CAMU_PlayShutterSound: 0x%08X\n", (unsigned int) CAMU_PlayShutterSound(SHUTTER_SOUND_TYPE_NORMAL));

	printf("CAMU_StopCapture: 0x%08X\n", (unsigned int) CAMU_StopCapture(PORT_BOTH));

	svcCloseHandle(camReceiveEvent);
	svcCloseHandle(camReceiveEvent2);

	printf("CAMU_Activate: 0x%08X\n", (unsigned int) CAMU_Activate(SELECT_NONE));
}

int videoLoop() {

	// Save current stack frame for easy exit
	if(setjmp(exitJmp)) {
		cleanup();
		return 0;
	}

	u32 kDown;
    u32 kUp;
    bool useInnerCam = false;
    bool btnDebounce=false;

	printf("Initializing camera\n");
	camInit();

    //Inner Setup
	CAMU_SetSize(SELECT_IN1, SIZE_CTR_TOP_LCD, CONTEXT_A);
	CAMU_SetOutputFormat(SELECT_IN1, OUTPUT_RGB_565, CONTEXT_A);
	CAMU_SetFrameRate(SELECT_IN1, FRAME_RATE_30);
	CAMU_SetNoiseFilter(SELECT_IN1, true);
	CAMU_SetAutoExposure(SELECT_IN1, true);
	CAMU_SetAutoWhiteBalance(SELECT_IN1, true);

    //OuterSetup
	CAMU_SetSize(SELECT_OUT1_OUT2, SIZE_CTR_TOP_LCD, CONTEXT_A);
	CAMU_SetOutputFormat(SELECT_OUT1_OUT2, OUTPUT_RGB_565, CONTEXT_A);
	CAMU_SetFrameRate(SELECT_OUT1_OUT2, FRAME_RATE_30);
	CAMU_SetNoiseFilter(SELECT_OUT1_OUT2, true);
	CAMU_SetAutoExposure(SELECT_OUT1_OUT2, true);
	CAMU_SetAutoWhiteBalance(SELECT_OUT1_OUT2, true);


	CAMU_SetTrimming(PORT_CAM1, false);
	CAMU_SetTrimming(PORT_CAM2, false);

	u8 *buf = malloc(BUF_SIZE);
	if(!buf) {
		hang("Failed to allocate memory!");
	}

	u32 bufSize;
	CAMU_GetMaxBytes(&bufSize, WIDTH, HEIGHT);
	CAMU_SetTransferBytes(PORT_BOTH, bufSize, WIDTH, HEIGHT);

	CAMU_Activate(SELECT_OUT1_OUT2);

	Handle camReceiveEvent[4] = {0};
	bool captureInterrupted = false;
	s32 index = 0;

	// events 0 and 1 for interruption
	CAMU_GetBufferErrorInterruptEvent(&camReceiveEvent[0], PORT_CAM1);
	CAMU_GetBufferErrorInterruptEvent(&camReceiveEvent[1], PORT_CAM2);

	CAMU_ClearBuffer(PORT_BOTH);
	CAMU_SynchronizeVsyncTiming(SELECT_OUT1, SELECT_OUT2);

	CAMU_StartCapture(PORT_BOTH);
	// printf("CAMU_PlayShutterSound: 0x%08X\n", (unsigned int) CAMU_PlayShutterSound(SHUTTER_SOUND_TYPE_MOVIE));

	gfxFlushBuffers();
	gspWaitForVBlank();
	gfxSwapBuffers();

	printf("\nUse slider to enable/disable 3D\n");
	printf("Press Start to exit to Homebrew Launcher\n");

	// Main loop
	while (aptMainLoop()) {

		if (!captureInterrupted) {
			// Read which buttons are currently pressed or not
			hidScanInput();
			kDown = hidKeysDown();
            kUp = hidKeysUp();
            
			// If START button is pressed, break loop and quit
			if (kDown & KEY_START) {
				break;
			}
            
            if(kDown & KEY_A){
				btnDebounce=true;
			}
			else if(kUp & KEY_A){
				if(btnDebounce){
					sleep(1);
					btnDebounce=false;
					useInnerCam=!useInnerCam;
					changeCams(useInnerCam);
				}
			}

            if(kDown & KEY_R){
                printf("Taking photo...\n");
                u8* picture=takePictureFromFramebufferRGB565(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), buf, 0, 0, WIDTH, HEIGHT);
                saveRGBToPPM("output.ppm",picture, HEIGHT,WIDTH);
                printf("Successfully took photo!\n");
				//Upload Logic
            }
		}

		// events 2 and 3 for capture
		if (camReceiveEvent[2] == 0) {
			CAMU_SetReceiving(&camReceiveEvent[2], buf, PORT_CAM1, SCREEN_SIZE, (s16)bufSize);
		}
		if (camReceiveEvent[3] == 0) {
			CAMU_SetReceiving(&camReceiveEvent[3], buf + SCREEN_SIZE, PORT_CAM2, SCREEN_SIZE, (s16)bufSize);
		}

		if (captureInterrupted) {
			CAMU_StartCapture(PORT_BOTH);
			captureInterrupted = false;
		}

        svcWaitSynchronizationN(&index, camReceiveEvent, 4, false, WAIT_TIMEOUT);
		switch (index) {
		case 0:
			svcCloseHandle(camReceiveEvent[2]);
			camReceiveEvent[2] = 0;

			captureInterrupted = true;
			continue; //skip screen update
			break;
		case 1:
			svcCloseHandle(camReceiveEvent[3]);
			camReceiveEvent[3] = 0;

			captureInterrupted = true;
			continue; //skip screen update
			break;
		case 2:
			svcCloseHandle(camReceiveEvent[2]);
			camReceiveEvent[2] = 0;
			break;
		case 3:
			svcCloseHandle(camReceiveEvent[3]);
			camReceiveEvent[3] = 0;
			break;
		default:
			break;
		}

		if(CONFIG_3D_SLIDERSTATE > 0.0f) {
			gfxSet3D(true);
			writePictureToFramebufferRGB565(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), buf, 0, 0, WIDTH, HEIGHT);
			writePictureToFramebufferRGB565(gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL), buf + SCREEN_SIZE, 0, 0, WIDTH, HEIGHT);
		} else {
			gfxSet3D(false);
			writePictureToFramebufferRGB565(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), buf, 0, 0, WIDTH, HEIGHT);
		}

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gspWaitForVBlank();
		gfxSwapBuffers();
	}
    CAMU_StopCapture(PORT_BOTH);

	// Close camera event handles
	for (int i = 0; i < 4; i++)	{
		if (camReceiveEvent[i] != 0) {
			svcCloseHandle(camReceiveEvent[i]);
		}
	}
    
    CAMU_Activate(SELECT_NONE);
	printf("Shutting off camera!\n");

    // Exit
	free(buf);
	cleanup();

    return 0;
}
