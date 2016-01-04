CC 	   ?= clang
CFLAGS += -O2 -std=gnu1x -Wall -Wextra -Wno-format -Wunreachable-code
LIBS   += -lrt

all: sdl2

pkg_config = pkg-config --cflags --libs $(1)

x11: src/mptet.c src/gfxX11.c
	$(CC) $(CFLAGS) -DUSE_X11 src/mptet.c `$(call pkg_config,x11)` -o mptet $(LIBS)

directfb: src/mptet.c src/gfxdirectfb.c
	$(CC) $(CFLAGS) -DUSE_DIRECTFB src/mptet.c `$(call pkg_config,directfb)` -o mptet $(LIBS)

sdl2: src/mptet.c src/gfxsdl2.c
	$(CC) $(CFLAGS) -DUSE_SDL2 src/mptet.c `$(call pkg_config,sdl2)` -o mptet $(LIBS)

.PHONY: clean test

test: src/mptet.c test/main.c
	$(CC) $(CFLAGS) -g -DTEST src/mptet.c -o testmp $(LIBS)

clean:
	rm -f mptet testmp
