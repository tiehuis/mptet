#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <sched.h>
#include <gmp.h>

//TODO: Redo main game loop to avoid using cpu too much

#define MPTET_USE_TERMINAL    0
#define MPTET_USE_FRAMEBUFFER 1
#define MPTET_USE_DIRECTFB    2

#ifndef MPTET_RENDERER
#   define MPTET_RENDERER MPTET_USE_TERMINAL
#endif

/* Current active block */
mpz_t block;

/* A set ghost piece would save on computations */
mpz_t ghost;

/* Current block type */
short type;

/* Current block rotation */
short rot;

/* Playfield */
mpz_t field;

/* Current hold piece */
mpz_t hold;

/* Randomizer : holds all next pieces */
short bag[14];

/* Current head of bag index */
short bag_head;

/* Temporary copy variables */
mpz_t temp1, temp2, temp3;

bool recalc_ghost;

/* Other gameplay data */
static double current_frames    = 0;
static int64_t total_frames     = 0;
static int64_t frame_since_last = 0;
static int bottom_frame_count   = 0;

/* Tetrimino data
 *
 * The following array stores all information about each rotation of each
 * block.
 *
 * The lower 40 bits stores the rotations of each piece.
 *
 *  I:
 *  0010000000 0000000000
 *  0010000000 1111000000
 *  0010000000 0000000000
 *  0010000000 0000000000
 *
 *  T:
 *  0000000000 0100000000 0000000000 0100000000
 *  1110000000 1100000000 0100000000 0110000000
 *  0100000000 0100000000 1110000000 0100000000
 *  0000000000 0000000000 0000000000 0000000000
 *
 *  L:
 *  0000000000 0100000000 0000000000 0110000000
 *  1110000000 0100000000 1000000000 0100000000
 *  0010000000 1100000000 1110000000 0100000000
 *  0000000000 0000000000 0000000000 0000000000
 *
 *  J:
 *  0000000000 0100000000 0000000000 0110000000
 *  1110000000 0100000000 1000000000 0100000000
 *  0010000000 1100000000 1110000000 0100000000
 *  0000000000 0000000000 0000000000 0000000000
 *
 *  S:
 *  0000000000 1000000000
 *  0110000000 1100000000
 *  1100000000 0100000000
 *  0000000000 0000000000
 *
 *  Z:
 *  0000000000 0010000000
 *  1100000000 0110000000
 *  0110000000 0100000000
 *  0000000000 0000000000
 *
 *  O:
 *  0000000000
 *  0110000000
 *  0110000000
 *  0000000000
 *
 * Bits 40-44 stores the offsets of the piece from the origin of the containing
 * 4x10 box.
 *
 * Bits 45-48 stores the width of the piece, in order to determine if the piece
 * intersects any walls.
 *
 * Bits 49-52 stores the number of empty columns before each piece.
 */

/* Correct the modulo operation to give a true modulo and not remainder */
#define mod(x, y) ((((x) % (y)) + (y)) % (y))

/* Bit shift an mpz_t. Negative values correspond to right shifts.
 * rop and op1 are of type mpz_t, op2 is a signed integer. */
#define mpz_bshift(rop, op1, op2)                               \
    do {                                                        \
        if (op2 >= 0)                                           \
            mpz_mul_2exp(rop, op1, (unsigned long) (op2));      \
        else if (op2 < 0)                                       \
            mpz_tdiv_q_2exp(rop, op1, (unsigned long) -(op2));  \
    } while (0)

/* Get the current index of the leading bit of x */
#define LEADING(x) mpz_sizeinbase(x, 2)

/* Extract piece bit data */
#define PIECE(type, rot) ((bdata[type][rot]) & 0xffffffffff)

/* Extract offset to top of a 4x4 grid */
#define OFFSET(type, rot) (((bdata[type][rot]) >> 40) & 0xf)

/* Extract width of the piece */
#define WIDTH(type, rot) (((bdata[type][rot]) >> 44) & 0xf)

