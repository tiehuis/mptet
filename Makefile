CC 	   ?= gcc
CFLAGS += -O2 -Wall -Wextra -Wno-format -Wunreachable-code

all: directfb

directfb: src/mptet.c src/gfxdirectfb.c
	gcc $(CFLAGS) src/mptet.c `pkg-config --cflags --libs directfb` -o mptet
