/* SPDX-License-Identifier: MIT
 *
 * Application entry point called from MX_FREERTOS_Init() in the CubeMX
 * generated freertos.c (inside a USER CODE block). Creates shared queues and
 * starts the node tasks.
 */
#pragma once

void app_start(void);
