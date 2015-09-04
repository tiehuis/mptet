#include <stdio.h>
#include <unistd.h>
#include <directfb.h>

static IDirectFB *dfb = NULL;
static IDirectFBSurface *primary = NULL;
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
}

bool update(void)
{
    int ch = getchar();
        switch (ch) {
            case 'h':
                move_horizontal(1);
                break;
            case 'l':
                xxx += 50;
                move_horizontal(-1);
                break;
            case 'j':
                xxx -= 50;
                move_down();
                break;
            case 'z':
                move_rotate(-1);
                break;
            case 'x':
                move_rotate(1);
                break;
            case 'b':
                move_harddrop();
                clear_lines();
                break;
            case 'q':
                return false;
        }

    return true;
}

void render(void)
{
    DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width, screen_height));
    DFBCHECK(primary->SetColor(primary, 0x80, 0x80, 0xff, 0xff));
    DFBCHECK(primary->Flip(primary, NULL, 0));
}

void gui_free(void)
{
    primary->Release(primary);
    dfb->Release(dfb);
}