/* Extract # of leading columns */
#define SPACE(type, rot) (((bdata[type][rot]) >> 48) & 0xf)

/* Number of different types of tetriminoes */
#define NUMBER_OF_PIECES 7

// TODO: Add wallkick data somewhere in here
static const int64_t bdata[NUMBER_OF_PIECES][4] =
{
    { 0x00004a003c000000, 0x0002122008020080, 0x00004a003c000000, 0x0002122008020080 },
    { 0x00003a0038040000, 0x0000214030040000, 0x00003b00100e0000, 0x0001214018040000 },
    { 0x00003a0038080000, 0x000020c010040000, 0x00003c00080e0000, 0x0001214010060000 },
    { 0x00003a0038020000, 0x00002140100c0000, 0x00003a00200e0000, 0x0001216010040000 },
    { 0x00003b00180c0000, 0x0000208030040000, 0x00003b00180c0000, 0x0000208030040000 },
    { 0x00003a0030060000, 0x0001222018040000, 0x00003a0030060000, 0x0001222018040000 },
    { 0x00012b0018060000, 0x00012b0018060000, 0x00012b0018060000, 0x00012b0018060000 }
};
/* Determine if the block given by op is in a collision state on the field.
 * This function clobbers the global 'temp3' variable, so wherever this is
 * called, should not store any data required after the function call in */
bool collision(const mpz_t op)
{
    mpz_ior(temp3, field, op);

    return
        /* Block should not overlap with field anywhere */
        mpz_popcount(temp3) != mpz_popcount(op) + mpz_popcount(field)
        /* Block should not have descended beneath the floor */
     || mpz_popcount(op) != 4
        /* Check if the block has extended across a wall-boundary. This
         * does not work for an upwards I-block, and extra checks are required
         * for this edge case */
     || ((LEADING(op) + OFFSET(type, rot) - 1 -
          SPACE(type, rot)) % 10 < WIDTH(type, rot) - 1);
}

/* Place a block on the field. A call to collision would usually be called
 * prior. */
#define place_block() mpz_ior(field, field, block);

/* Move the currently active block. Check for upwards I-block case also.
 * Returns if the the block was successfully moved. -1 moves right, 1 moves
 * left, all other values are invalid. */
bool move_horizontal(const int direction)
{
    mpz_bshift(temp1, block, direction);
    if (!collision(temp1) && (type || rot & 2 ? true : LEADING(block) % 10 != (direction < 0 ? 1 : 0))) {
        mpz_set(block, temp1);
        recalc_ghost = true;
        return true;
    }
    else {
        return false;
    }
}

bool move_down(void)
{
    mpz_bshift(temp1, block, -10);
    if (!collision(temp1)) {
        mpz_set(block, temp1);
        recalc_ghost = true;
        return true;
    }
    else {
        return false;
    }
}

/* Returns if the block cannot be moved any lower. */
bool at_bottom(const mpz_t block)
{
    mpz_bshift(temp1, block, -10);
    return collision(temp1);
}

/* 1 indicates right rotation, -1 indicates left rotation. Any other values
 * are considered invalid */
// TODO: Rotation fails near edges on occassion
bool move_rotate(int direction)
{
    /* This may be negative the new rotation is in bottom 3 rows so cast to an
     * integer */
    const int shift = (int) LEADING(block) + OFFSET(type, rot) - 40;

    const int newrot = mod(rot + direction, 4);
    mpz_set_ui(temp1, PIECE(type, newrot));
    mpz_bshift(temp1, temp1, shift);

    if (!collision(temp1)) {
        mpz_set(block, temp1);
        rot = newrot;
        recalc_ghost = true;
        return true;
    }
    else {
        return false;
    }
}

// Use a random permutation style algorithm here instead,
// keep effectively two bags, (one of double size), which allows
// us to permute a new bag whenever the old is used up. This will allow
// a 7-piece lookahead which will suffice in most cases
void shuffle(int start)
{
    for (int i = 0; i < 7; ++i) {
        bag[start + i] = i; // half of the bag with all pieces
    }

    // Perform a Fisher-Yates shuffle
    for (int i = 0; i < 7; ++i) {
        int j = (rand() % (7 - i)) + i;
        int temp = bag[start + j];
        bag[start + j] = bag[start + i];
        bag[start + i] = temp;
    }
}

