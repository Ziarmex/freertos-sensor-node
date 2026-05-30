/* SPDX-License-Identifier: MIT
 *
 * Synthetic three-channel sensor. Deterministic and smooth so the RTOS
 * behaviour (tasks, queues, stats, low power) can be exercised without a
 * BME280 wired up.
 */
#include <math.h>

#include "app_config.h"
#include "micros.h"
#include "sensor_driver.h"

#define TWO_PI 6.28318530718f

static sensor_status_t sim_init(void)
{
    return SENSOR_OK;
}

static float noise_1e4(void)
{
    /* Cheap deterministic pseudo-noise in [-0.1, +0.1]. */
    static uint32_t lfsr = 0xACE1U;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1U) & 0xB400U);
    return ((float)(lfsr % 2001U) - 1000.0f) / 10000.0f;
}

static sensor_status_t sim_read(sensor_sample_t *out)
{
    if (out == NULL) {
        return SENSOR_ERR;
    }

    float t = (float)(micros() % 60000000U) / 1000000.0f; /* 0..60 s */

    out->temperature_c = 22.0f + 4.0f * sinf(TWO_PI * t / 60.0f) + noise_1e4();
    out->pressure_hpa = 1013.0f + 6.0f * sinf(TWO_PI * t / 45.0f + 1.0f)
                        + 2.0f * noise_1e4();
    out->humidity_pct = 45.0f + 15.0f * sinf(TWO_PI * t / 30.0f + 2.0f)
                        + 5.0f * noise_1e4();
    return SENSOR_OK;
}

const sensor_driver_t sensor_driver_sim = {
    .name = "simulated",
    .init = sim_init,
    .read = sim_read,
};
