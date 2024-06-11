# SPDX-License-Identifier: GPL-3.0
#
# Author : Per Hallsmark <per@hallsmark.se>
#

APP=build/v4l2-gstreamer-raylib

SRCS += src/v4l2-gstreamer-raylib.c
SRCS += src/resources.c
SRCS += build/builtin_resources.c

CFLAGS += $(shell pkg-config --cflags gstreamer-1.0)
CFLAGS += -I build
LDFLAGS += $(shell pkg-config --libs gstreamer-1.0)
LDFLAGS += $(shell pkg-config --libs gstreamer-app-1.0)
LDFLAGS += $(shell pkg-config --libs raylib)

all: $(APP)

-include lib.mk/*mk

$(APP): $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean::
	$(RM) $(APP)