void random_block(void)
{
    recalc_ghost = true;
    type = bag[bag_head];
    rot = 0;
    mpz_set_ui(block, PIECE(type, rot));
    mpz_bshift(block, block, (type ? 207 : 197));

    // Determine new bag head and if we need to shuffle and if so, what part
    // of the bag
    bag_head = bag_head + 1 == 14 ? 0 : bag_head + 1;
    const int start = bag_head == 7 ? 0 : bag_head == 0 ? 7 : -1;

    if (start != -1)
        shuffle(start);
}

bool move_harddrop(void)
{
    while (move_down()) {}

    place_block();
    random_block();

    if (!collision(block))
        return true;
    else
        return false;
}

// Check multiple line clears are indeed clearing correctly
// TODO: Lines are not cleared correctly on occassion
int clear_lines(void)
{
    int cleared = 0;

    /* line bits set */
    mpz_set_ui(temp1, 0x3ff);

    /* Careful changing this upper bound, as using mpz_setbit will not
     * do any bounds checking */
    for (int i = 0; i < LEADING(field) / 10 + 1 - cleared; ++i) {
        mpz_and(temp2, field, temp1);

        // Found a completely set line */
        if (mpz_popcount(temp2) == 10) {
            /* Lower bits set to 1 for flag use */
            mpz_set_ui(temp2, 0);
            mpz_setbit(temp2, 10*i + 1);
            mpz_sub_ui(temp2, temp2, 1);

            /* Zero lower 10*i + 10 bits and place the
             * stored bits in temp2 */
            mpz_set_ui(temp3, 0);
            mpz_setbit(temp3, 231);
            mpz_sub_ui(temp3, temp3, 1);
            mpz_xor(temp3, temp3, temp2);

            /* Shift field removing cleared line */
            mpz_and(temp2, field, temp2);   // field bottom
            mpz_and(temp3, field, temp3);   // field top

            /* Copy parts back to field */
            mpz_tdiv_q_2exp(field, temp3, 10);
            mpz_ior(field, field, temp2);

            ++cleared;

            --i; // We just cleared the ith line, so the new current now needs
                 // to be rechecked
        }

        mpz_bshift(temp1, temp1, 10);
    }

    return cleared;
}

/*
 *  0 - left
 *  1 - right
 *  2 - down
 *  3 - z
 *  4 - x
 *  5 - space
 *  6 - quit
 */
int keyboardState[10] = { 0 };

/* Include appropriate render functions */
#if MPTET_RENDERER == MPTET_USE_TERMINAL
#   include "gui/term.c"
#elif MPTET_RENDERER == MPTET_USE_FRAMEBUFFER
#   include "gui/framebuffer.c"
#elif MPTET_RENDERER == MPTET_USE_DIRECTFB
#   include "gui/directfb.c"
#else
#   error "no gui specified"
#endif

#define DAS_DELAY 4

/*
 *  0 - lf
 *  1 - right
 *  2 - down
 *  3 - z
 *  4 - x
 *  5 - space
 *  6 - quit
 */
