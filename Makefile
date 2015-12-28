CC 	   ?= gcc
CFLAGS += -O2 -Wall -Wextra -Wno-format -Wunreachable-code

all: sdl

directfb: src/mptet.c src/gfxdirectfb.c
	gcc $(CFLAGS) src/mptet.c `pkg-config --cflags --libs directfb` -o mptet

sdl: src/mptet.c src/gfxsdl2.c
	gcc $(CFLAGS) src/mptet.c `pkg-config --cflags --libs sdl2` -o mptet
