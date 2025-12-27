#ifdef OS_TIME_C
#error "time.c included more than once"
#endif
#define OS_TIME_C

#include "../time.h"

static bool time_initialized;
static struct timespec time_res;

void time_init(void)
{
    get_wall_clock_res(&time_res);
    time_initialized = true;
}

int get_wall_clock_res(struct timespec *ts)
{
    LARGE_INTEGER result;
    if (QueryPerformanceFrequency(&result) == 0)
        return 1;
    ts->tv_sec = 0;
    ts->tv_nsec = (long)(1000000000ll / result.QuadPart);
    return 0;
}

int get_wall_clock_time(struct timespec *ts)
{
    assert(time_initialized);
    LARGE_INTEGER result;
    if (QueryPerformanceCounter(&result) == 0)
        return 1;
    uint64_t total = result.QuadPart * time_res.tv_nsec;
    ts->tv_sec = total / 1000000000ull;
    ts->tv_nsec = total % 1000000000ull;
    return 0;
}

void time_sleep_ns(unsigned int ns)
{
    unsigned int sleep_time = ns / 1000000;
    Sleep(sleep_time);
}

void time_sleep_us(unsigned int us)
{
    unsigned int sleep_time = us / 1000;
    Sleep(sleep_time);
}

void time_sleep_ms(unsigned int ms)
{
    Sleep(ms);
}

void time_sleep_sec(unsigned int sec)
{
    Sleep(sec * 1000);
}

uint64_t time_get_ms(void)
{
    return GetTickCount64();
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
    errno_t err;
    if ((err = localtime_s(result, &timep)) != 0)
        return false;
    return true;
}

uint64_t time_get_epoch_sec(void)
{
    STATIC_ASSERT(sizeof(__time64_t) == sizeof(uint64_t));
    return _time64(NULL);
}
