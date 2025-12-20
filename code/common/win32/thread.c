#ifdef OS_THREAD_C
#error "thread.c included more than once"
#endif
#define OS_THREAD_C

// #include "win32.h"
#include "../log.h"
#include "../thread.h"

static DWORD WINAPI ThreadProc(LPVOID lpParam)
{
    assert(lpParam != NULL);
    struct thread *thread = (struct thread* )lpParam;
    int retval = thread->start(thread->param);
    return (DWORD)retval;
}

int thread_create(struct thread *thread, thread_start_t start, void *param)
{
    thread->start = start;
    thread->param = param;
    thread->handle = CreateThread(NULL, 0, ThreadProc, thread, 0, 0);

    if (!thread->handle) {
        log_error("CreateThread failed (%lu)", GetLastError());
        return -1;
    }

    return 0;
}

_Noreturn void thread_exit(int retval)
{
    ExitThread((DWORD)retval);
}

struct thread thread_self(void)
{
    struct thread thread = {0};
    thread.handle = GetCurrentThread();
    return thread;
}

int thread_detach(struct thread *thread)
{
    CloseHandle(thread->handle);
    thread->handle = 0;
    return 0;
}

int thread_join(struct thread *thread, int *retval)
{
    DWORD reason = WaitForSingleObject(thread->handle, INFINITE);
    if (reason != WAIT_OBJECT_0) {
        log_error("WaitForSingleObject failed {reason: %lu, error: %lu",
            reason, GetLastError());
        return -1;
    }

    DWORD rv;
    if (!GetExitCodeThread(thread->handle, &rv)) {
        log_error("GetExitCodeThread failed (%lu)", GetLastError());
        CloseHandle(thread->handle);
        return -1;
    }

    CloseHandle(thread->handle);
    thread->handle = NULL;

    *retval = (int)rv;
    return 0;
}

int thread_sleep(struct thread *thread, const struct timespec *ts)
{
    UNREFERENCED_PARAMETER(thread);

    DWORD sleep_time_ms;
    sleep_time_ms =  (DWORD)ts->tv_sec * 1000;
    sleep_time_ms += ts->tv_nsec / 1000000;
    Sleep(sleep_time_ms);
    return 0;
}

void thread_yield(void)
{
    SwitchToThread();
}

union mutex_impl {
    thread_mutex_t      mtx;
    CRITICAL_SECTION    sec;
};
static_assert(sizeof(CRITICAL_SECTION) <= sizeof(thread_mutex_t), "thread_mutex implementation too small");

int thread_mutex_init(thread_mutex_t *mutex)
{
    union mutex_impl *mtx = (union mutex_impl *)mutex;
    InitializeCriticalSection(&mtx->sec);
    return 0;
}

int thread_mutex_destroy(thread_mutex_t *mutex)
{
    union mutex_impl *mtx = (union mutex_impl *)mutex;
    DeleteCriticalSection(&mtx->sec);
    return 0;
}

int thread_mutex_lock(thread_mutex_t *mutex)
{
    union mutex_impl *mtx = (union mutex_impl *)mutex;
    EnterCriticalSection(&mtx->sec);
    return 0;
}

int thread_mutex_trylock(thread_mutex_t *mutex)
{
    union mutex_impl *mtx = (union mutex_impl *)mutex;
    BOOL could_enter = TryEnterCriticalSection(&mtx->sec);
    return !could_enter;
}

int thread_mutex_unlock(thread_mutex_t *mutex)
{
    union mutex_impl *mtx = (union mutex_impl *)mutex;
    LeaveCriticalSection(&mtx->sec);
    return 0;
}

int thread_event_init(thread_event_t *event)
{
    event->handle = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!event->handle) {
        log_error("CreateEventW failed (%lu)", GetLastError());
        return -1;
    }
    return 0;
}

int thread_event_destroy(thread_event_t *event)
{
    CloseHandle(event->handle);
    event->handle = NULL;
    return 0;
}

int thread_event_signal(thread_event_t *event)
{
    if (SetEvent(event->handle) != TRUE) {
        log_error("SetEvent failed (%lu)", GetLastError());
        return -1;
    }
    return 0;
}

int thread_event_reset(thread_event_t *event)
{
    ResetEvent(event->handle);
    return 0;
}

int thread_event_wait(thread_event_t *event)
{
    if (WaitForSingleObject(event->handle, INFINITE) == WAIT_FAILED) {
        log_error("WaitForSingleObject failed (%lu)", GetLastError());
        return -1;
    }
    return 0;
}

int thread_event_timedwait(thread_event_t *event, uint32_t ms)
{
    if (WaitForSingleObject(event->handle, ms) == WAIT_FAILED) {
        log_error("WaitForSingleObject failed (%lu)", GetLastError());
        return -1;
    }
    return 0;
}
