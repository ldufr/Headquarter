#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "log.h"
#include "process.h"
#include "thread.h"


#define LOG_MSG_SIZE    (1024)
#define LOG_BUFFER_SIZE (16 * LOG_MSG_SIZE)

static unsigned int     log_print_level;

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static thread_event_t   log_event;
static thread_mutex_t   log_mutex;

static size_t           log_wpos = 0;
static char             log_buffer[LOG_BUFFER_SIZE];

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

static FILE            *log_file;

static const char *
log_print_level_s(unsigned int level)
{
    switch (level) {
        case LOG_CRITICAL:
            return "critical";
        case LOG_ERROR:
            return "error";
        case LOG_WARN:
            return "warn";
        case LOG_INFO:
            return "info";
        case LOG_DEBUG:
            return "debug";
        case LOG_TRACE:
            return "trace";
        default:
            abort();
    }
}

void log_init(const char *log_file_name)
{
    int err;
    log_print_level = LOG_INFO;

    int error = thread_mutex_init(&log_mutex);
    if (error) {
        assert(!"log_init: thread_mutex_init");
        return;
    }

    // client specify the file path
    char file_path[1128];
    if (log_file_name && *log_file_name != 0) {
        snprintf(file_path, sizeof(file_path), "%s", log_file_name);
    } else {
        size_t length = 0;
        char dir_path[1024];
        if ((err = get_executable_dir(dir_path, sizeof(dir_path), &length)) != 0) {
            fprintf(stderr, "Failed to get the 'dlldir'\n");
            return;
        }
        snprintf(file_path, sizeof(file_path), "%s/logs/%s", dir_path, log_file_name);
    }

    if ((log_file = fopen(file_path, "a")) == NULL) {
        fprintf(stderr, "Failed to open the file '%s'\n", file_path);
        return;
    }

    printf("Logging to %s\n",file_path);
}

void log_set_level(unsigned int level)
{
    log_print_level = level;
}

static int log_time(char *buffer, size_t size)
{
    time_t t = time(NULL);
    struct tm ts;
    time_localtime(&t, &ts);
    return (int)strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &ts);
}

int log_vmsg(unsigned int level, const char *format, va_list ap);
int log_msg(unsigned int level, const char *format, ...)
{
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = log_vmsg(level, format, ap);
    va_end(ap);

    return ret;
}

int log_vmsg(unsigned int level, const char *format, va_list ap)
{
    int nr_chars;
    char buffer[LOG_MSG_SIZE];

    if (!log_file || level > log_print_level) {
        return 0;
    }

    nr_chars = vsnprintf(buffer, sizeof(buffer), format, ap);
    if ((unsigned int)nr_chars >= sizeof(buffer)) {
        log_msg(level, "log: message too long");
        return 0;
    }

    thread_mutex_lock(&log_mutex);
    {
        char timestamp[64];
        log_time(timestamp, sizeof(timestamp));

        nr_chars = fprintf(stderr, "[%s] %7s: %s\n", timestamp, log_print_level_s(level), buffer);
        fprintf(log_file, "[%s] %7s: %s\n", timestamp, log_print_level_s(level), buffer);
        fflush(log_file);
    }
    thread_mutex_unlock(&log_mutex);

    return nr_chars;
}

int log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = log_vinfo(fmt, ap);
    va_end(ap);
    return ret;
}

int log_debug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = log_vdebug(fmt, ap);
    va_end(ap);
    return ret;
}

int log_trace(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = log_vtrace(fmt, ap);
    va_end(ap);
    return ret;
}

int log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = log_verror(fmt, ap);
    va_end(ap);
    return ret;
}

int log_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = log_vwarn(fmt, ap);
    va_end(ap);
    return ret;
}

int log_vinfo(const char *format, va_list ap)
{
    return log_vmsg(LOG_INFO, format, ap);
}

int log_vdebug(const char *format, va_list ap)
{
    return log_vmsg(LOG_DEBUG, format, ap);
}

int log_vtrace(const char *format, va_list ap)
{
    return log_vmsg(LOG_TRACE, format, ap);
}

int log_verror(const char *format, va_list ap)
{
    return log_vmsg(LOG_ERROR, format, ap);
}

int log_vwarn(const char *format, va_list ap)
{
    return log_vmsg(LOG_WARN, format, ap);
}
