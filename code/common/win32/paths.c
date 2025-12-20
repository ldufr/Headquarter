#ifdef OS_DLFUNC_C
#error "dlfunc.c included more than once"
#endif
#define OS_DLFUNC_C
#include "../paths.h"

int open_dll(const char *filename, void **handle)
{
    if (filename == NULL) {
        *handle = GetModuleHandle(NULL);
        return 0;
    }
    if ((*handle = LoadLibraryA(filename)) != NULL) {
        return 0;
    }
    fprintf(stderr, "LoadLibraryA Error: %d\n", GetLastError());
    return 1;
}

int close_dll(void *handle)
{
    if (FreeLibrary((HMODULE)handle))
        return 0;
    else
        return 1;
}

int get_dll_symbol(void* handle, const char *symbol, void **out)
{
    if ((*out = (void *) GetProcAddress((HMODULE)handle, symbol)) != NULL) {
        return 0;
    }
    fprintf(stderr, "GetProcAddress Error: %d findng symbol %s\n", GetLastError(), symbol);
    return 1;
}

int get_executable_path(char* buffer, size_t capacity, size_t *length)
{
    DWORD dwLength = max_size_t(capacity, MAXDWORD);
    DWORD result = GetModuleFileName(NULL, buffer, dwLength);
    if (result == 0 || result == dwLength) {
        return 1;
    } else {
        *length = (size_t)result;
        return 0;
    }
}

int get_executable_dir(char *buffer, size_t capacity, size_t *length)
{
    int err;

    size_t len;
    if ((err = get_executable_path(buffer, capacity, &len)) != 0) {
        return 0;
    }

    size_t idx;
    for (idx = len - 1; idx < len; --idx) {
        if (buffer[idx] == '\\' || buffer[idx] == '/') {
            buffer[idx] = 0;
            *length = idx;
            return 0;
        }
    }

    return 1;
}
