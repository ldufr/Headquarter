#ifdef OS_TIME_C
#error "time.c included more than once"
#endif
#define OS_TIME_C

#include "../time.h"

static inline uint64_t nanos_since_boot()
{
    struct timespec t;
    clock_gettime(CLOCK_BOOTTIME, &t);
    return t.tv_sec * 1000000000ULL + t.tv_nsec;
}

static inline double millis_since_boot()
{
    struct timespec t;
    clock_gettime(CLOCK_BOOTTIME, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec * 1e-6;
}

static inline double seconds_since_boot()
{
    struct timespec t;
    clock_gettime(CLOCK_BOOTTIME, &t);
    return (double)t.tv_sec + t.tv_nsec * 1e-9;;
}

static inline uint64_t nanos_since_epoch()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * 1000000000ULL + t.tv_nsec;
}

static inline double seconds_since_epoch()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (double)t.tv_sec + t.tv_nsec * 1e-9;
}

// you probably should use nanos_since_boot instead
static inline uint64_t nanos_monotonic()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000ULL + t.tv_nsec;
}

static inline uint64_t nanos_monotonic_raw()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return t.tv_sec * 1000000000ULL + t.tv_nsec;
}

void time_init(void)
{
}

int get_wall_clock_time(struct timespec *ts)
{
    return clock_gettime(CLOCK_MONOTONIC, ts);
}

void time_sleep_ns(unsigned int ns)
{
    struct timespec ts, rem;
    ts.tv_sec = ns / 1000000000ULL;
    ts.tv_nsec = (long)(ns % 1000000000ULL);
    nanosleep(&ts, &rem);
}

void time_sleep_us(unsigned int us)
{
    time_sleep_ns(us * 1000);
}

void time_sleep_ms(unsigned int ms)
{
    time_sleep_ns(ms * 1000000);
}

void time_sleep_sec(unsigned int sec)
{
    sleep(sec);
}

uint64_t time_get_ms(void)
{
    return (uint64_t)millis_since_boot();
}
uint64_t get_time_since_epoch(void)
{
    return (uint64_t)time(NULL);
}

long time_diff_nsec(struct timespec *end, struct timespec *beginning)
{
    double diffsec = difftime(end->tv_sec, beginning->tv_sec);
    long diffnsec = end->tv_nsec - beginning->tv_nsec;
    return (long)(diffsec * 1000000000) + diffnsec;
}

bool time_localtime(time_t timep, struct tm* result)
{
	if (localtime_r(&timep, result) == NULL)
		return false;
	return true;
}

uint64_t time_get_epoch_sec(void)
{
    STATIC_ASSERT(sizeof(time_t) == sizeof(uint64_t));
    return time(NULL);
}
