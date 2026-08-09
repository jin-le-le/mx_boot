/**
 * @file bitops.h
 * @brief Bit manipulation utilities
 */

#ifndef BITOPS_H
#define BITOPS_H

#include <stdint.h>
#include <string.h>

#define __GET_VALUE_TEMPATE(_nick, _type) \
static inline _type get_##_nick(uint8_t *ptr) { \
    return *(_type *)ptr; \
}

#define __GET_VALUE_INC_TEMPATE(_nick, _type) \
static inline _type get_##_nick##_inc(uint8_t **ptr) { \
    uint8_t *p = *ptr; \
    *ptr += sizeof(_type); \
    return *(_type *)p; \
}

#define __PUT_VALUE_TEMPATE(_nick, _type) \
static inline void put_##_nick(uint8_t *ptr, _type data) { \
    *(_type *)ptr = data; \
}

#define __PUT_VALUE_INC_TEMPATE(_nick, _type) \
static inline void put_##_nick##_inc(uint8_t **ptr, _type data) { \
    uint8_t *p = *ptr; \
    *ptr += sizeof(_type); \
    put_##_nick(p, data); \
}

#define __VALUE_OPERATION_TEMPATE(_micro) \
    _micro(i8, int8_t) \
    _micro(u8, uint8_t) \
    _micro(i16, int16_t) \
    _micro(u16, uint16_t) \
    _micro(i32, int32_t) \
    _micro(u32, uint32_t) \
    _micro(i64, int64_t) \
    _micro(u64, uint64_t)

__VALUE_OPERATION_TEMPATE(__GET_VALUE_TEMPATE)
__VALUE_OPERATION_TEMPATE(__GET_VALUE_INC_TEMPATE)
__VALUE_OPERATION_TEMPATE(__PUT_VALUE_TEMPATE)
__VALUE_OPERATION_TEMPATE(__PUT_VALUE_INC_TEMPATE)

#define __GET_VALUE_INC_CONST_TEMPLATE(_nick, _type) \
static inline _type get_##_nick##_inc_const(const uint8_t **ptr) { \
    const uint8_t *p = *ptr; \
    *ptr += sizeof(_type); \
    return *(_type *)p; \
}

#define __VALUE_OPERATION_CONST_TEMPLATE(_micro) \
    _micro(i8, int8_t) \
    _micro(u8, uint8_t) \
    _micro(i16, int16_t) \
    _micro(u16, uint16_t) \
    _micro(i32, int32_t) \
    _micro(u32, uint32_t)

__VALUE_OPERATION_CONST_TEMPLATE(__GET_VALUE_INC_CONST_TEMPLATE)

static inline void get_bytes(const uint8_t *ptr, uint8_t *data, uint32_t length)
{
    memcpy(data, ptr, length);
}

static inline void get_bytes_inc(const uint8_t **ptr, uint8_t *data, uint32_t length)
{
    memcpy(data, *ptr, length);
    *ptr += length;
}

static inline void put_bytes(uint8_t *ptr, const uint8_t *data, uint32_t length)
{
    if (data == NULL || length == 0) return;
    memcpy(ptr, data, length);
}

static inline void put_bytes_inc(uint8_t **ptr, const uint8_t *data, uint32_t length)
{
    if (data == NULL || length == 0) return;
    memcpy(*ptr, data, length);
    *ptr += length;
}

#endif /* BITOPS_H */
