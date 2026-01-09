#pragma once

static inline size_t max_size_t(size_t left, size_t right)
{
    return left < right ? right : left;
}

static inline size_t min_size_t(size_t left, size_t right)
{
    return left < right ? left : right;
}
