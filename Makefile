CC ?= clang
CFLAGS += -lgmp -O2 -o tet

all: src/tet.c
	$(CC) $(CFLAGS) src/tet.c

fb: src/tet.c
	$(CC) $(CFLAGS) src/tet.c -DTET_GUI=tetFRAMEBUFFER

directfb: src/tet.c
	$(CC) $(CFLAGS) src/tet.c -DTET_GUI=tetDIRECTFB -I/usr/include/directfb -ldirectfb
