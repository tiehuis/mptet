CC ?= clang

all:
	$(CC) tetribit.c -lgmp
