# SPDX-License-Identifier: GPL-3.0
#
# Author : Per Hallsmark <per@hallsmark.se>
#

build/builtin_resources.c build/builtin_resources.h: scripts/builtin_resources.py
	mkdir -p build
	$< $@

clean::
	$(RM) build/builtin_resources.c build/builtin_resources.h
