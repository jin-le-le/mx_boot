/**
 * @file ringbuffer.h
 * @brief Ring buffer implementation
 */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *buffer;
    uint32_t size;
    uint32_t head;
    uint32_t tail;
    bool full;
} rb_t;

rb_t rb_new(uint8_t *buffer, uint32_t size);
bool rb_put(rb_t *rb, uint8_t byte);
bool rb_get(rb_t *rb, uint8_t *byte);
bool rb_empty(rb_t *rb);
bool rb_full(rb_t *rb);
uint32_t rb_used(rb_t *rb);
uint32_t rb_puts(rb_t *rb, const uint8_t *data, uint32_t len);
uint32_t rb_gets(rb_t *rb, uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* RINGBUFFER_H */
