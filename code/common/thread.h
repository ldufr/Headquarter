#ifndef COMMON_THREAD_H
#define COMMON_THREAD_H

#include <time.h>
#include <stdint.h>

#ifndef _WIN32
#include <pthread.h>
#endif

#include "align.h"
#include "noreturn.h"

#if !defined(__clang__) && defined(_MSC_VER)
# define _Thread_local  __declspec(thread)
#endif
#if defined(__GNUC__)
# define _Thread_local  __thread
#endif
#define thread_local _Thread_local

#ifdef _WIN32
# ifdef _WIN64
#  define _mtx_internal_imp_size         40
#  define _mtx_internal_imp_alignment    8
# else
#  define _mtx_internal_imp_size         24
#  define _mtx_internal_imp_alignment    4
# endif // _WIN64
#else // _WIN32
# define _mtx_internal_imp_size         80
# define _mtx_internal_imp_alignment    8
#endif

typedef int (*thread_start_t)(void *);
struct thread {
#ifdef _WIN32
    void* handle;
#else
    pthread_t handle;  // Use pthread_t directly on Linux
#endif
    thread_start_t start;
    void *param;
};

int thread_create(struct thread *thread, thread_start_t start, void *param);
_Noreturn void thread_exit(int retval);
struct thread thread_self(void);

int thread_detach(struct thread *thread);
int thread_join(struct thread *thread, int *retval);
int thread_sleep(struct thread *thread, const struct timespec *ts);
void thread_yield(void);

typedef struct thread_mutex {
    alignas(_mtx_internal_imp_alignment)
        unsigned char storage[_mtx_internal_imp_size];
} thread_mutex_t;

int thread_mutex_init(thread_mutex_t *mutex);
int thread_mutex_destroy(thread_mutex_t *mutex);
int thread_mutex_lock(thread_mutex_t *mutex);
int thread_mutex_trylock(thread_mutex_t *mutex);
int thread_mutex_unlock(thread_mutex_t *mutex);

typedef struct thread_event {
    void* handle;
} thread_event_t;

int thread_event_init(thread_event_t* event);
int thread_event_destroy(thread_event_t* event);
int thread_event_signal(thread_event_t* event);
int thread_event_reset(thread_event_t* event);
int thread_event_wait(thread_event_t* event);
int thread_event_timedwait(thread_event_t* event, uint32_t ms);

#endif // COMMON_THREAD_H
