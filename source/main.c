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

int main()
{
	// Initializations
	acInit();
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);

	// Enable double buffering to remove screen tearing
	gfxSetDoubleBuffering(GFX_TOP, true);
	gfxSetDoubleBuffering(GFX_BOTTOM, false);

	videoLoop();

	// Return to hbmenu
	return 0;
}