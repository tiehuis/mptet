CC 	   ?= clang
CFLAGS += -O2 -Wall -Wextra -Wno-format -Wunreachable-code

all: sdl2

x11: src/mptet.c src/gfxX11.c
	$(CC) $(CFLAGS) -DUSE_X11 src/mptet.c `pkg-config --cflags --libs x11` -o mptet

directfb: src/mptet.c src/gfxdirectfb.c
	$(CC) $(CFLAGS) -DUSE_DIRECTFB src/mptet.c `pkg-config --cflags --libs directfb` -o mptet

sdl2: src/mptet.c src/gfxsdl2.c
	$(CC) $(CFLAGS) -DUSE_SDL2 src/mptet.c `pkg-config --cflags --libs sdl2` -o mptet
