/**
 * mem256.h
 *
 * Implements a 256-bit contiguous memory block with support for fast shifting
 * across the entire memory block.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    uint64_t limb[4]; /* Least to Most Significant */
} mem256_t;

/**
 * Return the number of 1-bits set in a 64-bit integer.
 */
#if defined(__GNUC__)
#   define mem256_64popcnt(x) __builtin_popcountll(x)
#else
static int mem256_64popcnt(uint64_t x)
{
    int count = 0;
    while (x) {
        x &= x - 1;
        count++;
    }
    return count;
}
#endif

/**
 * Return the index of the highest 1-bit set in a 64-bit integer.
 */
#if defined(__GNUC__)
#   define mem256_64highbit(x) (64 - __builtin_clzll(x))
#else
static int mem256_64highbit(uint64_t x)
{
    int highbit = 0;

    while (x > 0x10000) {
        highbit += 16;
        x >>= 16;
    }

    while (x) {
        highbit++;
        x >>= 1;
    }

    return highbit;
}
#endif

/**
 * Shift the entire 256 memory block left by the specified shift. Any shift
 * value > 255 is first truncated before being applied.
 *
 * Return true if overflow occured during shift, else false.
 */
bool mem256_lshift(mem256_t *rop, int shift)
{
    bool overflow = false;
    const int shift_type = (shift & 255) >> 6;

    /* Normalize shift into the region of a single limb */
    shift &= 63;

    /* Mask for obtaining specific regions of limb */
    const uint64_t hmask = ((1ULL << shift) - 1) << (64 - shift);

    switch (shift_type) {
    case 0:; { /* Less than one limb shift: (0 <= shift < 64) */

        #define SHIFT_LIMB(x)                                          \
        do {                                                           \
            const uint64_t upper = rop->limb[(x)] & hmask;             \
            rop->limb[(x) + 1] |= upper >> (64 - shift);               \
            rop->limb[(x)] <<= shift;                                  \
        } while (0)

        overflow = rop->limb[3] & hmask;
        rop->limb[3] <<= shift;

        /* This could likely be vectorized */
        SHIFT_LIMB(2);
        SHIFT_LIMB(1);
        SHIFT_LIMB(0);

        break;

        #undef SHIFT_LIMB
    }
    case 1:; { /* One limb shift: (64 <= shift < 128)  */

        #define SHIFT_LIMB(x)                                          \
        do {                                                           \
            const uint64_t upper = rop->limb[(x)] & hmask;             \
            rop->limb[(x) + 2] |= (upper >> (64 - shift));             \
            rop->limb[(x) + 1] |= (rop->limb[(x)] << shift);           \
            rop->limb[(x)] = 0;                                        \
        } while (0)

        overflow = rop->limb[3] || rop->limb[2] & hmask;
        rop->limb[3] = rop->limb[2] << shift;
        rop->limb[2] = 0;

        SHIFT_LIMB(1);
        SHIFT_LIMB(0);
        break;

        #undef SHIFT_LIMB
    }
    case 2:; { /* Two limb shift: (128 <= shift < 192) */

        overflow = rop->limb[3] || rop->limb[2] || rop->limb[1] & hmask;
        rop->limb[2] = 0;

        rop->limb[3] = rop->limb[1] << shift;
        rop->limb[3] |= ((rop->limb[0] & hmask) >> (64 - shift));
        rop->limb[2] |= rop->limb[0] << shift;
        rop->limb[1] = rop->limb[0] = 0;
        break;
    }
    case 3:; { /* Three limb shift: (192 <= shift < 256) */

        overflow = rop->limb[3] || rop->limb[2] || rop->limb[1] ||
                        rop->limb[0] & hmask;

        rop->limb[3] = rop->limb[0] << shift;
        rop->limb[2] = rop->limb[1] = rop->limb[0] = 0;
        break;
    }
    default: /* Unreached */
        return -1;
    }

    return overflow;
}

/**
 * Shift the entire 256 memory block right by the specified shift. Any shift
 * value > 255 is first truncated before being applied.
 *
 * Return true if underflow occured during shift, else false.
 */
