#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "log.h"
#include "process.h"
#include "thread.h"

#define LOG_DISABLE  0
#define LOG_MSG_SIZE (64 * 1024)

static unsigned int log_print_level;
static thread_mutex_t log_mutex;
static _Thread_local char log_buffer[LOG_MSG_SIZE];
static FILE *log_file;

static const char *
log_print_level_s(unsigned int level)
{
    switch (level) {
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

int log_init(void)
{
    int err;
    log_print_level = LOG_INFO;
    if ((err = thread_mutex_init(&log_mutex)) != 0) {
        return err;
    }
    return 0;
}

void log_cleanup(void)
{
    // Disable the logs and ensure nobody else is logging.
    log_print_level = LOG_DISABLE;
    if (thread_mutex_lock(&log_mutex) == 0) {
        (void) thread_mutex_unlock(&log_mutex);
        (void) thread_mutex_destroy(&log_mutex);
    }

    if (log_file) {
        (void) fclose(log_file);
        log_file = NULL;
    }
}

void log_set_level(unsigned int level)
{
    log_print_level = level;
}

int log_set_file_output(const char *log_path)
{
    if (log_path && *log_path != 0) {
        thread_mutex_lock(&log_mutex);
        if (log_file)
            fclose(log_file);
        log_file = fopen(log_path, "a");
        thread_mutex_unlock(&log_mutex);

        if (log_file == NULL) {
            log_error("Failed to open the file '%s'", log_path);
            return 1;
        }
        log_info("Logging to '%s'", log_path);
    }
    return 0;
}

static size_t log_time(char *buffer, size_t size)
{
    struct tm ts;
    time_localtime(time(NULL), &ts);
    return strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &ts);
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

    if (log_print_level < level) {
        return 0;
    }

    char timestamp[64];
    if (log_time(timestamp, sizeof(timestamp)) == 0 ) {
        abort();
    }

    nr_chars = snprintf(log_buffer, sizeof(log_buffer), "[%s] %7s: ", timestamp, log_print_level_s(level));
    if (!(0 < nr_chars && nr_chars < sizeof(log_buffer))) {
        abort();
    }

    char *buffer = log_buffer + nr_chars;
    size_t size = sizeof(log_buffer) - nr_chars;
    nr_chars = vsnprintf(buffer, size, format, ap);

    // We may not be able to write the all string (e.g., too long), but the
    // buffer will still be null-terminated.
    if (nr_chars < 0) {
        nr_chars = snprintf(buffer, size, "failed to write log with format '%s'", format);
        if (nr_chars < 0) {
            abort();
        }
    }

    thread_mutex_lock(&log_mutex);
    fputs(buffer, stderr);
    fputc('\n', stderr);
    if (log_file) {
        fputs(buffer, log_file);
        fputc('\n', log_file);
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
