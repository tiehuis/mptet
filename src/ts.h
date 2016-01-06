/**
 * Define cross-platform timekeeping functions.
 *
 * We allow for a number of different timekeeping functions with varying
 * degrees of accuracy depending on the target system.
 *
 * Timeshares (ts) are represented as integer units and should indicate a
 * number of small-denomination time periods (i.e. nanoseconds/microseconds).
 */

/**
 * Specifies how many timeshares occur per second. This is used internally and
 * can be used to calculate the sleep threshold based on the current FPS.
 *
#define TS_IN_A_SECOND
*/

/**
 * Specifies the scaling factor required to adjust the timeshares to the sleep
 * function being used. This likely will always be greater than 1.
 *
#define TS_SCALE_FACTOR
*/

#include <unistd.h>

/* Set appropriate defines for what functions we use */
#if _POSIX_C_SOURCE >= 199309L
#   define TS_HAVE_NANOSLEEP
#endif

#if defined(_POSIX_TIMERS)
#   define TS_HAVE_CLOCK_GETTIME
#   if !defined(_POSIX_MONOTONIC_CLOCK)
#       error "_POSIX_MONOTONIC_CLOCK not defined when expected"
#   endif
#endif

/* TODO: Fix this unlikely combination. Related to the scale factor < 1 */
#if !defined(TS_HAVE_CLOCK_GETTIME) && defined(TS_HAVE_NANOSLEEP)
#error "Invalid time function combination"
#endif

/* Determine clock function and accuracy */
#if defined(TS_HAVE_CLOCK_GETTIME)
    /* clock_gettime */
#   include <time.h>
#   define TS_IN_A_SECOND 1000000000ull
#   if defined(HAVE_CLOCK_MONOTONIC_RAW)
#       define TS_CLOCK CLOCK_MONOTONIC_RAW
#   else
#       define TS_CLOCK CLOCK_MONOTONIC
#endif
#else
    /* gettimeofday */
#   include <sys/time.h>
#   define TS_IN_A_SECOND 1000000ull
#endif

/* Determine sleep function and scale factor */
#if defined(TS_HAVE_NANOSLEEP)
    /* nanosleep */
#   include <time.h>
#   define TS_SCALE_FACTOR ((double) TS_IN_A_SECOND / 1000000000ull)
#else
    /* usleep */
#   include <unistd.h>
#   define TS_SCALE_FACTOR ((double) TS_IN_A_SECOND / 1000000ull)
#endif

/**
 * Return a number of ts as a uint64_t
 */
int64_t ts_get_current_time(void)
{
#if defined(TS_HAVE_CLOCK_GETTIME)
    struct timespec ts;
    clock_gettime(TS_CLOCK, &ts);
    return ((uint64_t) ts.tv_sec * TS_IN_A_SECOND + ts.tv_nsec);
#else
    struct timeval ts;
    gettimeofday(&ts, NULL);
    return ((uint64_t) ts.tv_sec * TS_IN_A_SECOND + ts.tv_usec);
#endif
}

void ts_sleep(uint64_t no_of_ts)
{
#if defined(TS_HAVE_NANOSLEEP)
    no_of_ts = no_of_ts / TS_SCALE_FACTOR;
    struct timespec ts = { no_of_ts / TS_IN_A_SECOND, no_of_ts % TS_IN_A_SECOND };
    nanosleep(&ts, NULL);
#else
    usleep(no_of_ts / TS_SCALE_FACTOR);
#endif
}
