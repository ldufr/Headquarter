#ifndef COMMON_PATHS_H
#define COMMON_PATHS_H

#ifdef _WIN32
#define PATH_SEP '\\'
#define COMMON_PATH_MAX 260
#else
#define PATH_SEP '/'
#define COMMON_PATH_MAX 4096
#endif

int open_dll(const char *filename, void **handle);
int close_dll(void *handle);
int get_dll_symbol(void* handle, const char *symbol, void **out);

int get_executable_path(char* buffer, size_t length, size_t *ret);
int get_executable_dir(char* buffer, size_t length, size_t *ret);

#endif // COMMON_PATHS_H
