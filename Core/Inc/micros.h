/* SPDX-License-Identifier: MIT
 *
 * Microsecond timebase using the Cortex-M4 DWT cycle counter. The SysTick is
 * owned by the RTOS, so we use DWT for latency measurement instead.
 */
#pragma once

#include <stdint.h>

void micros_init(void);

/* Monotonic microseconds, wraps every ~71 minutes at 80 MHz. All latency
 * computations use unsigned subtraction, which is wrap-safe. */
uint32_t micros(void);
