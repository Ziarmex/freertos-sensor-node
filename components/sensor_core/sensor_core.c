/* SPDX-License-Identifier: MIT */
#include "sensor_core.h"

#include <stddef.h>

void ringbuf_init(ringbuf_t *rb, float *storage, size_t cap)
{
    if (rb == NULL) {
        return;
    }
    rb->buf = storage;
    rb->cap = cap;
    rb->head = 0;
    rb->count = 0;
    rb->sum = 0.0f;
}

void ringbuf_push(ringbuf_t *rb, float value)
{
    if (rb == NULL || rb->buf == NULL || rb->cap == 0) {
        return;
    }

    if (rb->count < rb->cap) {
        rb->buf[rb->head] = value;
        rb->sum += value;
        rb->count++;
    } else {
        /* Overwrite the oldest sample: subtract it before replacing. */
        rb->sum -= rb->buf[rb->head];
        rb->buf[rb->head] = value;
        rb->sum += value;
    }

    rb->head = (rb->head + 1) % rb->cap;
}

size_t ringbuf_count(const ringbuf_t *rb)
{
    return (rb == NULL) ? 0 : rb->count;
}

float ringbuf_mean(const ringbuf_t *rb)
{
    if (rb == NULL || rb->count == 0) {
        return 0.0f;
    }
    return rb->sum / (float)rb->count;
}

float ringbuf_min(const ringbuf_t *rb)
{
    if (rb == NULL || rb->count == 0) {
        return 0.0f;
    }
    float min = rb->buf[0];
    for (size_t i = 1; i < rb->count; i++) {
        if (rb->buf[i] < min) {
            min = rb->buf[i];
        }
    }
    return min;
}

float ringbuf_max(const ringbuf_t *rb)
{
    if (rb == NULL || rb->count == 0) {
        return 0.0f;
    }
    float max = rb->buf[0];
    for (size_t i = 1; i < rb->count; i++) {
        if (rb->buf[i] > max) {
            max = rb->buf[i];
        }
    }
    return max;
}

ring_stats_t ringbuf_stats(const ringbuf_t *rb)
{
    ring_stats_t stats = { 0.0f, 0.0f, 0.0f, 0 };
    if (rb == NULL || rb->count == 0) {
        return stats;
    }
    stats.mean = ringbuf_mean(rb);
    stats.min = ringbuf_min(rb);
    stats.max = ringbuf_max(rb);
    stats.count = rb->count;
    return stats;
}
