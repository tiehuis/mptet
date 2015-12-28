#include <directfb.h>

#define DC_(...)                                                 \
    do {                                                         \
        DFBResult err = __VA_ARGS__;                             \
        if (err != DFB_OK) {                                     \
            fprintf(stderr, "%s <%d>:\n\t", __FILE__, __LINE__); \
            DirectFBErrorFatal(#__VA_ARGS__, err);               \
        }                                                        \
    } while (0)

typedef struct {
    int width;
    int height;
    IDirectFB *dfb;
    IDirectFBSurface *primary;
    IDirectFBInputDevice *keyboard;
} mpgfx;

static int keystates__[] = {
    DIKI_LEFT, DIKI_RIGHT, DIKI_DOWN, DIKI_Z, DIKI_X, DIKI_C,
    DIKI_SPACE, DIKI_Q
};

void mpgfx_init(mpgfx *mx, int *argc, char ***argv)
{
    DFBSurfaceDescription dsc;
    DC_(DirectFBInit(argc, argv));
    DC_(DirectFBCreate(&mx->dfb));
    DC_(mx->dfb->SetCooperativeLevel(mx->dfb, DFSCL_EXCLUSIVE));

    dsc.flags = DSDESC_CAPS;
    dsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;

    /* Construct surface */
    DC_(mx->dfb->CreateSurface(mx->dfb, &dsc, &mx->primary));
    DC_(mx->primary->GetSize(mx->primary, &mx->width, &mx->height));
    DC_(mx->primary->FillRectangle(mx->primary, 0, 0, mx->width, mx->height));
    DC_(mx->primary->Flip(mx->primary, NULL, DSFLIP_NONE));

    /* Setup keyboard */
    DC_(mx->dfb->GetInputDevice(mx->dfb, DIDID_KEYBOARD, &mx->keyboard));
}

#include <unistd.h>

void mpgfx_update(mpstate *ms, mpgfx *mx)
{
    DFBInputDeviceKeyState state;

    for (size_t i = 0; i < sizeof(keystates__) / sizeof(keystates__[0]); ++i) {
        DC_(mx->keyboard->GetKeyState(mx->keyboard, keystates__[i], &state));

        if (state == DIKS_DOWN)
            ms->keystate[i]++;
        else
            ms->keystate[i] = 0;
    }
}

void mpgfx_free(mpgfx *mx)
{
    mx->keyboard->Release(mx->keyboard);
    mx->primary->Release(mx->primary);
    mx->dfb->Release(mx->dfb);
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
#define H_X_OFFSET (M_X_OFFSET / 2 -  (M_BLOCK_SIDE * H_BLOCK_SCALE) * 2)

/* Hold preview y offset */
#define H_Y_OFFSET 40

void mpgfx_render(mpstate *ms, mpgfx *mx)
{
    // Clear Screen
    DC_(mx->primary->SetColor(mx->primary, 0, 0, 0, 0xff));
    DC_(mx->primary->FillRectangle(mx->primary, 0, 0, mx->width, mx->height));

    // Draw bounding field
    DC_(mx->primary->SetColor(mx->primary, 0x80, 0x80, 0x80, 0xff));
    DC_(mx->primary->DrawRectangle(mx->primary,
                M_X_OFFSET - 1,
                M_Y_OFFSET - 1,
                10 * M_BLOCK_SIDE + 2,
                22 * M_BLOCK_SIDE + 2));

    DC_(mx->primary->DrawRectangle(mx->primary,
                M_X_OFFSET - 2,
                M_Y_OFFSET - 2,
                10 * M_BLOCK_SIDE + 4,
                22 * M_BLOCK_SIDE + 4));

    // Draw blocks
    for (int i = 219; i >= 0; --i) {
        if (mem256_get(&ms->field, i) || mem256_get(&ms->block, i))
            DC_(mx->primary->SetColor(mx->primary, 0x80, 0x80, 0xff, 0xff));
        else if (mem256_get(&ms->ghost, i))
            DC_(mx->primary->SetColor(mx->primary, 0x80, 0x80, 0xff / 2, 0));
        else
            DC_(mx->primary->SetColor(mx->primary, 0, 0, 0, 0xff));

        DC_(mx->primary->FillRectangle(mx->primary,
                    M_X_OFFSET + M_BLOCK_SIDE * (9 - i % 10) + 1,
                    M_Y_OFFSET + M_BLOCK_SIDE * (21 - (i / 10)) + 1,
                    M_BLOCK_SIDE - 2,
                    M_BLOCK_SIDE - 2));
    }

    DC_(mx->primary->SetColor(mx->primary, 0x80, 0x80, 0xff, 0xff));
    uint64_t block = mptetd_block[ms->hold][0];

    if (ms->hold != -1) {
        for (int x = 0; x < 4; ++x) {
            for (int y = 0; y < 4; ++y) {
                if (block & ((1 << ((4 - y) * 10 - x - 1))))
                    DC_(mx->primary->FillRectangle(mx->primary,
                                H_X_OFFSET + x * M_BLOCK_SIDE * H_BLOCK_SCALE,
                                M_Y_OFFSET + H_Y_OFFSET + y * M_BLOCK_SIDE * H_BLOCK_SCALE,
                                H_BLOCK_SCALE * M_BLOCK_SIDE - 2,
                                H_BLOCK_SCALE * M_BLOCK_SIDE - 2));
            }
        }
    }

    // Draw preview pieces
    for (int i = 0; i < PREVIEW_NUMBER; ++i) {
        uint64_t block = mptetd_block[ms->bag[ms->bhead + i] % 14][0];

        for (int x = 0; x < 4; ++x) {
            for (int y = 0; y < 4; ++y) {
                if (block & ((1 << ((4 - y) * 10 - x - 1))))
                    DC_(mx->primary->FillRectangle(mx->primary,
                                M_X_OFFSET + 10 * M_BLOCK_SIDE + P_X_OFFSET + x * M_BLOCK_SIDE * P_BLOCK_SCALE,
                                M_Y_OFFSET + i * (P_Y_OFFSET + 4 * M_BLOCK_SIDE * P_BLOCK_SCALE)
                                + y * M_BLOCK_SIDE * P_BLOCK_SCALE,
                                P_BLOCK_SCALE * M_BLOCK_SIDE - 2,
                                P_BLOCK_SCALE * M_BLOCK_SIDE - 2));
            }
        }
    }

    DC_(mx->primary->Flip(mx->primary, NULL, DSFLIP_NONE));
}
