#pragma once

#ifdef _MSC_VER
#define UNUSED_FUNC
#else
#define UNUSED_FUNC __attribute__((unused))
#endif

static inline size_t max_size_t(size_t left, size_t right) UNUSED_FUNC;
static inline size_t max_size_t(size_t left, size_t right)
{
    return left < right ? right : left;
}

static inline size_t min_size_t(size_t left, size_t right) UNUSED_FUNC;
static inline size_t min_size_t(size_t left, size_t right)
{
    return left < right ? left : right;
}
