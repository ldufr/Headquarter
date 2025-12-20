#ifdef OS_THREAD_C
#error "thread.c included more than once"
#endif
#define OS_THREAD_C

#include "../log.h"
#include "../thread.h"

static void *start_routine(void *param)
{
    struct thread *thread = param;
    int ret = thread->start(thread->param);
    return (void *)((intptr_t)ret);
}

int thread_create(struct thread *thread, thread_start_t start, void *param)
{
    int ret;
    pthread_attr_t attr;

    thread->start = start;
    thread->param = param;

    if ((ret = pthread_attr_init(&attr)) != 0) {
        log_error("pthread_attr_init failed: %d", ret);
        return 1;
    }

    if ((ret = pthread_create(&thread->handle, &attr, start_routine, thread)) != 0) {
        log_error("pthread_create failed: %d", ret);
        return 1;
    }

    pthread_attr_destroy(&attr);
    return 0;
}

_Noreturn void thread_exit(int retval)
{
    pthread_exit((void *)((intptr_t)retval));
}

struct thread thread_self(void)
{
    struct thread thread;
    thread.handle = pthread_self();
    return thread;
}

int thread_detach(struct thread *thread)
{
    int retval = pthread_detach(thread->handle);
    if (retval != 0) {
        log_error("pthread_detach failed: %d", retval);
        return retval;
    } else {
        thread->handle = 0;
        return 0;
    }
}

int thread_join(struct thread *thread, int *retval)
{
    void *rv;
    int res = pthread_join(thread->handle, &rv);
    if (res != 0) {
        log_error("pthread_join failed: %d", res);
        return res;
    } else {
        thread->handle = 0;
        *retval = (int)((intptr_t)rv);
        return 0;
    }
}

int thread_sleep(struct thread *thread, const struct timespec *ts)
{
    struct timespec rem;
    int retval = nanosleep(ts, &rem);
    // @Cleanup:
    // Deal with the retval + logging?
    return retval;
}

void thread_yield(void)
{
}

union mutex_impl {
    thread_mutex_t  mtx;
    pthread_mutex_t pmtx;
};
static_assert(sizeof(pthread_mutex_t) <= sizeof(thread_mutex_t), "thread_mutex implementation too small");

int thread_mutex_init(thread_mutex_t *mutex)
{
    union mutex_impl *mtx = (union mutex_impl *)mutex;
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
    int error = pthread_mutex_init(&mtx->pmtx, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);
    return error;
}

int thread_mutex_destroy(thread_mutex_t *mutex)
{
    union mutex_impl *mtx = (union mutex_impl *)mutex;
    return pthread_mutex_destroy(&mtx->pmtx);
}

int thread_mutex_lock(thread_mutex_t *mutex)
{
    union mutex_impl *mtx = (union mutex_impl *)mutex;
    return pthread_mutex_lock(&mtx->pmtx);
}

int thread_mutex_trylock(thread_mutex_t *mutex)
{
    union mutex_impl *mtx = (union mutex_impl *)mutex;
    return pthread_mutex_trylock(&mtx->pmtx);
}

int thread_mutex_unlock(thread_mutex_t *mutex)
{
    union mutex_impl *mtx = (union mutex_impl *)mutex;
    return pthread_mutex_unlock(&mtx->pmtx);
}
