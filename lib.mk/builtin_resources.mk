build/builtin_resources.c build/builtin_resources.h: scripts/builtin_resources.py
	$< $@

clean::
	$(RM) build/builtin_resources.c build/builtin_resources.h
