build/builtin_resources.c build/builtin_resources.h: scripts/builtin_resources.py
	mkdir -p build
	$< $@

clean::
	$(RM) build/builtin_resources.c build/builtin_resources.h
