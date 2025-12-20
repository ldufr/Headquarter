#ifdef OS_DLFUNC_C
#error "dlfunc.c included more than once"
#endif
#define OS_DLFUNC_C

#include "../paths.h"

int open_dll(const char* filename, void** handle)
{
    if ((*handle = dlopen(filename, RTLD_LAZY)) == NULL) {
        printf("dlopen Error: %s\n", dlerror());
        return 1;
    }
    return 0;
}

int close_dll(void* handle) {
    return dlclose(handle);
}
int get_dll_symbol(void* handle, const char* symbol, void** out) {
    if ((*out = dlsym(handle, symbol)) == NULL) {
        printf("dlsym Error: %s\n", dlerror());
        return 1;
    }
    return 0;
}

int get_executable_path(char* buffer, size_t capacity, size_t* length)
{
    int err;
    if ((err = readlink("/proc/self/exe", buffer, capacity)) < 0)
        return errno;
    size_t bytes = min_size_t((size_t)err, capacity - 1);
    buffer[bytes] = '\0';
    *length = bytes;
    return 0;
}

int get_executable_dir(char* buffer, size_t capacity, size_t* length)
{
    size_t len;
    if (get_executable_path(buffer, capacity, &len) != 0)
        return 1;
    char* p = strrchr(buffer, '/');
    if (p) p[0] = 0;
    *length = strlen(buffer);
    return 0;
}
