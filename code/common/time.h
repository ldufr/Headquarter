#ifndef COMMON_TIME_H
#define COMMON_TIME_H

#include <time.h>
#include <stdint.h>

typedef uint64_t msec_t;

void time_init(void);

int get_wall_clock_res(struct timespec *ts);
int get_wall_clock_time(struct timespec *ts);
uint64_t get_time_since_epoch(void);

void time_sleep_us(unsigned int us);
void time_sleep_ns(unsigned int ns);
void time_sleep_ms(unsigned int ms);
void time_sleep_sec(unsigned int sec);

uint64_t time_get_ms(void);

long time_diff_nsec(struct timespec *end, struct timespec *beginning);
bool time_localtime(time_t timep, struct tm* result);
uint64_t time_get_epoch_sec(void);
#endif // COMMON_TIME_H
