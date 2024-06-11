/* SPDX-License-Identifier: GPL-3.0
 
  Author : Per Hallsmark <per@hallsmark.se>
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <raylib.h>

#include "resources.h"

static GstElement *pVideoInput;
static GstElement *pVideoApp;
static GstElement *pVideoRecording;
static GstElement *sink;
static GstBus *busRecording;
static GstPad *sinkRecording;
static RenderTexture2D renderTexture;

static int monitorWidth = 0;
static int monitorHeight = 0;
static int screenWidth = 1280;
static int screenHeight = 720;
static char windowTitle[] = "Nifty video/GUI demo using v4l2, gstreamer and raylib";
static int pausing = 0;
static int playback = 0;
static int recording = 0;
static gint64 position;
static gint64 duration;
static struct timeval timeofday;

//#define WEBCAM_LAPTOP
#define WEBCAM_PLEXGEAR

static GstElement *createPipelineVideoInput(const char *videodev, const char *channel)
{
	GError *error;

#ifdef WEBCAM_LAPTOP
	gchar *pipelineString = g_strdup_printf("v4l2src device=%s ! clockoverlay time-format=\"%%Y-%%m-%%d  %%T\" halignment=center valignment=bottom ! intervideosink channel=%s",
#elif defined(WEBCAM_PLEXGEAR)
	gchar *pipelineString = g_strdup_printf("v4l2src device=%s ! decodebin ! clockoverlay time-format=\"%%Y-%%m-%%d  %%T\" halignment=center valignment=bottom ! intervideosink channel=%s",
#endif
		videodev, channel);
	GstElement *pipeline = gst_parse_launch(pipelineString, &error);

	if (error != NULL) {
		printf("Something goes wrong with pipeline initialization: %s\n", error->message);
		g_clear_error(&error);
		exit(-1);
	}

	g_free(pipelineString);
	return pipeline;
}

static GstElement *createPipelineVideoApp(const char *channel)
{
	GError *error;
#ifdef WEBCAM_LAPTOP
	gchar *pipelineString = g_strdup_printf("intervideosrc channel=%s ! queue ! videoconvert ! video/x-raw,width=640,height=480,framerate=30/1,format=RGBA ! appsink name=output max-buffers=2 drop=true",
#elif defined(WEBCAM_PLEXGEAR)
	gchar *pipelineString = g_strdup_printf("intervideosrc channel=%s ! queue ! videoconvert ! video/x-raw,width=1920,height=1080,framerate=30/1,format=RGBA ! appsink name=output max-buffers=2 drop=true",
#endif
		channel);
	GstElement *pipeline = gst_parse_launch(pipelineString, &error);

	if (error != NULL) {
		printf("Something goes wrong with pipeline initialization: %s\n", error->message);
		g_clear_error(&error);
		exit(-1);
	}

	g_free(pipelineString);
	return pipeline;
}

static GstElement *createPipelineVideoRecording(const char *channel, const char *filename)
{
	GError *error;
	gchar *pipelineString = g_strdup_printf("intervideosrc channel=%s ! queue ! videoconvert ! x264enc ! h264parse ! queue ! mp4mux ! filesink location=%s",
		channel, filename);
	GstElement *pipeline = gst_parse_launch(pipelineString, &error);

	if (error != NULL) {
		printf("Something goes wrong with pipeline initialization: %s\n", error->message);
		g_clear_error(&error);
		exit(-1);
	}

	g_free(pipelineString);
	return pipeline;
}

static void gstInit(const char *videodev, const char *recordfile)
{
	pVideoInput = createPipelineVideoInput(videodev, videodev);
	pVideoApp = createPipelineVideoApp(videodev);
	pVideoRecording = createPipelineVideoRecording(videodev, recordfile);
	sink = gst_bin_get_by_name(GST_BIN(pVideoApp), "output");
}

static void raylibInit(void)
{
	SetTraceLogLevel(LOG_ERROR);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, windowTitle);
	monitorWidth = GetMonitorWidth(0);
	monitorHeight = GetMonitorHeight(0);
	//printf("monitorWidth %d, monitorHeight %d\n", monitorWidth, monitorHeight);
	loadResources();
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
		vector.y = screenHeight-50;
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

	colorButton = Fade(WHITE, 0.5f);

	vector.x = 50;
	vector.y = 30;
	textButton(niceFont, "(P)ause ", vector, 20, 1, BLUE, colorButton);

	vector.y = 70;
	if (recording) {
		textButton(niceFont, "Recording", vector, 20, 1, BLUE, colorButton);
	} else {
		textButton(niceFont, "(R)ecord", vector, 20, 1, BLUE, colorButton);
	}
}

int main(int argc, char *argv[])
{
	int frameWidth = 0, frameHeight = 0;
	int renderWidth = 0, renderHeight = 0;
	float renderScale = -1.0f;
	int status = 0;
	GstMapInfo mapInfo;
	GstState state;
	Vector2 vector;
	char str[64];
	Color color;
	int key_pressed=0;

	gst_init(&argc, &argv);

	gstInit(argv[1], argv[2]);

	raylibInit();

	gst_element_set_state(pVideoInput, GST_STATE_PLAYING);
	gst_element_set_state(pVideoApp, GST_STATE_PLAYING);

	while (!WindowShouldClose()) {
		gst_element_query_position(pVideoInput, GST_FORMAT_TIME, &position);
		gst_element_query_duration(pVideoInput, GST_FORMAT_TIME, &duration);
		gettimeofday(&timeofday, NULL);
		
		key_pressed = GetKeyPressed();

		switch (key_pressed) {
			case 0:
				break;

			case KEY_P: // Pause
				gst_element_get_state(pVideoApp, &state, NULL, 1);

				if (state == GST_STATE_PLAYING) {
					pausing = 1;
					gst_element_set_state(pVideoApp, GST_STATE_PAUSED);
				} else {
					pausing = 0;
					gst_element_set_state(pVideoApp, GST_STATE_PLAYING);
				}
				break;

			case KEY_R: // Record
				if (recording) {
					recording = 0;
					gst_element_send_event(pVideoRecording, gst_event_new_eos());
				} else {
					recording = 1;
					gst_element_set_state(pVideoRecording, GST_STATE_PLAYING);
				}
				break;

			case KEY_S: // Stream
				break;

			default:
				printf("key pressed %d\n", key_pressed);
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			vector = GetMousePosition();
			if (vector.x >= 5 && vector.x <= screenWidth - 5
			    && vector.y >= screenHeight - 15 && vector.y <= screenHeight - 5)
				gst_element_seek_simple(pVideoApp, GST_FORMAT_TIME,
							GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_FLUSH,
							(gint64) ((1.0f * (vector.x - 5) / (screenWidth - 10)) * duration));
		}

		BeginDrawing();

		ClearBackground(WHITE);

		GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 1);

		if (IsWindowResized())
			renderScale = -1.0f;

		if (sample != NULL && frameWidth == 0) {
			GstCaps *caps = gst_sample_get_caps(sample);
			GstStructure *s = gst_caps_get_structure(caps, 0);

			gst_structure_get_int(s, "width", &frameWidth);
			gst_structure_get_int(s, "height", &frameHeight);
			//printf("frameWidth %d, frameHeight %d\n", frameWidth, frameHeight);

			renderTexture = LoadRenderTexture(frameWidth, frameHeight);
		}
		if (renderScale < 0 && frameWidth > 0) {
			screenWidth = GetScreenWidth();
			screenHeight = GetScreenHeight();
			//printf("screenWidth %d, screenHeight %d\n", screenWidth, screenHeight);

			renderWidth = screenWidth;
			renderHeight = (int)(screenWidth * 1.0f * frameHeight / frameWidth);

			if (renderHeight > screenHeight) {
				renderHeight = screenHeight;
				renderWidth = (int)(screenHeight * 1.0f * frameWidth / frameHeight);
			}
			//printf("renderWidth %d, renderHeight %d\n", renderWidth, renderHeight);

			renderScale = 1.0f * renderWidth / frameWidth;
		}
		if (sample != NULL) {
			GstBuffer *buffer = gst_sample_get_buffer(sample);
			GstMapInfo mapInfo;

			if (gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
				UpdateTexture(renderTexture.texture, mapInfo.data);
				gst_buffer_unmap(buffer, &mapInfo);
			}

			gst_sample_unref(sample);
		}

		vector.x = (screenWidth - renderWidth) / 2;
		vector.y = (screenHeight - renderHeight) / 2;

		DrawTextureEx(renderTexture.texture, vector, 0, 1.0f * renderWidth / frameWidth, WHITE);

		drawGUI_with_icons();
		drawGUI_with_text();


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

		EndDrawing();
	}

	return 0;
}
