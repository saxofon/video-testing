APPS += build/drm-raylib
APPS += build/v4l2-gstreamer-raylib

SRCS += src/resources.c
SRCS += build/builtin_resources.c

CFLAGS += -I build
LDFLAGS += $(shell pkg-config --libs raylib)

all: $(APPS)

-include lib.mk/*mk

build/v4l2-gstreamer-raylib: CFLAGS += $(shell pkg-config --cflags gstreamer-1.0)
build/v4l2-gstreamer-raylib: LDFLAGS += $(shell pkg-config --libs gstreamer-1.0)
build/v4l2-gstreamer-raylib: LDFLAGS += $(shell pkg-config --libs gstreamer-app-1.0)
build/v4l2-gstreamer-raylib: src/v4l2-gstreamer-raylib.c $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

build/drm-raylib: CFLAGS += $(shell pkg-config --cflags libdrm)
build/drm-raylib: LDFLAGS += -L build $(shell pkg-config --libs libdrm)
build/drm-raylib: LDFLAGS += -lm -lGLESv2 -lEGL -lgbm
build/drm-raylib: src/drm-raylib.c $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean::
	$(RM) $(APPS)
