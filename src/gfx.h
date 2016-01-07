#pragma once

#include "mptet.h"

#if defined(MP_GFX_SDL2)
#   include "sdl2.h"
#elif defined(MP_GFX_DIRECTFB)
#   include "directfb.h"
#elif defined(MP_GFX_X11)
#   include "x11.h"
#else
#   define MP_NO_GFX
#endif

