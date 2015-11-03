CC ?= clang
CFLAGS += -O2 -o mptet
LDFLAGS += -lgmp
MAIN := src/mptet.c

all: directfb

testing: src/testing
	$(CC) $(CFLAGS) src/testing.c -DMPTET_RENDERER=MPTET_USE_DIRECTFB -I/usr/include/directfb -ldirectfb $(LDFLAGS)

terminal: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN) $(LDFLAGS)

fb: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN) -DMPTET_RENDERER=MPTET_USE_FRAMEBUFFER $(LDFLAGS)

directfb: $(MAIN)
	$(CC) $(CFLAGS) $(MAIN) -DMPTET_RENDERER=MPTET_USE_DIRECTFB -I/usr/include/directfb -ldirectfb $(LDFLAGS)

clean:
	rm mptet

.PHONY: clean
