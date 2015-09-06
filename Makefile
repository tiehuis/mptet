CC ?= clang
CFLAGS += -lgmp -O2 -o mptet
MAIN := src/mptet.c

all: directfb

testing: src/testing
	$(CC) $(CFLAGS) src/testing.c -DMPTET_RENDERER=MPTET_USE_DIRECTFB -I/usr/include/directfb -ldirectfb

terminal: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN)

fb: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN) -DMPTET_RENDERER=MPTET_USE_FRAMEBUFFER

directfb: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN) -DMPTET_RENDERER=MPTET_USE_DIRECTFB -I/usr/include/directfb -ldirectfb
