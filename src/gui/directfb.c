#include <stdio.h>
#include <unistd.h>
#include <directfb.h>

static IDirectFB *dfb = NULL;
static IDirectFBSurface *primary = NULL;
static IDirectFBInputDevice *keyboard = NULL;
static int screen_width = 0;
static int screen_height = 0;

#define DFBCHECK(...)                                            \
    do {                                                         \
        DFBResult err = __VA_ARGS__;                             \
        if (err != DFB_OK) {                                     \
            fprintf(stderr, "%s <%d>:\n\t", __FILE__, __LINE__); \
            DirectFBErrorFatal(#__VA_ARGS__, err);               \
        }                                                        \
    } while (0)

void gui_init(int argc, char **argv)
{
    DFBSurfaceDescription dsc;
    DFBCHECK(DirectFBInit(&argc, &argv));
    DFBCHECK(DirectFBCreate(&dfb));
    DFBCHECK(dfb->SetCooperativeLevel(dfb, DFSCL_FULLSCREEN));

    dsc.flags = DSDESC_CAPS;
    dsc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;

    DFBCHECK(dfb->CreateSurface(dfb, &dsc, &primary));
    DFBCHECK(primary->GetSize(primary, &screen_width, &screen_height));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width, screen_height));
    DFBCHECK(primary->Flip(primary, NULL, 0));

    /* Setup keyboard */
    DFBCHECK(dfb->GetInputDevice(dfb, DIDID_KEYBOARD, &keyboard));
}

static int keystates__[] =
{
    DIKI_LEFT, DIKI_RIGHT, DIKI_DOWN, DIKI_Z, DIKI_X, DIKI_SPACE, DIKI_Q
};

bool gui_update(void)
{
    DFBInputDeviceKeyState state;

    for (int i = 0; i < sizeof(keystates__) / sizeof(keystates__[0]); ++i) {
        DFBCHECK(keyboard->GetKeyState(keyboard, keystates__[i], &state));

        if (state == DIKS_DOWN)
            keyboardState[i]++;
        else
            keyboardState[i] = 0;
    }

    return true;
}

void gui_render(void)
{
    // Clear Screen
    DFBCHECK(primary->SetColor(primary, 0, 0, 0, 0xff));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width, screen_height));

    #define BLOCK_SIDE 36

    // Draw bounding field
    DFBCHECK(primary->SetColor(primary, 0x80, 0x80, 0x80, 0xff));
    DFBCHECK(primary->DrawRectangle(primary, 99, 99, 10 * BLOCK_SIDE + 2, 22 * BLOCK_SIDE + 2));
    DFBCHECK(primary->DrawRectangle(primary, 98, 98, 10 * BLOCK_SIDE + 4, 22 * BLOCK_SIDE + 4));

    // Draw Bricks
    for (int i = 219; i >= 0; --i) {
        if (mpz_tstbit(field, i) | mpz_tstbit(block, i)) {
            DFBCHECK(primary->SetColor(primary, 0x80, 0x80, 0xff, 0xff));
        }
        else if (mpz_tstbit(ghost, i)) {
            DFBCHECK(primary->SetColor(primary, 0x80, 0x80, 0xff, 0));
        }
        else {
            DFBCHECK(primary->SetColor(primary, 0, 0, 0, 0xff));
        }

        int xx = 100 + BLOCK_SIDE * (9 - (i % 10));
        int yy = 100 + BLOCK_SIDE * (21 - (i / 10));

        DFBCHECK(primary->FillRectangle(primary, xx + 1, yy + 1, BLOCK_SIDE - 2, BLOCK_SIDE - 2));
    }

    // Draw preview pieces
    for (int i = 0; i < 3; ++i) {
        int64_t block = (bdata[type][rot]) & 0xffffffffff;
    }

    DFBCHECK(primary->Flip(primary, NULL, 0));
}

void gui_free(void)
{
    keyboard->Release(keyboard);
    primary->Release(primary);
    dfb->Release(dfb);
}
