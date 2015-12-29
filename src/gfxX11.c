#include <X11/Xlib.h>
#include <X11/keysym.h>

typedef struct {
    Display *display;
    Window window;
    GC gc;
    int id;
} mpgfx;

void mpgfx_init(mpgfx *mx, int *argc, char ***argv)
{
    (void) argc;
    (void) argv;

    mx->display = XOpenDisplay(NULL);

    if (!mx->display) {
        fprintf(stderr, "Cannot open display\n");
        exit(-1);
    }

    mx->id = DefaultScreen(mx->display);
    mx->gc = DefaultGC(mx->display, mx->id);
    mx->window = XCreateSimpleWindow(mx->display,
            RootWindow(mx->display, mx->id), 0, 0, 1920, 1080, 0,
            BlackPixel(mx->display, mx->id), WhitePixel(mx->display, mx->id));

    XSelectInput(mx->display, mx->window, KeyPressMask | KeyReleaseMask);
    XMapWindow(mx->display, mx->window);
}

static int xk_index(XEvent *e)
{
    switch (XLookupKeysym(&e->xkey, 0)) {
    case XK_Left:
        return K_Left;
    case XK_Right:
        return K_Right;
    case XK_Down:
        return K_Down;
    case XK_Z: case XK_z:
        return K_z;
    case XK_X: case XK_x:
        return K_x;
    case XK_C: case XK_c:
        return K_c;
    case XK_space:
        return K_Space;
    case XK_Q: case XK_q:
        return K_q;
    default:
        return -1;
    }
}

void mpgfx_update(mpstate *ms, mpgfx *mx)
{
    XEvent e;

    /* Did we encounter an event for this key */
    int seen[10] = { 0 };

    /* Consume all events in queue until done */
    while (XCheckMaskEvent(mx->display, KeyPressMask | KeyReleaseMask, &e)) {
        /* If a key is released then set to 0.
         * If a key value is 1 and it was not pressed, then set to zero
         * If a key is pressed and set to 1, then allow */

        int index = xk_index(&e);

        /* Only deal with key events we understand */
        if (index == -1)
            continue;

        switch (e.type) {
        case KeyPress:
            ms->keystate[index] = 1;
            seen[index] = 1;
            break;
        case KeyRelease:
            ms->keystate[index] = 0;
            seen[index] = 1;
            break;
        default:
            break;
        }
    }

    for (int i = 0; i < 10; ++i) {
        /* Only increment those which we did not see an event */
        if (ms->keystate[i] && !seen[i])
            ms->keystate[i]++;
    }
}

void mpgfx_free(mpgfx *mx)
{
    XDestroyWindow(mx->display, mx->window);
    XCloseDisplay(mx->display);
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
    /* Clear screen */
    XSetForeground(mx->display, mx->gc, BlackPixel(mx->display, 0));
    XFillRectangle(mx->display, mx->window, mx->gc, 0, 0, 1920, 1080);

    XSetForeground(mx->display, mx->gc, WhitePixel(mx->display, 0));

    XDrawRectangle(mx->display, mx->window, mx->gc,
                    M_X_OFFSET - 1,
                    M_Y_OFFSET - 1,
                    10 * M_BLOCK_SIDE + 2,
                    22 * M_BLOCK_SIDE + 2);

    XDrawRectangle(mx->display, mx->window, mx->gc,
                    M_X_OFFSET - 2,
                    M_Y_OFFSET - 2,
                    10 * M_BLOCK_SIDE + 4,
                    22 * M_BLOCK_SIDE + 4);

    // Draw blocks
    for (int i = 219; i >= 0; --i) {
        bool fill = false;

        if (mem256_get(&ms->field, i) || mem256_get(&ms->block, i)) {
            XSetForeground(mx->display, mx->gc, WhitePixel(mx->display, 0));
            fill = true;
        }
        else if (mem256_get(&ms->ghost, i))
            XSetForeground(mx->display, mx->gc, WhitePixel(mx->display, 0));
        else
            XSetForeground(mx->display, mx->gc, BlackPixel(mx->display, 0));

        if (fill) {
            XFillRectangle(mx->display, mx->window, mx->gc,
                        M_X_OFFSET + M_BLOCK_SIDE * (9 - i % 10) + 1,
                        M_Y_OFFSET + M_BLOCK_SIDE * (21 - (i / 10)) + 1,
                        M_BLOCK_SIDE - 2,
                        M_BLOCK_SIDE - 2);
        }
        else {
            XDrawRectangle(mx->display, mx->window, mx->gc,
                        M_X_OFFSET + M_BLOCK_SIDE * (9 - i % 10) + 1,
                        M_Y_OFFSET + M_BLOCK_SIDE * (21 - (i / 10)) + 1,
                        M_BLOCK_SIDE - 2,
                        M_BLOCK_SIDE - 2);
        }
    }

    XSetForeground(mx->display, mx->gc, WhitePixel(mx->display, 0));
    uint64_t block = mptetd_block[ms->hold][0];

    if (ms->hold != -1) {
        for (int x = 0; x < 4; ++x) {
            for (int y = 0; y < 4; ++y) {
                if (block & ((1 << ((4 - y) * 10 - x - 1))))
                    XFillRectangle(mx->display, mx->window, mx->gc,
                                H_X_OFFSET + x * M_BLOCK_SIDE * H_BLOCK_SCALE,
                                M_Y_OFFSET + H_Y_OFFSET + y * M_BLOCK_SIDE * H_BLOCK_SCALE,
                                H_BLOCK_SCALE * M_BLOCK_SIDE - 2,
                                H_BLOCK_SCALE * M_BLOCK_SIDE - 2);
            }
        }
    }

    // Draw preview pieces
    for (int i = 0; i < PREVIEW_NUMBER; ++i) {
        uint64_t block = mptetd_block[ms->bag[ms->bhead + i] % 14][0];

        for (int x = 0; x < 4; ++x) {
            for (int y = 0; y < 4; ++y) {
                if (block & ((1 << ((4 - y) * 10 - x - 1))))
                    XFillRectangle(mx->display, mx->window, mx->gc,
                                M_X_OFFSET + 10 * M_BLOCK_SIDE + P_X_OFFSET + x * M_BLOCK_SIDE * P_BLOCK_SCALE,
                                M_Y_OFFSET + i * (P_Y_OFFSET + 4 * M_BLOCK_SIDE * P_BLOCK_SCALE)
                                + y * M_BLOCK_SIDE * P_BLOCK_SCALE,
                                P_BLOCK_SCALE * M_BLOCK_SIDE - 2,
                                P_BLOCK_SCALE * M_BLOCK_SIDE - 2);
            }
        }
    }
}
