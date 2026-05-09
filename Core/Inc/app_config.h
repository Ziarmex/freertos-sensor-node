/* SPDX-License-Identifier: MIT
 *
 * Application configuration. On STM32 this replaces Kconfig: edit the values
 * here (or override with -D flags in the build).
 */
#pragma once

/* Acquisition period in milliseconds. */
#ifndef APP_SAMPLE_PERIOD_MS
#define APP_SAMPLE_PERIOD_MS 1000U
#endif

/* Sliding-window length per channel. */
#ifndef APP_STATS_WINDOW
#define APP_STATS_WINDOW 32U
#endif

/* Print one stat line every N samples. */
#ifndef APP_CONSOLE_EVERY
#define APP_CONSOLE_EVERY 10U
#endif

/* FreeRTOS queue depths. */
#ifndef APP_SAMPLE_QUEUE_LEN
#define APP_SAMPLE_QUEUE_LEN 16U
#endif
#ifndef APP_STAT_QUEUE_LEN
#define APP_STAT_QUEUE_LEN 8U
#endif

/* 1 = synthetic sensor (no hardware), 0 = real BME280 over I2C. */
#ifndef APP_USE_SIMULATED_SENSOR
#define APP_USE_SIMULATED_SENSOR 1
#endif

/* BME280 I2C address: 0x76, 0x77, or 0 to auto-probe both. */
#ifndef APP_BME280_ADDRESS
#define APP_BME280_ADDRESS 0x77U
#endif

#ifndef APP_I2C_TIMEOUT_MS
#define APP_I2C_TIMEOUT_MS 100U
#endif
