/* SPDX-License-Identifier: MIT */
#include "sensor_driver.h"

#include "app_config.h"

#if APP_USE_SIMULATED_SENSOR
extern const sensor_driver_t sensor_driver_sim;
#define SELECTED_DRIVER (&sensor_driver_sim)
#else
extern const sensor_driver_t sensor_driver_bme280;
#define SELECTED_DRIVER (&sensor_driver_bme280)
#endif

const sensor_driver_t *sensor_get_driver(void)
{
    return SELECTED_DRIVER;
}
