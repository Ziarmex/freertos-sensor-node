/* SPDX-License-Identifier: MIT */
#include "micros.h"

#include "stm32l4xx_hal.h"

static uint32_t s_cycles_per_us = 1U;

void micros_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55U; /* unlock DWT (required on some cores) */
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    s_cycles_per_us = SystemCoreClock / 1000000U;
    if (s_cycles_per_us == 0U) {
        s_cycles_per_us = 1U;
    }
}

uint32_t micros(void)
{
    return DWT->CYCCNT / s_cycles_per_us;
}
