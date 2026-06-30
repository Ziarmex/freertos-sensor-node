/* SPDX-License-Identifier: MIT
 *
 * Minimal, dependency-free console over the ST-Link VCP UART. Uses snprintf
 * into a fixed buffer plus HAL_UART_Transmit, so no newlib syscall retargeting
 * (and no float printf / -u _printf_float) is required.
 */
#pragma once

#include <stddef.h>

void console_write(const char *text);
void console_printf(const char *fmt, ...);

/* Format a float as a fixed-point string without printf float support.
 * decimals: 0, 1 or 2. */
void console_format_fixed(char *out, size_t cap, float value, unsigned decimals);
