#pragma once

#include <stdbool.h>
#include "mem256.h"

/* Game configuration */
#define FPS 60
#define DAS 8

enum {
    K_Left, K_Right, K_Down, K_z, K_x, K_c, K_Space, K_q
};

/**
 * Store an entire gamestate.
 */
typedef struct {

    /* Current block top-left x position of bounding square */
    int bx;

    /* Current block top-left y position of bounding square */
    int by;

    /* Current block type identifier */
    int id;

    /* Current block rotation */
    int br;

    /* Current block that is superimposed over field */
    mem256_t block;

    /* Current block ghost */
    mem256_t ghost;

    /* Field state. The field's origin is at the bottom-left boundary */
    mem256_t field;

    /* How long each key was pressed down for
     *
     * 0 - left
     * 1 - right
     * 2 - down
     * 3 - z
     * 4 - x
     * 5 - c
     * 6 - space
     * 7 - quit
     * */
    int keystate[10];

    /* Id of current hold piece */
    int hold;

    /* Can we currently hold? */
    bool can_hold;

    /* Randomizer bag for next pieces */
    int bag[14];

    /* Current bag index */
    int bhead;

    /* Is the game running? */
    bool running;

    /* Should the current piece be locked? */
    bool lock_piece;

    /* Total frames elapsed for game */
    int64_t total_frames;

    /* Number of lines cleared */
    int lines_cleared;

    /* Time that this state was initialized */
    uint64_t start_time;

} mpstate;


void mpstate_init(mpstate *ms);

void mpstate_free(mpstate *ms);

bool mptet_collision(mpstate *ms, mem256_t *block,
        const int id, const int br, const int x, const int y);

bool mptet_move(mpstate *ms, int d);

bool mptet_rotate(mpstate *ms, int d);

int mptet_lineclear(mpstate *ms);

/**
 * Initial block values for all rotations. Each block is always considered to
 * be contained in a 4x4 bounding square. Since the field itself is 10 bits
 * wide, each row has 6 bits of 0 padding to align it to this 10-bit field
 * boundary. An example of a J-block as it is stored in memory is given:
 *
 * 39: .1........
 * 29: .1........
 * 19: 11........
 * 9:  ..........
 */
static uint64_t mptetd_block[7][4] = {
    /* I-block */
    {0x3c000000, 0x2008020080, 0x3c000000, 0x2008020080},
    /* T-block */
    {0x38040000, 0x4030040000, 0x100e0000, 0x4018040000},
    /* L-block */
    {0x38080000, 0xc010040000, 0x80e0000, 0x4010060000},
    /* J-block */
    {0x38020000, 0x40100c0000, 0x200e0000, 0x6010040000},
    /* S-block */
    {0x180c0000, 0x8030040000, 0x180c0000, 0x8030040000},
    /* Z-block */
    {0x30060000, 0x2018040000, 0x30060000, 0x2018040000},
    /* O-block */
    {0x18060000, 0x18060000, 0x18060000, 0x18060000}
};

/**
 * Block metadata. The following information is packed into a 64-bit integer.
 *
 * [ height | width | v-offset | h-offset ]
 *
 * Each slot uses 4-bits of data, which gives a usable range of [-7, 8]. Each
 * piece of data uses 1 hex character for its value.
 *
 * Height   - The height of the piece from the top of its bounding box.
 * Width    - The width of the piece from the left of its bouding box.
 * V-Offset - The number of empty rows above the piece.
 * H-Offset - The number of empty columns to the left of the piece.
 */
static uint64_t mptetd_meta[7][4] = {
    /* I-block */
    {0x2410, 0x4302, 0x2410, 0x4302},
    /* T-block */
    {0x3310, 0x3200, 0x3310, 0x3301},
    /* L-block */
    {0x3310, 0x3200, 0x3310, 0x3301},
    /* J-block */
    {0x3310, 0x3200, 0x3310, 0x3301},
    /* S-block */
    {0x3310, 0x3200, 0x3310, 0x3200},
    /* Z-block */
    {0x3310, 0x3301, 0x3310, 0x3301},
    /* O-block */
    {0x3311, 0x3311, 0x3311, 0x3311}
};

/**
 * Wallkick data. We only store right rotations. Left rotations are calculated
 * as the negative of these rotations.
 */
static uint64_t mptetd_wallk[2][4] = {
    { /* Non I-Blocks */
      0xa9a0190900, /* 0 -> R */
      0x2120910100, /* R -> 2 */
      0xa1a0110100, /* 2 -> L */
      0x2920990900, /* L -> 0 */
    },
    { /* I-Block */
      0x219a010a00, /* 0 -> R */
      0x9229020900, /* R -> 2 */
      0xa912090200, /* 2 -> L */
      0x1aa10a0100, /* L -> 0 */
    }
};

/**
 * Retrieve an integer from some bit-packed data.
 */
static inline int mptetd_get(uint64_t data, int offset)
{
    const int value = (data >> 4 * offset) & 15;
    return (value & 8) ? -(value & 7) : (value & 7);
}
