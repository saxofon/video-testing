#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <raylib.h>

static int screenWidth = 800;
static int screenHeight = 450;
static char windowTitle[] = "Nifty video/GUI demo using v4l2, gstreamer and raylib";
static GstElement *pVideoInput;
static GstElement *pVideoApp;
static GstElement *pVideoRecording;
static GstElement *sink;
static GstBus *busRecording;
static GstPad *sinkRecording;
static Font niceFont;
static RenderTexture2D renderTexture;

static int pause_enabled=0;
static int progressbar_enabled=0;
static int recording_enabled=0;

static GstElement *createPipelineVideoInput(const char *videodev, const char *channel)
{
	GError *error;
	gchar *pipelineString = g_strdup_printf("v4l2src device=%s ! clockoverlay time-format=\"%%Y-%%m-%%d  %%T\" ! intervideosink channel=%s",
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
	gchar *pipelineString = g_strdup_printf("intervideosrc channel=%s ! queue ! videoconvert ! video/x-raw, format=RGBA ! appsink name=output max-buffers=2 drop=true",
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
	gchar *pipelineString = g_strdup_printf("intervideosrc channel=%s ! queue ! videoconvert ! x264enc ! h264parse ! queue ! qtmux ! filesink location=%s",
		channel, filename);
	GstElement *pipeline = gst_parse_launch(pipelineString, &error);

	if (error != NULL) {
		printf("Something goes wrong with pipeline initialization: %s\n", error->message);
		g_clear_error(&error);
		exit(-1);
	}

	//sinkRecording = gst_element_get_static_pad(filesink, "sink");
	//gst_pad_add_probe(sinkRecording, GstPadProbeType(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM), cb_event_eos, pVideoRecording, NULL)

	//busRecording = gst_pipeline_get_bus(GST_PIPELINE(pVideoRecording));
	//gst_bus_add_signal_watch(bus);
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
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, windowTitle);
	niceFont = LoadFontEx("/usr/share/fonts/open-sans/OpenSans-Bold.ttf", 20, NULL, 0);
}

static void exit_app(void)
{
	gst_element_send_event(pVideoRecording, gst_event_new_eos());

	sleep(1);

	gst_element_set_state(pVideoRecording, GST_STATE_NULL);
	gst_element_set_state(pVideoApp, GST_STATE_NULL);
	gst_element_set_state(pVideoInput, GST_STATE_NULL);

	gst_object_unref(pVideoRecording);
	gst_object_unref(pVideoApp);
	gst_object_unref(pVideoInput);

	gst_deinit();

	UnloadRenderTexture(renderTexture);

	CloseWindow();
}

static void drawMenu(void)
{
	Color color;
	Vector2 vector;

	color = Fade(WHITE, 0.3f);
	DrawRectangle(5, 5, 100, 80, color);
	vector.x = 15;
	vector.y = 15;
	color = Fade(GRAY, 0.4f);
	if (pause_enabled)
		color.a = 255;
	DrawTextEx(niceFont, "(P)ause", vector, 20, 1, color);
	vector.y = 30;
	color = Fade(RED, 0.4f);
	if (recording_enabled)
		color.a = 255;
	DrawTextEx(niceFont, "(R)ecord", vector, 20, 1, color);
}


int main(int argc, char *argv[])
{
	gint64 duration, position;
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

		key_pressed = GetKeyPressed();
		if (key_pressed)
			printf("key pressed %d\n", key_pressed);

		switch (key_pressed) {
			case KEY_P:
				gst_element_get_state(pVideoInput, &state, NULL, 1);

				if (state == GST_STATE_PLAYING) {
					pause_enabled = 1;
					gst_element_set_state(pVideoInput, GST_STATE_PAUSED);
				} else {
					pause_enabled = 0;
					gst_element_set_state(pVideoInput, GST_STATE_PLAYING);
				}
				break;

			case KEY_R:
				if (recording_enabled) {
					recording_enabled = 0;
					gst_element_send_event(pVideoRecording, gst_event_new_eos());
				} else {
					recording_enabled = 1;
					gst_element_set_state(pVideoRecording, GST_STATE_PLAYING);
				}
				break;

			case KEY_Q:
				exit_app();
				break;
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
			printf("width %d, height %d\n", frameWidth,  frameHeight);

			renderTexture = LoadRenderTexture(frameWidth, frameHeight);
		}
		if (renderScale < 0 && frameWidth > 0) {
			screenWidth = GetScreenWidth();
			screenHeight = GetScreenHeight();
			printf("screenWidth %d, screenHeight %d\n", screenWidth,  screenHeight);

			renderWidth = screenWidth;
			renderHeight = (int)(screenWidth * 1.0f * frameHeight / frameWidth);
			printf("renderWidth %d, renderHeight %d\n", screenWidth,  screenHeight);

			if (renderHeight > screenHeight) {
				renderHeight = screenHeight;
				renderWidth = (int)(screenHeight * 1.0f * frameWidth / frameHeight);
			}
			printf("renderWidth %d, renderHeight %d\n", screenWidth,  screenHeight);

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
		drawMenu();

		// overlay progressbar, during playback only
		if (progressbar_enabled) {
			color = Fade(WHITE, 0.5f);
			DrawRectangle(5, screenHeight - 15, screenWidth - 10, 10, color);
			color = Fade(BLUE, 0.5f);
			DrawRectangle(5, screenHeight - 15, (int)((screenWidth - 10) * (1.0f * position / duration)), 10, color);
		}
		//vector.y = screenHeight - 40;
		//vector.x = 5;
		//sprintf(str, "Time: %" GST_TIME_FORMAT, GST_TIME_ARGS(position));
		//DrawTextEx(niceFont, str, vector, 20, 1, GRAY);

		EndDrawing();
	}

	return 0;
}
