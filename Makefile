CC 	   ?= clang
CFLAGS += -O2 -std=gnu1x -Wall -Wextra -Wno-format -Wunreachable-code \
		  -Wwrite-strings -Wcast-align #-Wswitch-default -Wshadow -Wconversion
LIBS   += -lrt

all: sdl2

pkg_config = pkg-config --cflags --libs $(1)

x11: src/mptet.c src/mem256.c src/x11.h
	$(CC) $(CFLAGS) -DMP_GFX_X11 src/mptet.c src/mem256.c `$(call pkg_config,x11)` -o mptet $(LIBS)

directfb: src/mptet.c src/directfb.h
	$(CC) $(CFLAGS) -DMP_GFX_DIRECTFB src/mptet.c src/mem256.c `$(call pkg_config,directfb)` -o mptet $(LIBS)

sdl2: src/mptet.c src/sdl2.h
	$(CC) $(CFLAGS) -DMP_GFX_SDL2 src/mptet.c src/mem256.c `$(call pkg_config,sdl2)` -o mptet $(LIBS)

.PHONY: clean test

test: src/mptet.c src/test.c
	$(CC) $(CFLAGS) -g -fstack-check -fno-omit-frame-pointer -fsanitize=undefined \
		src/mptet.c src/mem256.c src/test.c -o test $(LIBS)

clean:
	rm -f mptet test
