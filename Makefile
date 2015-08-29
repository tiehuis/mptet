CC ?= clang

all: src/tet.c
	$(CC) src/tet.c -lgmp -o tet

fb:
	$(CC) tetribit.c -I/usr/include/directfb -DFRAMEBUFFER -lgmp -ldirectfb
