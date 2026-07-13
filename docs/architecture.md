# Architecture

## Why this structure

A sensor node has two very different rates: the sensor must be read on a precise
period, while printing/storing/transmitting can tolerate latency and jitter.
Coupling them directly (read -> print in one loop) makes the fast path hostage
to the slow path. The node splits them with queues and fixed priorities.

## Tasks (CMSIS-RTOS2 over FreeRTOS)

| Task | Priority | Stack | Blocks on |
|------|----------|-------|-----------|
| `acq_task` | `osPriorityHigh` | 4 KB | `osDelay` until the next period |
| `process_task` | `osPriorityAboveNormal` | 4 KB | `g_sample_queue` |
| `console_task` | `osPriorityNormal` | 4 KB | `g_stat_queue` (1 s timeout) |

Priority rule: the task with the tightest deadline runs highest. `console_task`
is lowest on purpose - if it falls behind, only log lines are lost.

## Data flow

```
sensor -> acq_task --sample_msg_t--> process_task --stat_msg_t--> console_task
```

`sample_msg_t` carries the sample, the acquisition start timestamp and the
measured read duration. `stat_msg_t` adds per-channel `ring_stats_t` and the
end-to-end latency. Carrying timestamps in the message (not in globals) is what
makes the latency numbers trustworthy.

## Timebase

SysTick belongs to FreeRTOS, so latency is measured with the Cortex-M4 **DWT
cycle counter** (`micros.c`): enable `DEMCR.TRCENA` and `DWT.CYCCNT`, convert
cycles to microseconds with `SystemCoreClock`. 32-bit microseconds wrap every
~71 minutes at 80 MHz; all latency math uses unsigned subtraction and is
therefore wrap-safe.

- `acq_latency_us = t1 - t0` around the sensor read.
- `e2e_latency_us = t_proc - t_acq`.
- Scheduling jitter = actual wake minus expected `next_us`; the acq task logs
  when it is behind and resynchronises.

## Backpressure

- Sample queue full -> drop the newest sample, count it, log it. Acquisition
  never blocks on the consumer.
- Stat queue full -> drop the stat view. Stats are lossy by design.

## Fault detection

`acq_task` increments `g_acq_heartbeat` on every successful publish.
`console_task` records when the heartbeat last changed and logs an error when it
stops advancing for three sample periods. This catches a wedged sensor or a
crashed high-priority task without a hardware watchdog.

## Low power

FreeRTOS run-time stats are disabled and the design keeps tasks blocked except
during work. With **tickless idle** enabled in CubeMX, the kernel suppresses
SysTick and the port sleeps when all tasks are blocked, waking on the next
scheduled tick. Deep modes (STOP2) and measured currents belong in the README
results table once measured.

## Testability

The statistics/ring-buffer logic has no RTOS or HAL dependency
(`components/sensor_core`), so it is compiled and tested with plain gcc in CI.
This keeps the untestable firmware surface small: the RTOS/HAL wiring is covered
by the build plus on-target runs, while the arithmetic is covered on the host.
