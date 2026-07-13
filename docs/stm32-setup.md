# STM32 setup (STM32CubeMX / CubeIDE)

Reference target: **NUCLEO-L476RG** (STM32L476RGT6, Cortex-M4F, ultra-low-power).
The code is portable across STM32L4/F4 families with minor changes; only the
registers touched by `micros.c` (DWT) and the HAL handles (`hi2c1`, `huart2`)
are MCU-specific.

## 1. Create the project

New project -> select **STM32L476RGTx** (or board NUCLEO-L476RG).

### Clock

- SYSCLK **80 MHz** (L4 maximum). A working setup:
  HSI16 -> PLL (`PLLM=1`, `PLLN=10`, `PLLR=2`) -> 80 MHz.
- AHB = 80 MHz, APB1 = 80 MHz, APB2 = 80 MHz.

### Peripherals

| Peripheral | Pins | Settings | Purpose |
|-----------|------|----------|---------|
| I2C1 | PB8 = SCL, PB9 = SDA | 400 kHz, 7-bit | BME280 |
| USART2 | PA2 = TX, PA3 = RX | 115200 8N1 | Console / ST-Link VCP |
| GPIO | PA5 | output | LD2 status LED |
| PWR | - | enabled | low-power modes |

### FreeRTOS

- Interface: **CMSIS-RTOS2** (default; generates FreeRTOS).
- `configTICK_RATE_HZ` = **1000** (matches the 1 ms `osDelay` unit in the code).
- `configTOTAL_HEAP_SIZE` >= **15360** bytes (3 tasks + 2 queues).
- `configUSE_PREEMPTION` = 1, `configMAX_PRIORITIES` = 7.
- Optional: **Tickless idle** enabled (hardware timer RTC/TIM) for automatic
  light sleep when all tasks are blocked. See `power_mgmt.c`.

### Project settings

- Toolchain: **STM32CubeIDE** (recommended) or Makefile.
- Keep the default `Core/Inc` and `Core/Src` layout.

## 2. Add the application files

Copy from this repository into the generated project:

```
Core/Inc/app_config.h      Core/Src/app_tasks.c
Core/Inc/app_queues.h      Core/Src/acq_task.c
Core/Inc/acq_task.h        Core/Src/process_task.c
Core/Inc/process_task.h    Core/Src/console_task.c
Core/Inc/console_task.h    Core/Src/console.c
Core/Inc/sensor_driver.h   Core/Src/sensor.c
Core/Inc/console.h         Core/Src/sensor_sim.c
Core/Inc/micros.h          Core/Src/sensor_bme280.c
Core/Inc/power_mgmt.h      Core/Src/power_mgmt.c
Core/Inc/app_tasks.h       Core/Src/micros.c
```

Copy `components/sensor_core/{include/sensor_core.h,sensor_core.c}` into the
project too (e.g. `Core/Inc/sensor_core.h`, `Core/Src/sensor_core.c`), or add
the folder to the include path. In this repository the core is also built and
tested on the host - keep it dependency-free.

### Makefile projects

CubeMX lists sources explicitly, so add the new files to `C_SOURCES` in the
generated `Makefile`:

```make
C_SOURCES += \
Core/Src/app_tasks.c \
Core/Src/acq_task.c \
Core/Src/process_task.c \
Core/Src/console_task.c \
Core/Src/sensor.c \
Core/Src/sensor_sim.c \
Core/Src/sensor_bme280.c \
Core/Src/power_mgmt.c \
Core/Src/micros.c
```

Also add `Core/Inc` to `C_INCLUDES` (it usually already is).

## 3. Start the node

In the generated `Core/Src/freertos.c`, inside `MX_FREERTOS_Init()`, add the
call **inside the USER CODE block** (CubeMX preserves these across
regeneration):

```c
/* USER CODE BEGIN RTOS_THREADS */
app_start();
/* USER CODE END RTOS_THREADS */
```

Add `#include "app_tasks.h"` in the `/* USER CODE BEGIN Includes */` section.

`app_start()` creates the two queues and the three tasks itself, so you do not
need to define any FreeRTOS task in the CubeMX UI.

## 4. Build and flash

STM32CubeIDE: Build -> Run/Debug. The console prints on the ST-Link VCP at
115200 baud.

Makefile:

```bash
make -j
st-flash write build/<project>.bin 0x8000000   # or use the CubeMX/STM32CubeProgrammer flow
```

## 5. Real BME280

Wire SDA->PB9, SCL->PB8, VCC->3V3, GND->GND. Then set in `app_config.h`:

```c
#define APP_USE_SIMULATED_SENSOR 0
#define APP_BME280_ADDRESS 0x77U   /* or 0x76, or 0 to auto-probe */
```

The simulated sensor stays the default so the node runs with no wiring.

## Troubleshooting

- **Nothing on the console**: confirm USART2 is on PA2/PA3 and the terminal is
  115200 8N1 on the ST-Link VCP.
- **`[bme280] no device found`**: check pull-ups (many breakout boards have
  them), address strapping, and that I2C1 is PB8/PB9.
- **Task creation fails**: increase `configTOTAL_HEAP_SIZE`.
- **`osDelay` too coarse**: ensure `configTICK_RATE_HZ = 1000`.