bool update(void)
{
    if (keyboardState[0] == 1 || keyboardState[0] > DAS_DELAY)
        move_horizontal(1);
    else if (keyboardState[1] == 1 || keyboardState[1] > DAS_DELAY)
        move_horizontal(-1);

    if (keyboardState[2] == 1 || keyboardState[2] > DAS_DELAY)
        move_down();

    if (keyboardState[3] == 1)
        move_rotate(-1);
    else if (keyboardState[4] == 1)
        move_rotate(1);

    if (keyboardState[5] == 1) {
        if (!move_harddrop())
            return false;
        clear_lines();
    }

    if (keyboardState[6])
        return false;

    // Add some gravity. Ensure that we don't down drop more than once
    // if we update multiple times per frame
    // Perform Gravity
    static bool status = false;
    if (total_frames % 60 == 0 && status == false) {
        if (at_bottom(block)) {
            if (!move_harddrop())
                return false;
            clear_lines();
        }
        else {
            move_down();
        }

        status = true;
    }
    else if (total_frames % 60 == 1) {
        status = false;
    }

    // Determine if we need to recalculate ghost and if so, do ti
    if (recalc_ghost == true) {
        mpz_set(ghost, block);
        while (!at_bottom(ghost))
            mpz_bshift(ghost, ghost, -10);
        recalc_ghost = false;
    }

    return true;
}

#define T_MAXFPS 30
#define __NANO_ADJUST 1000000000ULL

static int64_t get_nanotime(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t) (ts.tv_sec * __NANO_ADJUST + ts.tv_nsec);
}

/* Main game loop adapted from NullpoMino */
// TODO: Doesn't sleep enough, too much CPU usage currently
int run(void)
{
    random_block();
    gui_render();

    bool sleep_flag, running;
    double actual_fps;
    int64_t time_diff, sleep_time, sleep_time_ms, oversleep_time, after_time,
            maxfps_current, period_current, before_time, prev_time, fps_delay,
            framecount, calc_interval;

    oversleep_time = 0;
    maxfps_current = T_MAXFPS;
    period_current = (int64_t) (1.0 / maxfps_current * __NANO_ADJUST);
    before_time    = get_nanotime();
    prev_time      = before_time;
    framecount     = 0;
    calc_interval  = 0;
    actual_fps     = 0.0;
    fps_delay      = get_nanotime();
    running        = true;

    while (running) {
        // Update globals
        current_frames = actual_fps;
        total_frames = framecount;

        // Game update
        // Get new key state here, and move the pieces accordinglyA
        gui_update();

        if (!update())
            return 0;

        // Game render
        // Draw image to terminal here
        gui_render();

        sleep_flag = false;
        after_time = get_nanotime();
        time_diff  = after_time - before_time;
        sleep_time = (period_current - time_diff) - oversleep_time;
        sleep_time_ms = sleep_time / 1000000LL;

        // Always use perfect_fps
        oversleep_time = 0;

        if (maxfps_current > T_MAXFPS + 5)
            maxfps_current = T_MAXFPS + 5;

        // Never use perfect yield
        while (get_nanotime() < fps_delay + __NANO_ADJUST / T_MAXFPS) {
            sched_yield();
            usleep(1000);
        }

        fps_delay += __NANO_ADJUST / T_MAXFPS;
        sleep_flag = true;

        if (!sleep_flag) {
            oversleep_time = 0;
            fps_delay = get_nanotime();
        }

        before_time = get_nanotime();

        // Calculate fps
        framecount++;
        calc_interval += period_current;

        if (calc_interval >= __NANO_ADJUST) {
            const int64_t time_now     = get_nanotime(),
                          elapsed_time = time_now - prev_time;

            actual_fps    = ((double) framecount / elapsed_time) * __NANO_ADJUST;
            framecount    = 0;
            calc_interval = 0;
            prev_time     = time_now;
        }
    }

    return 1;
}

int main(int argc, char **argv)
{
    gui_init(argc, argv);

    // Init game data
    mpz_init2(block, 230);
    mpz_init2(field, 230);
    mpz_init2(temp1, 230);
    mpz_init2(temp2, 230);
    mpz_init2(hold, 230);
    mpz_init2(ghost, 230);

    bag_head = 0;
    srand(time(NULL));

    // Construct the initial bag by shuffling twice
    shuffle(0);
    shuffle(7);

    /* Run main program */
    run();

    // Clear game data
    mpz_clear(ghost);
    mpz_clear(block);
    mpz_clear(field);
    mpz_clear(temp1);
    mpz_clear(temp2);
    mpz_clear(temp3);
    mpz_clear(hold);

    gui_free();
    return 0;
}
