/* SPDX-License-Identifier: MIT
 *
 * Host unit tests for the pure-C ring buffer. Compiled with gcc in CI, no
 * ESP-IDF or hardware required.
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "sensor_core.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static int approx(float a, float b)
{
    return fabsf(a - b) < 1e-4f;
}

static void test_empty(void)
{
    float storage[4];
    ringbuf_t rb;
    ringbuf_init(&rb, storage, ARRAY_LEN(storage));

    assert(ringbuf_count(&rb) == 0);
    assert(approx(ringbuf_mean(&rb), 0.0f));
    assert(approx(ringbuf_min(&rb), 0.0f));
    assert(approx(ringbuf_max(&rb), 0.0f));
    puts("  empty ring: ok");
}

static void test_partial_fill(void)
{
    float storage[8];
    ringbuf_t rb;
    ringbuf_init(&rb, storage, ARRAY_LEN(storage));

    ringbuf_push(&rb, 10.0f);
    ringbuf_push(&rb, 20.0f);
    ringbuf_push(&rb, 30.0f);

    assert(ringbuf_count(&rb) == 3);
    assert(approx(ringbuf_mean(&rb), 20.0f));
    assert(approx(ringbuf_min(&rb), 10.0f));
    assert(approx(ringbuf_max(&rb), 30.0f));
    puts("  partial fill: ok");
}

static void test_wrap_keeps_last_cap(void)
{
    float storage[4];
    ringbuf_t rb;
    ringbuf_init(&rb, storage, ARRAY_LEN(storage));

    for (float v = 1.0f; v <= 10.0f; v += 1.0f) {
        ringbuf_push(&rb, v);
    }

    /* Last four values are 7,8,9,10. */
    assert(ringbuf_count(&rb) == 4);
    assert(approx(ringbuf_min(&rb), 7.0f));
    assert(approx(ringbuf_max(&rb), 10.0f));
    assert(approx(ringbuf_mean(&rb), 8.5f));
    puts("  wrap keeps last cap: ok");
}

static void test_sum_does_not_drift(void)
{
    float storage[16];
    ringbuf_t rb;
    ringbuf_init(&rb, storage, ARRAY_LEN(storage));

    for (int i = 0; i < 100000; i++) {
        ringbuf_push(&rb, 1.0f);
    }
    assert(ringbuf_count(&rb) == 16);
    assert(approx(ringbuf_mean(&rb), 1.0f));
    puts("  long run sum stable: ok");
}

static void test_stats_after_wrap(void)
{
    float storage[3];
    ringbuf_t rb;
    ringbuf_init(&rb, storage, ARRAY_LEN(storage));

    ringbuf_push(&rb, 5.0f);
    ringbuf_push(&rb, -1.0f);
    ringbuf_push(&rb, 3.0f);
    ringbuf_push(&rb, 9.0f); /* evicts 5.0 */

    ring_stats_t s = ringbuf_stats(&rb);
    assert(s.count == 3);
    assert(approx(s.min, -1.0f));
    assert(approx(s.max, 9.0f));
    assert(approx(s.mean, 11.0f / 3.0f));
    puts("  stats after wrap: ok");
}

static void test_zero_capacity_is_safe(void)
{
    ringbuf_t rb;
    ringbuf_init(&rb, NULL, 0);
    ringbuf_push(&rb, 1.0f);
    assert(ringbuf_count(&rb) == 0);
    puts("  zero capacity safe: ok");
}

int main(void)
{
    puts("sensor_core host tests");
    test_empty();
    test_partial_fill();
    test_wrap_keeps_last_cap();
    test_sum_does_not_drift();
    test_stats_after_wrap();
    test_zero_capacity_is_safe();
    puts("ALL PASSED");
    return 0;
}
