/* SPDX-License-Identifier: MIT */
#pragma once

void power_mgmt_init(void);

/* Enter Cortex-M4 sleep mode (WFI) until the next interrupt. */
void power_mgmt_enter_sleep(void);
