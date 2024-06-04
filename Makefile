APP=build/v4l2-gstreamer-raylib

SRCS += src/v4l2-gstreamer-raylib.c

CFLAGS += $(shell pkg-config --cflags gstreamer-1.0)
LDFLAGS += $(shell pkg-config --libs gstreamer-1.0)
LDFLAGS += $(shell pkg-config --libs gstreamer-app-1.0)
LDFLAGS += $(shell pkg-config --libs raylib)

all: $(APP)

$(APP): $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

test:
	$(APP) /dev/video0 build/testfil.mp4

clean:
	$(RM) $(APP)
