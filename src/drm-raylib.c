#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm/drm_fourcc.h>
#include <raylib.h>

#include "resources.h"

static RenderTexture2D renderTexture;

static char windowTitle[] = "Nifty GUI demo using DRM and raylib";
static int pausing = 0;
static int playback = 0;
static int recording = 0;
static struct timeval timeofday;

int screenWidth = 3300;
int screenHeight = 2200;

#define MAX_GESTURE_STRINGS 0

static void raylibInit(void)
{
	SetTraceLogLevel(LOG_ERROR);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, windowTitle);
#if MAX_GESTURE_STRINGS
	SetGesturesEnabled(GESTURE_TAP, GESTURE_DRAG);
#endif
	SetTargetFPS(60);
}

static void textButton(Font font, const char *str, Vector2 vector, float fontSize, float spacing, Color colorText, Color colorButton)
{
	Vector2 txtbox;
	txtbox = MeasureTextEx(font, str, fontSize, spacing);
	DrawRectangle(vector.x, vector.y, txtbox.x+10, txtbox.y+10, colorButton);
	vector.x += 5;
	vector.y += 5;
	DrawTextEx(font, str, vector, fontSize, spacing, colorText);
}

static void drawGUI_with_icons(void)
{
	Color color;
	Vector2 vector;
	int show_recording;

	vector.x = 10;
	vector.y = 25;
	color = Fade(WHITE, 0.5f);
	if (pausing)
		DrawTexture(icon_pause, vector.x, vector.y, color);
	else
		DrawTexture(icon_play, vector.x, vector.y, color);

	if (recording) {
		vector.x = screenWidth/2;
		vector.y = screenHeight-100;
		if (!(timeofday.tv_sec%2)) {
			show_recording = !show_recording;
		}
		if (show_recording)
			DrawTexture(icon_recording, vector.x, vector.y, WHITE);
	}
}

static void drawGUI_with_text(void)
{
	Color colorButton;
	Vector2 vector;

	colorButton = Fade(BLACK, 0.5f);

	vector.x = 200;
	vector.y = 30;
	if (pausing) {
		textButton(niceFont, "Pausing ", vector, 50, 1, BLUE, colorButton);
	} else {
		textButton(niceFont, "(P)ause ", vector, 50, 1, BLUE, colorButton);
	}

	vector.y = 70;
	if (recording) {
		textButton(niceFont, "Recording", vector, 50, 1, BLUE, colorButton);
	} else {
		textButton(niceFont, "(R)ecord", vector, 50, 1, BLUE, colorButton);
	}
}

int main(int argc, char *argv[])
{
	int frameWidth = 0, frameHeight = 0;
	int renderWidth = 0, renderHeight = 0;
	float renderScale = -1.0f;
	int status = 0;
	Vector2 vector;
	char str[64];
	Color color;
	int key_pressed=0;
	int i;
	//int touches=0;
	//Vector2 touchPosition[MAX_TOUCHES] = {0};
#if MAX_GESTURE_STRINGS
	int gestures = 0;
	int currentGesture = GESTURE_NONE;
	int lastGesture = GESTURE_NONE;
#endif
        struct framebuffer *fb;

	//DrmInit(DRM_DIR_NAME "card1"); surprise... made in raylibInit when built for platform drm :-D

	raylibInit();

	loadResources();

	while (!WindowShouldClose()) {
		gettimeofday(&timeofday, NULL);
		
		key_pressed = GetKeyPressed();

		switch (key_pressed) {
			case 0:
				break;

			case KEY_P: // Pause
				pausing = !pausing;
				break;

			case KEY_R: // Record
				recording = !recording;
				break;

			case KEY_S: // Stream
				break;

			default:
				printf("key pressed %d\n", key_pressed);
		}

#if 0
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			vector = GetMousePosition();
			if (vector.x >= 5 && vector.x <= screenWidth - 5
			    && vector.y >= screenHeight - 15 && vector.y <= screenHeight - 5)
				gst_element_seek_simple(pVideoApp, GST_FORMAT_TIME,
							GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_FLUSH,
							(gint64) ((1.0f * (vector.x - 5) / (screenWidth - 10)) * duration));
		}
#endif

#if 0
		touches = GetTouchPointCount();
		printf("touches %d\n", touches);
		if (touches>MAX_TOUCHES)
			touches = MAX_TOUCHES;
		for (i=0; i<touches; i++)
			touchPosition[i] = GetTouchPosition(i);
#endif

		BeginDrawing();

		ClearBackground(BLACK);

		vector.x = screenWidth / 2;
		vector.y = screenHeight / 2;
		DrawTextureEx(renderTexture.texture, vector, 0, 10, WHITE);

		drawGUI_with_icons();
		drawGUI_with_text();


#if 0
		// overlay progressbar, during playback only
		if (playback) {
			color = Fade(WHITE, 0.5f);
			DrawRectangle(5, screenHeight - 15, screenWidth - 10, 10, color);
			color = Fade(BLUE, 0.5f);
			DrawRectangle(5, screenHeight - 15, (int)((screenWidth - 10) * (1.0f * position / duration)), 10, color);
			vector.y = screenHeight - 40;
			vector.x = 5;
			sprintf(str, "Time: %" GST_TIME_FORMAT, GST_TIME_ARGS(position));
			DrawTextEx(niceFont, str, vector, 20, 1, GRAY);
		}
#endif

#if MAX_GESTURE_STRINGS
		for (i=0; i<touches; i++) {
			if ((touchPosition[i].x > 0) && (touchPosition[i].y > 0)) {
				DrawCircleV(touchPosition[i], 34, WHITE);
				DrawText(TextFormat("%d", i), (int)touchPosition[i].x - 10, (int)touchPosition[i].y - 70, 40, BLACK);

			}
		}
#endif

#ifndef MAX_GESTURE_STRINGS
		vector.x = GetTouchX();
		vector.y = GetTouchY();
		if ((vector.x>0) && (vector.y>0)) {
				DrawCircleV(vector, 34, WHITE);
				DrawText(TextFormat("%d", i), vector.x - 10, vector.y - 70, 40, BLACK);
		}
#endif

		EndDrawing();
	}

	return 0;
}
