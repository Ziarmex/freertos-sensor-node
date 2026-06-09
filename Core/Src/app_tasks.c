/* SPDX-License-Identifier: MIT
 *
 * Task graph:
 *   acq (High) --sample_msg--> process (AboveNormal) --stat_msg--> console (Normal)
 *
 * Queues decouple each stage so a slow console never blocks acquisition, and a
 * stalled acquisition is detected by the console's heartbeat check.
 */
#include "app_tasks.h"

#include "acq_task.h"
#include "app_config.h"
#include "app_queues.h"
#include "console.h"
#include "console_task.h"
#include "cmsis_os2.h"
#include "micros.h"
#include "power_mgmt.h"
#include "process_task.h"
#include "sensor_driver.h"

osMessageQueueId_t g_sample_queue;
osMessageQueueId_t g_stat_queue;
volatile uint32_t g_acq_heartbeat;

#define TASK_STACK_BYTES (4U * 1024U)

void app_start(void)
{
    micros_init();

    g_sample_queue = osMessageQueueNew(APP_SAMPLE_QUEUE_LEN,
                                       sizeof(sample_msg_t), NULL);
    g_stat_queue = osMessageQueueNew(APP_STAT_QUEUE_LEN,
                                     sizeof(stat_msg_t), NULL);
    if (g_sample_queue == NULL || g_stat_queue == NULL) {
        console_write("[app] queue creation failed\r\n");
        return;
    }

    const sensor_driver_t *driver = sensor_get_driver();
    console_printf("[app] sensor driver: %s\r\n", driver->name);
    if (driver->init() != SENSOR_OK) {
        console_write("[app] sensor init failed\r\n");
    }

    power_mgmt_init();

    const osThreadAttr_t acq_attr = {
        .name = "acq",
        .stack_size = TASK_STACK_BYTES,
        .priority = osPriorityHigh,
    };
    const osThreadAttr_t process_attr = {
        .name = "process",
        .stack_size = TASK_STACK_BYTES,
        .priority = osPriorityAboveNormal,
    };
    const osThreadAttr_t console_attr = {
        .name = "console",
        .stack_size = TASK_STACK_BYTES,
        .priority = osPriorityNormal,
    };

    osThreadId_t acq = osThreadNew(acq_task, NULL, &acq_attr);
    osThreadId_t process = osThreadNew(process_task, NULL, &process_attr);
    osThreadId_t console = osThreadNew(console_task, NULL, &console_attr);

    if (acq == NULL || process == NULL || console == NULL) {
        console_write("[app] task creation failed\r\n");
        return;
    }

    console_printf("[app] node started (period=%lu ms, window=%lu)\r\n",
                   (unsigned long)APP_SAMPLE_PERIOD_MS,
                   (unsigned long)APP_STATS_WINDOW);
}
