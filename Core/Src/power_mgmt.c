/* SPDX-License-Identifier: MIT
 *
 * Low-power bring-up for the STM32L4.
 *
 * The deep-sleep path used by this node is FreeRTOS tickless idle: when all
 * tasks are blocked the kernel suppresses SysTick and the port enters a low
 * power state, waking on the next scheduled tick. Enable it in CubeMX
 * (FreeRTOS interface -> "Tickless idle": enabled; hardware timer: RTC/TIM).
 *
 * `power_mgmt_enter_sleep()` is a plain WFI helper for a task that wants to
 * yield the core between events. Measure real currents with a meter; the
 * values belong in the README table, not in a comment.
 */
#include "power_mgmt.h"

#include "console.h"
#include "stm32l4xx_hal.h"

void power_mgmt_init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    console_write("[power] PWR clock enabled; tickless idle handles sleep\r\n");
}

void power_mgmt_enter_sleep(void)
{
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}
