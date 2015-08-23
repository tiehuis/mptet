#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <gmp.h>

/* Current active block */
mpz_t block;

/* Current block type */
short type;

/* Current block rotation */
short rot;

/* Playfield */
mpz_t field;

/* Current hold piece */
mpz_t hold;

/* Randomizer : holds all next pieces */
mpz_t bag[6];

/* Current head of bag index */
short int bag_head;

/* Temporary copy variables */
mpz_t temp1, temp2;

/* Termios settings */
struct termios tnew, told;

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
 */

#define DATA_GET_PIECE(x) ((x) & 0xffffffffff)
#define DATA_GET_ROTATION_OFFSET(x) (((x) >> 40) & 0xf)


const int64_t bdata[][4] =
{
    { 0x00000a003c000000, 0x0000022008020080, 0x00000a003c000000, 0x0000022008020080 },
    { 0x00000a0038040000, 0x0000014030040000, 0x00000b00100e0000, 0x0000014018040000 },
    { 0x00000a0038080000, 0x000000c010040000, 0x00000c00080e0000, 0x0000014010060000 },
    { 0x00000a0038020000, 0x00000140100c0000, 0x00000a00200e0000, 0x0000016010040000 },
    { 0x00000b00180c0000, 0x0000008030040000, 0x00000b00180c0000, 0x0000008030040000 },
    { 0x00000a0030060000, 0x0000022018040000, 0x00000a0030060000, 0x0000022018040000 },
    { 0x00000b0018060000, 0x00000b0018060000, 0x00000b0018060000, 0x00000b0018060000 }
};

__attribute__((constructor)) void init__(void)
{
    // Change console mode on program entry
    tcgetattr(0, &told);
    memcpy(&tnew, &told, sizeof(struct termios));
    tnew.c_lflag &= ~(ICANON | ECHO);
    tnew.c_cc[VEOL] = 1;
    tnew.c_cc[VEOF] = 2;
    tcsetattr(0, TCSANOW, &tnew);

    // Init game data
    mpz_init2(block, 230);
    mpz_init2(field, 230);
    mpz_init2(temp1, 230);
    mpz_init2(temp2, 230);
    mpz_init2(hold, 230);

    bag_head = 0;

    for (int i = 0; i < 6; ++i)
        mpz_init2(bag[i], 230);

    type = rot = -1;
    srand(time(NULL));
}

__attribute__((destructor)) void exit__(void)
{
    // Revert console move on program exit
    tcsetattr(0, TCSANOW, &told);

    // Clear game data
    mpz_clear(block);
    mpz_clear(field);
    mpz_clear(temp1);
    mpz_clear(temp2);
    mpz_clear(hold);

    for (int i = 0; i < 6; ++i)
        mpz_clear(bag[i]);
}

