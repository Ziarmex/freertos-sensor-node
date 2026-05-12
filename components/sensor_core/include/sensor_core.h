/* SPDX-License-Identifier: MIT
 *
 * Pure-C ring buffer and statistics. No FreeRTOS or ESP-IDF dependency, so it
 * is unit-tested on the host in CI.
 */
#pragma once

#include <stddef.h>

typedef struct {
    float *buf;
    size_t cap;
    size_t head;   /* next write index */
    size_t count;  /* number of valid samples, saturates at cap */
    float sum;
} ringbuf_t;

void ringbuf_init(ringbuf_t *rb, float *storage, size_t cap);

/* Push a sample, overwriting the oldest value once the buffer is full. */
void ringbuf_push(ringbuf_t *rb, float value);

size_t ringbuf_count(const ringbuf_t *rb);

float ringbuf_mean(const ringbuf_t *rb);
float ringbuf_min(const ringbuf_t *rb);
float ringbuf_max(const ringbuf_t *rb);

typedef struct {
    float mean;
    float min;
    float max;
    size_t count;
} ring_stats_t;

ring_stats_t ringbuf_stats(const ringbuf_t *rb);
