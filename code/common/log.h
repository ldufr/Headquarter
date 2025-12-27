#ifndef COMMON_LOG_H
#define COMMON_LOG_H

enum {
    LOG_ERROR = 1,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_TRACE,
    
    LOG_LEVEL_COUNT,
};

int  log_init(void);
void log_cleanup(void);
void log_set_level(unsigned int level);
int  log_set_file_output(const char *log_path);

int log_msg(unsigned int level, const char *format, ...);
int log_vmsg(unsigned int level, const char *format, va_list ap);

int log_info(const char *fmt, ...);
int log_debug(const char *fmt, ...);
int log_error(const char *fmt, ...);
int log_trace(const char *fmt, ...);
int log_warn(const char *fmt, ...);

int log_vinfo(const char *format, va_list ap);
int log_vdebug(const char *format, va_list ap);
int log_verror(const char *format, va_list ap);
int log_vtrace(const char *format, va_list ap);
int log_vwarn(const char *format, va_list ap);

#endif // COMMON_LOG_H
