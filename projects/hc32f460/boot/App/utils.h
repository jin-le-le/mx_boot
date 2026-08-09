/**
 * @file utils.h
 * @brief Utility macros
 */

#ifndef UTILS_H
#define UTILS_H

#include "bitops.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define offset_of(type, member) ((size_t) &((type *)0)->member)
#define container_of(ptr, type, member) ({          \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offset_of(type, member)); })

#endif /* UTILS_H */
