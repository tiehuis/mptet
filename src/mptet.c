/**
 * mptet (Multiple Precision Tetris)
 *
 * This was originally implemented using GMP, but has since been reduced to
 * implementing a 256-bit memory region instead, the name however hasn't
 * changed with this.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "mptet.h"
#include "gfx.h"
#include "mem256.h"
#include "ts.h"

/**
 * Determine if the given block will collide with the field or wall.
 */
bool mptet_collision(mpstate *ms, mem256_t *block,
        const int id, const int br, const int x, const int y)
{
    /* Right wall collision */
    if (x + mptetd_get(mptetd_meta[id][br], 2) > 10)
        return true;

    /* Left wall collision */
    if (x + mptetd_get(mptetd_meta[id][br], 0) < 0)
        return true;

    /* Floor collision */
    if (y + mptetd_get(mptetd_meta[id][br], 3) < 0)
        return true;

    /* Field collision */
    mem256_t tmp = *block;
    mem256_ior(&tmp, &ms->field);

    /* All tetriminoes are made up of 4 bits */
    return mem256_popcnt(&tmp) != mem256_popcnt(&ms->field) + 4;
}

/**
 * Attempt to move a block, returning if it was successful or not.
 */
bool mptet_move(mpstate *ms, int d)
{
    mem256_t tmp = ms->block;
    mem256_bshift(&tmp, d);

    /* Calculate x and y offsets from the given shift */
    const int x = ms->bx - d % 10;
    const int y = ms->by + d / 10;

    /* The move had no collision so repeat this with the actual block */
    if (!mptet_collision(ms, &tmp, ms->id, ms->br, x, y)) {
        ms->block = tmp;
        ms->bx = x;
        ms->by = y;
        return true;
    }
    else {
        return false;
    }
}

/**
 * Attempt to rotate a block, returning whether or not this was
 * successful.
 */
bool mptet_rotate(mpstate *ms, int d)
{
    const int br = (ms->br + 4 + d) % 4;

    mem256_t tmp;
    mem256_zero(&tmp);
    tmp.limb[0] = mptetd_block[ms->id][br];

    /**
     * Mirrored x axis means we must take the negative.
     * Blocks are initially spawned in at 0, 3, so an offset of 3 must be
     * applied to correct for this.
     */
    mem256_bshift(&tmp, -ms->bx + 10 * (ms->by - 3));

    /* Wallkick check */
    for (int test = 0; test < 5; ++test) {
        int bx = ms->bx;
        int by = ms->by;
        mem256_t wtmp;
        wtmp = tmp;

        /* Obtain the correct wallkick value for the given block and test */
        uint64_t *block = ms->id ? mptetd_wallk[0] : mptetd_wallk[1];
        const uint64_t value = block[d < 0 ? (ms->br + 3) & 3 : ms->br];

        /* Unpack the x, y values */
        const int tx = mptetd_get(value, 2 * test);
        const int ty = mptetd_get(value, 2 * test + 1);

        /* Set the block values to test */
        if (d < 0) {
            mem256_bshift(&wtmp, -tx + 10 * -ty);
            bx -= -tx;
            by -= -ty;
        }
        else {
            mem256_bshift(&wtmp, tx + 10 * ty);
            bx -= tx;
            by -= ty;
        }

        /* Check if we encountered a collision */
        if (!mptet_collision(ms, &wtmp, ms->id, br, bx, by)) {
            ms->block = wtmp;
            ms->br = br;
            ms->bx = bx;
            ms->by = by;
            return true;
        }
    }

    return false;
}

/**
 * Shuffle a 7 element run in a 14 element bag.
 *
 * 'region' should either be one of 0, or 7.
 */
void mptet_shuffle(mpstate *ms, int region)
{
    /* Insert all pieces in the bag */
    for (int i = 0; i < 7; ++i)
        ms->bag[region + i] = i;

    /* Perform a Fisher-Yates shuffle */
    for (int i = 0; i < 7; ++i) {
        const int j = (rand() % (7 - i)) + i;
        const int tmp = ms->bag[region + j];
        ms->bag[region + j] = ms->bag[region + i];
        ms->bag[region + i] = tmp;
    }
}

void mpstate_init(mpstate *ms)
{
    srand(time(NULL));

    ms->running = true;
    ms->lock_piece = false;
    ms->can_hold = true;
    ms->lines_cleared = 0;
    memset(ms->keystate, 0, sizeof(ms->keystate));
    mem256_zero(&ms->field);
    mem256_zero(&ms->ghost);

    ms->hold = -1;

    ms->total_frames = 0;

    /* Initialize random bag */
    ms->bhead = 0;
    mptet_shuffle(ms, 0);
    mptet_shuffle(ms, 7);
}

void mpstate_free(mpstate *ms)
{
    (void) ms;
}

void mptet_set_block(mpstate *ms, const int id)
{
    /* When generating a new block, we can now hold it if
     * we could not before */
    ms->can_hold = true;
    ms->id = id;
    ms->br = 0;

    ms->bx = 3;
    ms->by = 22;

    mem256_zero(&ms->block);
    ms->block.limb[0] = mptetd_block[ms->id][ms->br];
    mem256_bshift(&ms->block, 187);
}

void mptet_set_random_block(mpstate *ms)
{
    const int id = ms->bag[ms->bhead];

    /* Update bag head */
    ms->bhead = (ms->bhead + 1) % 14;
    if (ms->bhead % 7 == 0)
        mptet_shuffle(ms, (ms->bhead + 7) % 14);

    /* Set the block */
    mptet_set_block(ms, id);
}

/**
 * Try and hold the current piece.
 */
