/**
 * @file ringbuffer.c
 * @brief Ring buffer implementation
 */

#include "ringbuffer.h"
#include <string.h>

rb_t rb_new(uint8_t *buffer, uint32_t size)
{
    rb_t rb;
    rb.buffer = buffer;
    rb.size = size;
    rb.head = 0;
    rb.tail = 0;
    rb.full = false;
    return rb;
}

bool rb_put(rb_t *rb, uint8_t byte)
{
    if (rb->full) {
        return false;
    }
    rb->buffer[rb->head] = byte;
    rb->head = (rb->head + 1) % rb->size;
    if (rb->head == rb->tail) {
        rb->full = true;
    }
    return true;
}

bool rb_get(rb_t *rb, uint8_t *byte)
{
    if (rb_empty(rb)) {
        return false;
    }
    *byte = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    rb->full = false;
    return true;
}

bool rb_empty(rb_t *rb)
{
    return (!rb->full) && (rb->head == rb->tail);
}

bool rb_full(rb_t *rb)
{
    return rb->full;
}

uint32_t rb_used(rb_t *rb)
{
    if (rb->full) {
        return rb->size;
    }
    if (rb->head >= rb->tail) {
        return rb->head - rb->tail;
    }
    return rb->size - (rb->tail - rb->head);
}

uint32_t rb_puts(rb_t *rb, const uint8_t *data, uint32_t len)
{
    uint32_t written = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (!rb_put(rb, data[i])) {
            break;
        }
        written++;
    }
    return written;
}

uint32_t rb_gets(rb_t *rb, uint8_t *data, uint32_t len)
{
    uint32_t read = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (!rb_get(rb, &data[i])) {
            break;
        }
        read++;
    }
    return read;
}
