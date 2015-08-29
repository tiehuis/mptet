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
mpz_t temp1, temp2, temp3;

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
 *
 * Bits 45-48 stores the width of the piece, in order to determine if the piece
 * intersects any walls.
 *
 * Bits 49-52 stores the number of empty columns before each piece.
 */

#define debug(...) printf(__VA_ARGS__), usleep(50000)

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
#define __LEADING(x) mpz_sizeinbase(x, 2)

/* Extract piece bit data */
#define __PIECE(type, rot) ((bdata[type][rot]) & 0xffffffffff)

/* Extract offset to top of a 4x4 grid */
#define __OFFSET(type, rot) (((bdata[type][rot]) >> 40) & 0xf)

/* Extract width of the piece */
#define __WIDTH(type, rot) (((bdata[type][rot]) >> 44) & 0xf)

/* Extract # of leading columns */
#define __SPACE(type, rot) (((bdata[type][rot]) >> 48) & 0xf)

// TODO: Rotation width needs to take into account where the greatest width
// occurs and use that
const int64_t bdata[][4] =
{
    { 0x00004a003c000000, 0x0002122008020080, 0x00004a003c000000, 0x0002122008020080 },
    { 0x00003a0038040000, 0x0000214030040000, 0x00003b00100e0000, 0x0001214018040000 },
    { 0x00003a0038080000, 0x000020c010040000, 0x00003c00080e0000, 0x0001214010060000 },
    { 0x00003a0038020000, 0x00002140100c0000, 0x00003a00200e0000, 0x0001216010040000 },
    { 0x00003b00180c0000, 0x0000208030040000, 0x00003b00180c0000, 0x0000208030040000 },
    { 0x00003a0030060000, 0x0001222018040000, 0x00003a0030060000, 0x0001222018040000 },
    { 0x00012b0018060000, 0x00012b0018060000, 0x00012b0018060000, 0x00012b0018060000 }
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
    mpz_init2(temp3, 240);
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
    mpz_clear(temp3);
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

#include <unistd.h>

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
     || ((__LEADING(op) + __OFFSET(type, rot) - 1 -
          __SPACE(type, rot)) % 10 < __WIDTH(type, rot) - 1);
}

/* Place a block on the field. A call to collision would usually be called
 * prior. */
void place_block(void)
{
    mpz_ior(field, field, block);
}

/* Move the currently active block left. Check for upwards I-block case also.
 * Returns if the the block was successfully moved. */
bool move_left(void)
{
    mpz_bshift(temp1, block, 1);
    if (!collision(temp1) && (type || rot & 2 ?: __LEADING(block) % 10 != 0)) {
        mpz_set(block, temp1);
        return true;
    }
    else {
        return false;
    }
}

/* Move the currently active block right. Check for upwards I-block case also.
 * Returns if the the block was successfully moved. */
bool move_right(void)
{
    mpz_bshift(temp1, block, -1);
    if (!collision(temp1) && (type || rot & 2 ?: __LEADING(block) % 10 != 1)) {
        mpz_set(block, temp1);
        return true;
    }
    else {
        return false;
    }
}

/* Move the currently active block down one square. Returns if the block was
 * successfully moved. */
bool move_down(void)
{
    mpz_bshift(temp1, block, -10);
    if (!collision(temp1)) {
        mpz_set(block, temp1);
        return true;
    }
    else {
        return false;
    }
}

/* Returns if the block cannot be moved any lower. */
bool at_bottom(void)
{
    mpz_bshift(temp1, block, -10);
    return collision(temp1);
}

// TODO: Wallkicks
/* Attempt to rotate a block left. Wallkicks are Akira style.
 * Returns if the piece rotated successfully. */
bool move_rotateL(void)
{
    /* This may be negative the new rotation is in bottom 3 rows so cast to an
     * integer */
    const int shift = (int) __LEADING(block) + __OFFSET(type, rot) - 40;

    rot = rot == 3 ? 0 : rot + 1;
    mpz_set_ui(temp1, __PIECE(type, rot));
    mpz_bshift(temp1, temp1, shift);

    if (!collision(temp1)) {
        mpz_set(block, temp1);
        return true;
    }
    else {
        return false;
    }
}

bool move_rotateR(void)
{
    const int shift = (int) __LEADING(block) + __OFFSET(type, rot) - 40;

    rot = rot == 0 ? 3 : rot - 1;
    mpz_set_ui(temp1, __PIECE(type, rot));
    mpz_bshift(temp1, temp1, shift);

    if (!collision(temp1)) {
        mpz_set(block, temp1);
        return true;
    }
    else {
        return false;
    }
}

// TODO: Ghost piece on bottom

void random_block(void)
{
    type = rand() % 7;
    rot = 0;
    mpz_set_ui(block, __PIECE(type, rot));
    mpz_bshift(block, block, (type ? 207 : 197));
}

void move_harddrop(void)
{
    while (move_down()) {}
    place_block();
    random_block();
}

int clear_lines(void)
{
    int cleared = 0;

    /* line bits set */
    mpz_set_ui(temp1, 0x3ff);

    /* Careful changing this upper bound, as using mpz_setbit will not
     * do any bounds checking */
    for (int i = 0; i < __LEADING(field) / 10 + 1; ++i) {
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

// Game loop

enum {
    BUTTON_UP, BUTTON_DOWN, BUTTON_RIGHT, BUTTON_LEFT,
    BUTTON_z, BUTTON_x, BUTTON_c, BUTTON_RETURN, BUTTON_COUNT
};

int buttonTime[9] = { 0 };

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
            clear_lines();
            break;
        case 'q':
            exit(0);
    }
}

void render()
{
    printf("\033[2J%.2f Frames:\n", current_frames);
    printf("|");
    for (int i = 219; i >= 0; --i) {
        printf("%c", (mpz_tstbit(field, i) | mpz_tstbit(block, i)) ? '.' : ' ');
        if (i % 10 == 0)
            printf("|\n|");
    }

    printf("----------|\n");
}

#define T_MAXFPS 60
#define __NANO_ADJUST 1000000000ULL

static int64_t get_nanotime(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t) (ts.tv_sec * __NANO_ADJUST + ts.tv_nsec);
}

/* Main game loop adapted from NullpoMino */
int run(void)
{
    render();

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

int main(int argc, char **argv)
{
    random_block();
    return run();
}