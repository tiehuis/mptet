#include <stdio.h>
#include <unistd.h>
#include <directfb.h>

static IDirectFB *dfb = NULL;
static IDirectFBSurface *primary = NULL;
static IDirectFBInputDevice *keyboard = NULL;
static IDirectFBFont *font = NULL;
static int sWidth = 0;
static int sHeight = 0;

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
    /* Initialization */
    DFBSurfaceDescription dsc;
    DFBCHECK(DirectFBInit(&argc, &argv));
    DFBCHECK(DirectFBCreate(&dfb));
    DFBCHECK(dfb->SetCooperativeLevel(dfb, DFSCL_FULLSCREEN));

    dsc.flags = DSDESC_CAPS;
    dsc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;


    //DFBCHECK(primary->SetDrawingFlags(primary, DSDRAW_BLEND));

    /* Initialization fonts */
#ifdef MPTET_USE_FONTS
    DFBFontDescription font_dsc;
    font_dsc.flags = DFDESC_HEIGHT;
    font_dsc.height = 48;
    DFBCHECK(dfb->CreateFont(dfb, "./ubuntu-mono.ttf", &font_dsc, &font));
    DFBCHECK(primary->SetFont(primary, font));
#endif

    /* Construct surface */
    DFBCHECK(dfb->CreateSurface(dfb, &dsc, &primary));
    DFBCHECK(primary->GetSize(primary, &sWidth, &sHeight));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, sWidth, sHeight));
    DFBCHECK(primary->Flip(primary, NULL, DSFLIP_NONE));

    /* Setup keyboard */
    DFBCHECK(dfb->GetInputDevice(dfb, DIDID_KEYBOARD, &keyboard));
}

static int keystates__[] =
{
    DIKI_LEFT, DIKI_RIGHT, DIKI_DOWN, DIKI_Z, DIKI_X, DIKI_SPACE, DIKI_C, DIKI_Q
};

void gui_update(void)
{
    DFBInputDeviceKeyState state;

    for (int i = 0; i < sizeof(keystates__) / sizeof(keystates__[0]); ++i) {
        DFBCHECK(keyboard->GetKeyState(keyboard, keystates__[i], &state));

        if (state == DIKS_DOWN)
            keyboardState[i]++;
        else
            keyboardState[i] = 0;
    }
}

/* Number of preview pieces shown */
#define PREVIEW_NUMBER 3

/* Block side length */
#define M_BLOCK_SIDE 36

/* Main grid x offset */
#define M_X_OFFSET 200

/* Main grid y offset */
#define M_Y_OFFSET 100

/* Preview x offset */
#define P_X_OFFSET 40

/* Preview y offset */
#define P_Y_OFFSET 20

/* Preview block scale */
#define P_BLOCK_SCALE 0.9f

/* Hold preview block scale */
#define H_BLOCK_SCALE 0.9f

/* Hold preview x offset - assert(M_X_OFFSET > (M_BLOCK_SIDE * H_BLOCK_SCALE * 4))*/
#define H_X_OFFSET (M_X_OFFSET / 2 - MPTET_MAX(0, (M_BLOCK_SIDE * H_BLOCK_SCALE) * 2))

/* Hold preview y offset */
#define H_Y_OFFSET 40

void gui_render(void)
{
    // Clear Screen
    DFBCHECK(primary->SetColor(primary, 0, 0, 0, 0xff));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, sWidth, sHeight));

    // Draw bounding field
    DFBCHECK(primary->SetColor(primary, 0x80, 0x80, 0x80, 0xff));
    DFBCHECK(primary->DrawRectangle(primary,
                M_X_OFFSET - 1,
                M_Y_OFFSET - 1,
                10 * M_BLOCK_SIDE + 2,
                22 * M_BLOCK_SIDE + 2));

    DFBCHECK(primary->DrawRectangle(primary,
                M_X_OFFSET - 2,
                M_Y_OFFSET - 2,
                10 * M_BLOCK_SIDE + 4,
                22 * M_BLOCK_SIDE + 4));

    // Draw blocks
    for (int i = 219; i >= 0; --i) {
        if (mpz_tstbit(field, i) | mpz_tstbit(block, i))
            DFBCHECK(primary->SetColor(primary, 0x80, 0x80, 0xff, 0xff));
        else if (mpz_tstbit(ghost, i))
            DFBCHECK(primary->SetColor(primary, 0x80, 0x80, 0xff / 2, 0));
        else
            DFBCHECK(primary->SetColor(primary, 0, 0, 0, 0xff));

        DFBCHECK(primary->FillRectangle(primary,
                    M_X_OFFSET + M_BLOCK_SIDE * (9 - i % 10) + 1,
                    M_Y_OFFSET + M_BLOCK_SIDE * (21 - (i / 10)) + 1,
                    M_BLOCK_SIDE - 2,
                    M_BLOCK_SIDE - 2));
    }

    // Draw hold piece
    DFBCHECK(primary->SetColor(primary, 0x80, 0x80, 0xff, 0xff));
    uint64_t block = PIECE(hold, 0);

    if (hold != -1) {
        for (int x = 0; x < 4; ++x) {
            for (int y = 0; y < 4; ++y) {
                if (block & ((1 << ((4 - y) * 10 - x - 1))))
                    DFBCHECK(primary->FillRectangle(primary,
                                H_X_OFFSET + x * M_BLOCK_SIDE * H_BLOCK_SCALE,
                                M_Y_OFFSET + H_Y_OFFSET + y * M_BLOCK_SIDE * H_BLOCK_SCALE,
                                H_BLOCK_SCALE * M_BLOCK_SIDE - 2,
                                H_BLOCK_SCALE * M_BLOCK_SIDE - 2));
            }
        }
    }

    // Draw preview pieces
    for (int i = 0; i < PREVIEW_NUMBER; ++i) {
        uint64_t block = PIECE(bag[mod(bag_head + i, 14)], 0);

        for (int x = 0; x < 4; ++x) {
            for (int y = 0; y < 4; ++y) {
                if (block & ((1 << ((4 - y) * 10 - x - 1))))
                    DFBCHECK(primary->FillRectangle(primary,
                                M_X_OFFSET + 10 * M_BLOCK_SIDE + P_X_OFFSET + x * M_BLOCK_SIDE * P_BLOCK_SCALE,
                                M_Y_OFFSET + i * (P_Y_OFFSET + 4 * M_BLOCK_SIDE * P_BLOCK_SCALE)
                                + y * M_BLOCK_SIDE * P_BLOCK_SCALE,
                                P_BLOCK_SCALE * M_BLOCK_SIDE - 2,
                                P_BLOCK_SCALE * M_BLOCK_SIDE - 2));
            }
        }
    }

    // DFBCHECK(primary->DrawString(primary, "Tetris", -1, 10, 10, DSTF_LEFT));
    DFBCHECK(primary->Flip(primary, NULL, DSFLIP_NONE));
}

void gui_free(void)
{
#ifdef MPTET_USE_FONTS
    font->Release(font);
#endif
    keyboard->Release(keyboard);
    primary->Release(primary);
    dfb->Release(dfb);
}
