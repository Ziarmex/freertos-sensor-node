/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>

typedef struct {
    float temperature_c;
    float pressure_hpa;
    float humidity_pct;
} sensor_sample_t;

typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERR,
} sensor_status_t;

typedef struct {
    const char *name;
    sensor_status_t (*init)(void);
    sensor_status_t (*read)(sensor_sample_t *out);
} sensor_driver_t;

/* Returns the driver selected via APP_USE_SIMULATED_SENSOR. */
const sensor_driver_t *sensor_get_driver(void);