// Utility functions
static int kbhit(void)
{
    struct timeval tv = {0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

// TODO: Fix Z spawn location
// Fix rotate into third offset

/* Z is incorrect */


#include <unistd.h>

// Fix wall collision
bool collision(void)
{
    // Need to keep track of bits in each row and determine equality
    // against all
    /*
    mpz_tdiv_q_2exp(temp2, temp1, 10);
    while (mpz_popcount(temp2) == 4)
        mpz_tdiv_q_2exp(temp2, temp2, 10);

    if (mpz_popcount(temp2) > 0)
        return false;
        */

    mpz_ior(temp2, field, temp1);

    // Currently stores offse   mpz_ior(temp, field, block);
    return
        /* Check block does not overlap */
        mpz_popcount(temp2) != mpz_popcount(temp1) + mpz_popcount(field)
        /* Check block has not exited below floor */
        || mpz_popcount(temp1) != 4
        /* Check block does not intersect a wall */
        || 0;
}

void place_block(void)
{
    mpz_ior(field, field, block);
}

void move_left(void)
{
    mpz_mul_2exp(temp1, block, 1);
    if (!collision())
        mpz_set(block, temp1);
}

void move_right(void)
{
    mpz_tdiv_q_2exp(temp1, block, 1);
    if (!collision())
        mpz_set(block, temp1);
}

bool move_down(void)
{
    bool flag = false;

    mpz_tdiv_q_2exp(temp1, block, 10);
    if (!collision()) {
        mpz_set(block, temp1);
        flag = true;
    }

    return flag;
}

bool at_bottom(void)
{
    mpz_tdiv_q_2exp(temp1, block, 10);
    return collision();
}

void move_rotateL(void)
{
    const size_t shift = mpz_sizeinbase(block, 2) +
        DATA_GET_ROTATION_OFFSET(bdata[type][rot]) - 40;
    rot = rot == 3 ? 0 : rot + 1;
    mpz_set_ui(temp1, DATA_GET_PIECE(bdata[type][rot]));
    mpz_mul_2exp(temp1, temp1, shift);
    if (!collision())
        mpz_set(block, temp1);
}

void move_rotateR(void)
{
    const size_t shift = mpz_sizeinbase(block, 2) +
        DATA_GET_ROTATION_OFFSET(bdata[type][rot]) - 40;
    rot = rot == 0 ? 3 : rot - 1;
    mpz_set_ui(temp1, DATA_GET_PIECE(bdata[type][rot]));
    mpz_mul_2exp(temp1, temp1, shift);
    if (!collision())
        mpz_set(block, temp1);
}

void random_block(void)
{
    type = rand() % 7;
    rot = 0;
    mpz_set_ui(block, DATA_GET_PIECE(bdata[type][rot]));
    mpz_mul_2exp(block, block, (type ? 207 : 197 ));
}

void move_harddrop(void)
{
    while (move_down()) {}
    place_block();
    random_block();
}

// Game loop

/*
static const int kb_map =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static int kb_back[10], kb_buffer[10] = {0};
*/

void update()
{
    int ch = getchar();
    switch (ch) {
        case 'h':
            move_left();
            break;
        case 'l':
            move_right();
            break;
        case 'j':
            move_down();
            break;
        case 'z':
            move_rotateR();
            break;
        case 'x':
            move_rotateL();
            break;
        case 'b':
            move_harddrop();
            break;
        case 'q':
            exit(0);
    }
    /*
    memcpy(kb_back, kb_buffer, sizeof(kb_buffer));

    // Get current keypresses
    while (kbhit()) {
        int ch = getch();
        if (kbmap[ch] > 0) {
            kb_buffer[kbmap[ch]]++;
        }
    }

    bool key_down = false;

    // If key was removed, update held value
    for (int i = 0; i < 10; ++i) {
        if (kb_back[i] == kb_buffer[i])
            kb_buffer[i] = 0;

        if (kb_buffer[i])
            key_down = true;
    }

    // Filter out illogical movement, i.e. left + right at same time
    if (kb_buffer[0] && kb_buffer[1])
        kb_buffer[0] = kb_buffer[1] = 0;

    if (kb_buffer[3] && kb_buffer[4])
        kb_buffer[3] = kb_buffer[4] = 0;

    // If a key is pressed, deal with it, else do nothing
    if (key_down) {
        // Deal with hard drops first
        if (kb_buffer[2] == 1)
            move_harddrop();

        // Deal with rotation second - only deal with first frame pressed
        if (kb_buffer[3] == 1)
            move_rotateL();
        else if (kb_buffer[4] == 1)
            move_rotateR();

        // Deal with movement third
        if (kb_buffer[5] == 1) {
            move_down();
            frames_since_last = 0;
        }

        if (kb_buffer[0])
            move_left();
        else if (kb_buffer[1])
            move_right();
    }

    // Drop piece every 'gravity' frames
    if (total_frames % gravity == 0)
        move_down();


    if (at_bottom()) {
        bottom_frame_count++;
    }
    if (!at_bottom() && bottom_frame_count != 0) {
        bottom_frame_count = 0;
    }
    if (at_bottom() && bottom_frame_count > LOCK_DELAY) {
        move_placeblock();
        bottom_frame_count = 0;
   }
   */

    // Update the field
}

void render()
{
    printf("\033[2J%.2f Frames:\n", current_frames);
    for (int i = 219; i >= 0; --i) {
        printf("%c", (mpz_tstbit(field, i) | mpz_tstbit(block, i)) ? '.' : ' ');
        if (i % 10 == 0)
            printf("\n");
    }

    printf("----------\n");
}

#define T_MAXFPS 60
#define __NANO_ADJUST 1000000000ULL

static int64_t get_nanotime(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t) (ts.tv_sec * __NANO_ADJUST + ts.tv_nsec);
}

int run(void)
{
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
        update();

        // Game render
        // Draw image to terminal here
        render();

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
        while (get_nanotime() < fps_delay + __NANO_ADJUST / T_MAXFPS) {}

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

    return 0;
}

int main(void)
{
    random_block();
    return run();
}
