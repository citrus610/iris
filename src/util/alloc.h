#pragma once

#include <iostream>
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cassert>

#if defined(__linux__)
#include <sys/mman.h>
#endif

using usize = size_t;

inline void* malloc_aligned(usize alignment, usize size)
{
    void* ptr;

#if defined(__MINGW32__)
    ptr = _aligned_malloc(size, alignment);
#elif defined (__GNUC__)
    ptr = std::aligned_alloc(alignment, size);
#else
#error "Unsupported complier!"
#endif

#if defined(__linux__)
    madvise(ptr, size, MADV_HUGEPAGE);
#endif 

    return ptr;
};

inline void free_aligned(void* ptr)
{
#if defined(__MINGW32__)
    _aligned_free(ptr);
#elif defined (__GNUC__)
    std::free(ptr);
#else
#error "Unsupported complier!"
#endif
};