#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <raylib.h>

#include "builtin_resources.h"

Font niceFont;

Texture2D icon_pause;
Texture2D icon_pause;
Texture2D icon_play;
Texture2D icon_recording;
Texture2D icon_save;

struct s_icon_worklist {
	const unsigned char *image;
	const int *size;
	Texture2D *texture;
} icon_worklist[] = {
	{ &icons_pause, &icons_pause_size, &icon_pause },
	{ &icons_play, &icons_play_size, &icon_play },
	{ &icons_recording, &icons_recording_size, &icon_recording },
};


static void loadFonts(void)
{
	niceFont = LoadFontFromMemory(".ttf", &fonts_OpenSans_Bold, fonts_OpenSans_Bold_size, 20, NULL, 0);
}

static void loadIcons(void)
{
	int i;
	struct s_icon_worklist *iw;
	Image image;

	for (i=0;;i++) {
		iw = &icon_worklist[i];
		if (!(iw->image))
			break;
		printf("Loading %d\n", iw->image);
		image = LoadImageFromMemory(".png", iw->image, *(iw->size));
		ImageResize(&image, 40, 40);
		*(iw->texture) = LoadTextureFromImage(image);
		UnloadImage(image);
	}
}

void loadResources(void)
{
	loadFonts();
	loadIcons();
}
