CC ?= clang
CFLAGS += -lgmp -O2 -o tet

all: directfb

testing: src/testing.c
	$(CC) $(CFLAGS) src/testing.c -DMPTET_RENDERER=MPTET_USE_DIRECTFB -I/usr/include/directfb -ldirectfb

terminal: src/tet.c
	$(CC) $(CFLAGS) src/tet.c

fb: src/tet.c
	$(CC) $(CFLAGS) src/tet.c -DMPTET_RENDERER=MPTET_USE_FRAMEBUFFER

directfb: src/tet.c
	$(CC) $(CFLAGS) src/tet.c -DMPTET_RENDERER=MPTET_USE_DIRECTFB -I/usr/include/directfb -ldirectfb