void mptet_hold(mpstate *ms)
{
    if (ms->can_hold) {
        /* If we have a piece to swap */
        if (ms->hold != -1) {
            int tmp = ms->hold;
            ms->hold = ms->id;
            mptet_set_block(ms, tmp);
        }
        else {
            ms->hold = ms->id;
            mptet_set_random_block(ms);
        }

        ms->can_hold = false;
    }
}

void mptet_hard_drop(mpstate *ms)
{
    while (mptet_move(ms, -10));
    ms->lock_piece = true;
}

int mptet_lineclear(mpstate *ms)
{
    int cleared = 0;

    /* Generate our mask to check lines */
    mem256_t mask;
    mem256_zero(&mask);
    mask.limb[0] = 0x3ff;

    /* Determine the leading bit on the field so lineclear checks are faster */
    int leading = mem256_highbit(&ms->field);
    int y = 0;

    /* Iterate over each line on the field */
    while (y < leading) {
        mem256_t tmp = ms->field;
        mem256_and(&tmp, &mask);

        /* If the current isn't cleared go to the next line */
        if (mem256_popcnt(&tmp) != 10) {
            y += 10;
            mem256_bshift(&mask, 10);
            continue;
        }

        /* We check seperately if we are clearing the bottom row, and if so we
         * perform a simple shift instead of masking. This is more common than
         * one would think if playing with a standard well. */
        if (!y) {
            mem256_bshift(&ms->field, -10);
        }
        else {
            mem256_t lmask;
            mem256_zero(&lmask);
            mem256_fillones(&lmask, 0, y - 1);

            /* Save the region beneath the line that needs to be cleared */
            mem256_t lower = ms->field;
            mem256_and(&lower, &lmask);

            /* Shift the field one row and zero the bottom mask. This will
             * remove one row from the bottom. */
            mem256_bshift(&ms->field, -10);
            mem256_negate(&lmask);
            mem256_and(&ms->field, &lmask);

            /* Place the bottom rows back onto the field */
            mem256_ior(&ms->field, &lower);
        }

        /* If we cleared a line. the leading value is now reduced but we still
         * need to recheck the current line. */
        leading -= 10;

        /* We cannot clear more than 4 lines at once */
        if (++cleared >= 4)
            break;
    }

    return cleared;
}

/**
 * Deal with keypresses and updating of logic.
 */
void mptet_update(mpstate *ms)
{
    /* Horizontal movement */
    if (ms->keystate[K_Left] == 1 || ms->keystate[K_Left] > DAS)
        mptet_move(ms, 1);
    else if (ms->keystate[K_Right] == 1 || ms->keystate[K_Right] > DAS)
        mptet_move(ms, -1);

    /* Vertical movement */
    if (ms->keystate[K_Down] || ms->keystate[K_Down] > DAS)
        mptet_move(ms, -10);

    /* Rotation */
    if (ms->keystate[K_z] == 1)
        mptet_rotate(ms, -1);
    else if (ms->keystate[K_x] == 1)
        mptet_rotate(ms, 1);

    /* Hold piece */
    if (ms->keystate[K_c] == 1)
        mptet_hold(ms);

    /* Hard drop */
    if (ms->keystate[K_Space] == 1)
        mptet_hard_drop(ms);

    /* Quit toggle */
    if (ms->keystate[K_q])
        ms->running = false;

    /* Do we need to lock the current piece? Then add it to field,
     * spawn a new block and check for line clears */
    if (ms->lock_piece) {
        mem256_ior(&ms->field, &ms->block);
        mptet_set_random_block(ms);
        ms->lines_cleared += mptet_lineclear(ms);
        ms->lock_piece = false;
    }

    /* Recalc ghost every frame for now */
    mem256_t tmp = ms->block;
    const int bx = ms->bx;
    const int by = ms->by;

    while (mptet_move(ms, -10));

    ms->ghost = ms->block;
    ms->block = tmp;
    ms->bx = bx;
    ms->by = by;

    /* Perform some gravity. Ensure we don't down drop multiple times
     * per frame even if pressing. */
    if ((ms->total_frames & 63) == 0) {
        mptet_move(ms, -10);
    }

    /* Check the end condition */
    if (ms->lines_cleared >= 40)
        ms->running = false;
}

#if !defined(MP_NO_GFX)
void mptet_tick(mpstate *ms, mpgfx *mx)
{
    /* Update keyboard */
    mpgfx_update(ms, mx);

    /* Update game state by one tick */
    mptet_update(ms);

    /* Render the current frame */
    mpgfx_render(ms, mx);

    ms->total_frames++;
}

int main(int argc, char **argv)
{
    mpstate ms;
    mpgfx mx;

    mpstate_init(&ms);
    mpgfx_init(&mx, &argc, &argv);

    mptet_set_random_block(&ms);
    mpgfx_render(&ms, &mx);

    ms.start_time = ts_get_current_time();

    /* This loop is quite naive, and assumes that we can always perform a game
     * tick and update the field within 16ms. This should be no trouble for the
     * most part but we should take into account cases where this may not
     * always be the case.
     */
    while (ms.running) {
        const uint64_t start = ts_get_current_time();

        mptet_tick(&ms, &mx);

        ts_sleep(start + TS_IN_A_SECOND / FPS - ts_get_current_time());
    }

    printf("%" PRIu64 "\n", ms.total_frames);

    /* Calculating time from frames provides a much more accurate timing */
    printf("%lfs\n", (double) (ts_get_current_time() - ms.start_time) / TS_IN_A_SECOND);
    printf("%d\n", ms.lines_cleared);

    mpstate_free(&ms);
    mpgfx_free(&mx);
}
#endif /* !defined(MP_NO_GFX) */
