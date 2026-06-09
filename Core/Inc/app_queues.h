/* SPDX-License-Identifier: MIT
 *
 * Shared IPC objects and message types (CMSIS-RTOS2 message queues).
 */
#pragma once

#include <stdint.h>

#include "cmsis_os2.h"
#include "sensor_core.h"
#include "sensor_driver.h"

typedef struct {
    sensor_sample_t sample;
    uint32_t t_acq_us;       /* DWT timestamp at acquisition start */
    uint32_t acq_latency_us; /* sensor read duration */
    uint32_t seq;
} sample_msg_t;

typedef struct {
    uint32_t t_proc_us;
    uint32_t e2e_latency_us; /* acquisition start -> processed */
    uint32_t acq_latency_us;
    uint32_t seq;
    sensor_sample_t sample;
    ring_stats_t temperature;
    ring_stats_t pressure;
    ring_stats_t humidity;
} stat_msg_t;

extern osMessageQueueId_t g_sample_queue; /* acq -> process */
extern osMessageQueueId_t g_stat_queue;   /* process -> console */
extern volatile uint32_t g_acq_heartbeat; /* incremented by the acq task */
