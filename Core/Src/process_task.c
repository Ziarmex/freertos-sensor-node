/* SPDX-License-Identifier: MIT
 *
 * Processing task. Consumes raw samples, maintains one sliding window per
 * channel and forwards an aggregated stat message with measured end-to-end
 * latency.
 */
#include "process_task.h"

#include "app_config.h"
#include "app_queues.h"
#include "cmsis_os2.h"
#include "micros.h"
#include "sensor_core.h"

static float s_temp_buf[APP_STATS_WINDOW];
static float s_press_buf[APP_STATS_WINDOW];
static float s_hum_buf[APP_STATS_WINDOW];

static ringbuf_t s_temp;
static ringbuf_t s_press;
static ringbuf_t s_hum;

void process_task(void *argument)
{
    (void)argument;
    ringbuf_init(&s_temp, s_temp_buf, APP_STATS_WINDOW);
    ringbuf_init(&s_press, s_press_buf, APP_STATS_WINDOW);
    ringbuf_init(&s_hum, s_hum_buf, APP_STATS_WINDOW);

    sample_msg_t msg;
    for (;;) {
        if (osMessageQueueGet(g_sample_queue, &msg, NULL, osWaitForever) != osOK) {
            continue;
        }

        ringbuf_push(&s_temp, msg.sample.temperature_c);
        ringbuf_push(&s_press, msg.sample.pressure_hpa);
        ringbuf_push(&s_hum, msg.sample.humidity_pct);

        uint32_t t_proc = micros();
        stat_msg_t out = {
            .t_proc_us = t_proc,
            .e2e_latency_us = t_proc - msg.t_acq_us,
            .acq_latency_us = msg.acq_latency_us,
            .seq = msg.seq,
            .sample = msg.sample,
            .temperature = ringbuf_stats(&s_temp),
            .pressure = ringbuf_stats(&s_press),
            .humidity = ringbuf_stats(&s_hum),
        };

        /* Stats are best-effort: never stall acquisition on the console. */
        (void)osMessageQueuePut(g_stat_queue, &out, 0U, 0U);
    }
}
