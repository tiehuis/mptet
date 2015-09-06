# mptet

Tetris implemented using a multiple-precision library.

There is a requirement on GMP, which is likely installed on your system.

```
apt-get install libgmp-dev
```

The main graphics library used is directfb, which provides support for
hardware accelerated rendering onto the framebuffer.

```
apt-get install libdirectfb-dev
```

Then, to compile and run.

```
make
./mptet
```

This will attempt to open /dev/fb0 and memory map it so we can draw directly
to the framebuffer. If you have insufficient permissions it may fail, in which
case you can try running as root.

Running as root may also fail anyway, in which case we can fallback to SDL
in order to run this program. Ensure you have the SDL libraries installed.

```
apt-get install libsdl1.2debian
```

Then, alter the runtime system by running the program with the following
arguments.

```
./mptet --dfb:system=SDL
```

### Features

- DAS-like movement.
- Small memory footprint
- Instantly loads
- Playable in non-X environments
- Small code-base, easy to modify (kind of!)

### TODO

- [ ] Correct rotation and line-clear algorithms
- [ ] Add wallkicks to the rotation system
- [x] Add hold function
- [ ] Add a line race mode
- [ ] Add a scoring system to line clears
- [ ] Verify main loop CPU usage and look for any savings
- [ ] Change GMP allocator to use a custom stack-based allocator
- [ ] Automatic replays
- [ ] Multiple screen resolutions (Not only fullscreen)
