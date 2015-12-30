# mptet

[![Build Status](https://travis-ci.org/Tiehuis/mptet.svg?branch=master)](https://travis-ci.org/Tiehuis/mptet)

mptet (Multiple-Precision Tetris) is a tetris clone originally implemented
using the GMP bignum library. This dependency has since been removed however
the name remains.

The current main focus of this is to provide a fast, memory-efficient,
cross-platform and relatively feature-complete tetris game, which
requires minimal external dependencies if required.

### Installation

First clone the repository.

```
git clone https://github.com/tiehuis/mptet
cd mptet
```

Next compile the program with the chosen graphical frontend. A number of
seperate graphical frontends can be utilized at the moment. The
following steps are required for building each 64-bit Ubuntu 15.10.

#### SDL2

This frontend utilizes the SDL2 library, which is a commonly found cross
platform library.

```
apt-get install libsdl2-dev
make sdl2
```

#### Directfb

Directfb is a graphical library which allows running of programs without a
full X-Server required to be run also.

```
apt-get install libdirectfb-dev
make directfb
```

If you are using an X-Server, you can specify an alternative system to run
directfb with. A common example utilizes SDL and can be run with:

```
./mptet --dfb:system=SDL
```

#### X11

If running on a system which runs an X-Server, mptet can be compiled to use
X11 calls directly. This will likely be the quickest graphical version in
terms of speed and memory, and the required dependencies will already exist
on your system.

```
make x11
```

##### Executing

Once compiled, the program can be run with the following command.

```
./mptet
```

#### Focus

The focus of this is to provide a small tetris clone which provides a large
enough feature-set for serious play. In particular, the prioritized modes are
the line race modes.

The following are important features that should be apparent:

- Accurate Movement/Rotations
- Accurate Timing and Advanced Features, such as DAS/ARE
- Small Memory Footprint
- Small Code Base / Keep It Simple
- Cross-Platform (Minor)

#### Potential Things To Do

##### Platform Specific Code

Some parts of the code need to be rewritten for each operating system. In
particular, the timing mechanisms differ between each.

##### More Graphical Frontends

Writing different frontends is easy at this stage, and should remain easy
if the complexity is kept simple. Various frontends have different benefits
and having multiple allow a greater degree of user customisation.

##### Unit Tests

Writing a number of tests to ensure that various engine aspects behave as
expected would be invaluable. In Particular, if adding more rotation
systems these could be automatically verified for quirky cases that should
work.

##### Multiple Resolutions

It would be beneficial to support a number of different resolutions. Currently
we draw to a 1080p fullscreen with fixed block positions. It still has not been
decided exactly how this should be done, but it likely will follow a relative
drawing scheme where the region is scaled to fit the given screen size.

##### Storing Game Data

Times should be saved to file. In addition, other features such as fumen
recording could also potentially be added.
