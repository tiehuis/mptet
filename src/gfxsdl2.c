#include <SDL.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} mpgfx;

void mpgfx_init(mpgfx *mx, int *argc, char ***argv)
{
    (void) argc;
    (void) argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        exit(-1);
    }

    mx->window = SDL_CreateWindow("mptet", 0, 0, 1920, 1080, SDL_WINDOW_FULLSCREEN);

    if (!mx->window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        exit(-1);
    }

    mx->renderer = SDL_CreateRenderer(mx->window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!mx->renderer) {
        SDL_DestroyWindow(mx->window);
        SDL_Quit();
        exit(-1);
    }
}

static int keycodes[] = {
    SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_DOWN, SDL_SCANCODE_Z,
    SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_SPACE, SDL_SCANCODE_Q
};

void mpgfx_update(mpstate *ms, mpgfx *mx)
{
    (void) mx;

    SDL_PumpEvents();
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    for (size_t i = 0; i < sizeof(keycodes) / sizeof(keycodes[0]); ++i) {
        if (state[keycodes[i]])
            ms->keystate[i]++;
        else
            ms->keystate[i] = 0;
    }
}

void mpgfx_free(mpgfx *mx)
{
    SDL_DestroyRenderer(mx->renderer);
    SDL_DestroyWindow(mx->window);
    SDL_Quit();
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

#define DrawRect(rnd, r, _fun, _x, _y, _w, _h)          \
    do {                                                \
        r.x = (_x); r.y = (_y); r.w = (_w); r.h = (_h); \
        SDL_Render##_fun##Rect(rnd, &r);               \
    } while (0)

void mpgfx_render(mpstate *ms, mpgfx *mx)
{
    SDL_Rect r;

    // Clear Screen
    SDL_SetRenderDrawColor(mx->renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(mx->renderer);

    // Draw bounding field
    SDL_SetRenderDrawColor(mx->renderer, 0x80, 0x80, 0x80, 0xff);

    DrawRect(mx->renderer, r, Draw,
            M_X_OFFSET - 1, M_Y_OFFSET - 1,
            10 * M_BLOCK_SIDE + 2, 22 * M_BLOCK_SIDE + 2);

    DrawRect(mx->renderer, r, Draw,
            M_X_OFFSET - 2, M_Y_OFFSET - 2,
            10 * M_BLOCK_SIDE + 4, 22 * M_BLOCK_SIDE + 4);

    // Draw blocks
    for (int i = 219; i >= 0; --i) {
        bool fill = false;
        if (mem256_get(&ms->field, i) || mem256_get(&ms->block, i)) {
            SDL_SetRenderDrawColor(mx->renderer, 0x80, 0x80, 0xff, 0xff);
            fill = true;
        }
        else if (mem256_get(&ms->ghost, i)) {
            SDL_SetRenderDrawColor(mx->renderer, 0x80, 0x80, 0xff / 2, 0);
        }
        else {
            SDL_SetRenderDrawColor(mx->renderer, 0, 0, 0, 0xff);
        }

        if (fill) {
            DrawRect(mx->renderer, r, Fill,
                     M_X_OFFSET + M_BLOCK_SIDE * (9 - i % 10) + 1,
                     M_Y_OFFSET + M_BLOCK_SIDE * (21 - (i / 10)) + 1,
                     M_BLOCK_SIDE - 2,
                     M_BLOCK_SIDE - 2);
        }
        else {
            DrawRect(mx->renderer, r, Draw,
                     M_X_OFFSET + M_BLOCK_SIDE * (9 - i % 10) + 1,
                     M_Y_OFFSET + M_BLOCK_SIDE * (21 - (i / 10)) + 1,
                     M_BLOCK_SIDE - 2,
                     M_BLOCK_SIDE - 2);
        }
    }

    SDL_SetRenderDrawColor(mx->renderer, 0x80, 0x80, 0xff, 0xff);
    uint64_t block = mptetd_block[ms->hold][0];

    if (ms->hold != -1) {
        for (int x = 0; x < 4; ++x) {
            for (int y = 0; y < 4; ++y) {
                if (block & ((1 << ((4 - y) * 10 - x - 1))))
                    DrawRect(mx->renderer, r, Fill,
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
                    DrawRect(mx->renderer, r, Fill,
                             M_X_OFFSET + 10 * M_BLOCK_SIDE + P_X_OFFSET + x * M_BLOCK_SIDE * P_BLOCK_SCALE,
                             M_Y_OFFSET + i * (P_Y_OFFSET + 4 * M_BLOCK_SIDE * P_BLOCK_SCALE) +
                                    y * M_BLOCK_SIDE * P_BLOCK_SCALE,
                             P_BLOCK_SCALE * M_BLOCK_SIDE - 2,
                             P_BLOCK_SCALE * M_BLOCK_SIDE - 2);
            }
        }
    }

    SDL_RenderPresent(mx->renderer);
}