bool mem256_rshift(mem256_t *rop, int shift)
{
    bool overflow = false;
    const int shift_type = (shift & 255) >> 6;

    /* Normalize shift into the region of a single limb */
    shift &= 63;

    /* Upper and lower region masks are shared between different cases */
    const uint64_t lmask = (1ULL << shift) - 1;

    switch (shift_type) {
    case 0:; { /* Less than one limb shift: (0 <= shift < 64) */

        #define SHIFT_LIMB(x)                                          \
        do {                                                           \
            const uint64_t lower = rop->limb[(x)] & lmask;             \
            rop->limb[(x) - 1] |= lower << (64 - shift);               \
            rop->limb[(x)] >>= shift;                                  \
        } while (0)

        overflow = rop->limb[0] & lmask;
        rop->limb[0] >>= shift;

        /* This could likely be vectorized */
        SHIFT_LIMB(1);
        SHIFT_LIMB(2);
        SHIFT_LIMB(3);
        break;

        #undef SHIFT_LIMB
    }
    case 1:; { /* One limb shift: (64 <= shift < 128)  */

        #define SHIFT_LIMB(x)                                          \
        do {                                                           \
            const uint64_t lower = rop->limb[(x)] & lmask;             \
            rop->limb[(x) - 2] |= lower << (64 - shift);               \
            rop->limb[(x) - 1] |= rop->limb[(x)] >> shift;             \
            rop->limb[(x)] = 0;                                        \
        } while (0)

        overflow = rop->limb[0] || rop->limb[1] & lmask;
        rop->limb[0] = rop->limb[1] >> shift;
        rop->limb[1] = 0;

        SHIFT_LIMB(2);
        SHIFT_LIMB(3);
        break;

        #undef SHIFT_LIMB
    }
    case 2:; { /* Two limb shift: (128 <= shift < 192) */

        overflow = rop->limb[0] || rop->limb[1] || rop->limb[2] & lmask;
        rop->limb[1] = 0;

        rop->limb[0] = rop->limb[2] >> shift;
        rop->limb[0] |= (rop->limb[3] & lmask) << (64 - shift);
        rop->limb[1] |= rop->limb[3] >> shift;
        rop->limb[2] = rop->limb[3] = 0;
        break;
    }
    case 3:; { /* Three limb shift: (192 <= shift < 256) */

        overflow = rop->limb[0] || rop->limb[1] || rop->limb[2] ||
                        rop->limb[3] & lmask;

        rop->limb[0] = rop->limb[3] >> shift;
        rop->limb[1] = rop->limb[2] = rop->limb[3] = 0;
        break;
    }
    default: /* Unreached */
        return -1;
    }

    return overflow;
}

/**
 * A general shift function which translates negative shifts to rshifts */
bool mem256_bshift(mem256_t *rop, int index)
{
    return index > 0 ? mem256_lshift(rop, index) : mem256_rshift(rop, -index);
}

/**
 * Return non-zero if bit at index is non-zero, else zero.
 *
 * Without an explicit return type of bool, this produces odd results. If
 * the return type must be changed to int, then the whole expression should
 * be cast explicitly via '!!'.
 */
bool mem256_get(mem256_t *rop, int index)
{
    return rop->limb[index >> 6] & (1ull << (index & 63));
}

/**
 * Set the bit at index 'index' to 1.
 */
void mem256_set(mem256_t *rop, int index)
{
    rop->limb[index >> 6] |= (1ull << (index & 63));
}

/* Return the number of set bits over the entire memory region. */
int mem256_popcnt(mem256_t *rop)
{
    return mem256_64popcnt(rop->limb[3]) + mem256_64popcnt(rop->limb[2])
             + mem256_64popcnt(rop->limb[1]) + mem256_64popcnt(rop->limb[0]);
}

/* Return the index of the highest bit set in the memory region */
int mem256_highbit(mem256_t *rop)
{
    for (int i = 3; i >= 0; --i) {
        if (rop->limb[i])
            return mem256_64highbit(rop->limb[i]) + (i * 64);
    }

    return 0;
}

/* Fill a range of the regions with 1 from [start, end).
 * This does not zero the memory beforehand.
 *
 * End should be greater than start, and both < 256.
 * */
void mem256_fillones(mem256_t *rop, int start, int end)
{
    int amount = end - start;
    int i = 0;

    while (amount > 0) {
        rop->limb[i++] = amount > 64 ? ~(0ull) : (1ull << (amount & 63)) - 1;
        amount -= 64;
    }

    mem256_bshift(rop, start);
}

/* Zero a memory block */
void mem256_zero(mem256_t *rop)
{
    rop->limb[0] = rop->limb[1] = rop->limb[2] = rop->limb[3] = 0;
}

/* Negate a memory block */
void mem256_negate(mem256_t *rop)
{
    rop->limb[3] = ~rop->limb[3];
    rop->limb[2] = ~rop->limb[2];
    rop->limb[1] = ~rop->limb[1];
    rop->limb[0] = ~rop->limb[0];
}

/* Store the bitwise-or result of op1 and op2 in rop */
void mem256_ior(mem256_t *restrict rop, mem256_t *restrict op)
{
    rop->limb[3] |= op->limb[3];
    rop->limb[2] |= op->limb[2];
    rop->limb[1] |= op->limb[1];
    rop->limb[0] |= op->limb[0];
}

/* Store the bitwise-and result of op1 and op2 in rop */
void mem256_and(mem256_t *restrict rop, mem256_t *restrict op)
{
    rop->limb[3] &= op->limb[3];
    rop->limb[2] &= op->limb[2];
    rop->limb[1] &= op->limb[1];
    rop->limb[0] &= op->limb[0];
}

/* Store the bitwise-xor result of op1 and op2 in rop */
void mem256_xor(mem256_t *restrict rop, mem256_t *restrict op)
{
    rop->limb[3] ^= op->limb[3];
    rop->limb[2] ^= op->limb[2];
    rop->limb[1] ^= op->limb[1];
    rop->limb[0] ^= op->limb[0];
}
