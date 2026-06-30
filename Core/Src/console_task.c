/* SPDX-License-Identifier: MIT
 *
 * Console task (lowest priority). Prints periodic statistics and acts as a
 * lightweight software watchdog: it warns when the acquisition heartbeat stops
 * advancing, without blocking on any queue.
 */
#include "console_task.h"

#include "app_config.h"
#include "app_queues.h"
#include "cmsis_os2.h"
#include "console.h"
#include "micros.h"

#define HEARTBEAT_TIMEOUT_US (APP_SAMPLE_PERIOD_MS * 1000U * 3U)

void console_task(void *argument)
{
    (void)argument;
    stat_msg_t stat;
    uint32_t printed = 0U;
    uint32_t last_heartbeat = 0U;
    uint32_t last_change_us = micros();

    for (;;) {
        if (osMessageQueueGet(g_stat_queue, &stat, NULL, 1000U) == osOK) {
            if (++printed % APP_CONSOLE_EVERY == 0U) {
                char t[12], mean[12], min[12], max[12], press[12], hum[12];
                console_format_fixed(t, sizeof(t), stat.sample.temperature_c, 2);
                console_format_fixed(mean, sizeof(mean), stat.temperature.mean, 2);
                console_format_fixed(min, sizeof(min), stat.temperature.min, 2);
                console_format_fixed(max, sizeof(max), stat.temperature.max, 2);
                console_format_fixed(press, sizeof(press), stat.sample.pressure_hpa, 1);
                console_format_fixed(hum, sizeof(hum), stat.sample.humidity_pct, 1);

                console_printf(
                    "[node] seq=%lu T=%sC [mean=%s min=%s max=%s n=%u] "
                    "P=%shPa RH=%s%% acq=%luus e2e=%luus\r\n",
                    (unsigned long)stat.seq, t, mean, min, max,
                    (unsigned)stat.temperature.count, press, hum,
                    (unsigned long)stat.acq_latency_us,
                    (unsigned long)stat.e2e_latency_us);
            }
        }

        uint32_t heartbeat = g_acq_heartbeat;
        if (heartbeat != last_heartbeat) {
            last_heartbeat = heartbeat;
            last_change_us = micros();
        } else if ((micros() - last_change_us) > HEARTBEAT_TIMEOUT_US) {
            console_printf("[node] ERROR acquisition stalled (hb=%lu)\r\n",
                           (unsigned long)heartbeat);
            last_change_us = micros();
        }
    }
}
