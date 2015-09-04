CC ?= clang

all: src/tet.c
	$(CC) src/tet.c -lgmp -o tet -O2

fb: src/tet.c
	$(CC) src/tet.c -DTET_GUI=tetFRAMEBUFFER -lgmp -o tet -O2

directfb: src/tet.c
	$(CC) src/tet.c -DTET_GUI=tetDIRECTFB -lgmp -o tet -O2 -I/usr/include/directfb -ldirectfb
