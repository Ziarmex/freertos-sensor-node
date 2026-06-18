/* SPDX-License-Identifier: MIT
 *
 * Acquisition task (highest priority). Reads the sensor on a fixed period,
 * timestamps with the DWT microsecond counter, and publishes to the
 * processing task. Explicit scheduling lets us report jitter in microseconds.
 */
#include "acq_task.h"

#include "app_config.h"
#include "app_queues.h"
#include "console.h"
#include "cmsis_os2.h"
#include "micros.h"
#include "sensor_driver.h"

void acq_task(void *argument)
{
    (void)argument;
    const sensor_driver_t *driver = sensor_get_driver();
    const uint32_t period_us = APP_SAMPLE_PERIOD_MS * 1000U;
    uint32_t next_us = micros();
    uint32_t seq = 0U;

    for (;;) {
        uint32_t t0 = micros();
        sensor_sample_t sample;
        sensor_status_t status = driver->read(&sample);
        uint32_t t1 = micros();

        if (status == SENSOR_OK) {
            sample_msg_t msg = {
                .sample = sample,
                .t_acq_us = t0,
                .acq_latency_us = t1 - t0,
                .seq = seq++,
            };
            if (osMessageQueuePut(g_sample_queue, &msg, 0U, 0U) == osOK) {
                g_acq_heartbeat++;
            } else {
                console_printf("[acq] queue full, dropped seq=%lu\r\n",
                               (unsigned long)msg.seq);
            }
        } else {
            console_write("[acq] sensor read failed\r\n");
        }

        next_us += period_us;
        uint32_t now = micros();
        uint32_t wait = next_us - now;
        if (wait < period_us * 2U) {
            osDelay((wait + 999U) / 1000U);
        } else {
            /* Wrapped past the deadline: report and resynchronise. */
            console_printf("[acq] behind by %lu us\r\n",
                           (unsigned long)(now - next_us));
            next_us = now;
        }
    }
}
