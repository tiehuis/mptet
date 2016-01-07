#pragma once

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
static inline int mem256_64popcnt(uint64_t x)
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
static inline int mem256_64highbit(uint64_t x)
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
bool mem256_lshift(mem256_t *rop, int shift);

/**
 * Shift the entire 256 memory block right by the specified shift. Any shift
 * value > 255 is first truncated before being applied.
 *
 * Return true if underflow occured during shift, else false.
 */
bool mem256_rshift(mem256_t *rop, int shift);

/**
 * A general shift function which translates negative shifts to rshifts */
bool mem256_bshift(mem256_t *rop, int index);

/**
 * Return non-zero if bit at index is non-zero, else zero.
 *
 * Without an explicit return type of bool, this produces odd results. If
 * the return type must be changed to int, then the whole expression should
 * be cast explicitly via '!!'.
 */
bool mem256_get(mem256_t *rop, int index);

/**
 * Set the bit at index 'index' to 1.
 */
void mem256_set(mem256_t *rop, int index);

/* Return the number of set bits over the entire memory region. */
int mem256_popcnt(mem256_t *rop);

/* Return the index of the highest bit set in the memory region */
int mem256_highbit(mem256_t *rop);

/* Fill a range of the regions with 1 from [start, end).
 * This does not zero the memory beforehand.
 *
 * End should be greater than start, and both < 256.
 * */
void mem256_fillones(mem256_t *rop, int start, int end);

/* Zero a memory block */
void mem256_zero(mem256_t *rop);

/* Negate a memory block */
void mem256_negate(mem256_t *rop);

/* Store the bitwise-or result of op1 and op2 in rop */
void mem256_ior(mem256_t *restrict rop, mem256_t *restrict op);

/* Store the bitwise-and result of op1 and op2 in rop */
void mem256_and(mem256_t *restrict rop, mem256_t *restrict op);

/* Store the bitwise-xor result of op1 and op2 in rop */
void mem256_xor(mem256_t *restrict rop, mem256_t *restrict op);
