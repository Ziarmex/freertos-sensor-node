# SPDX-License-Identifier: MIT
# Portable CMake target for the pure-C sensor core. On STM32 you can also just
# add sensor_core.c to the CubeMX-generated project (see docs/stm32-setup.md).
add_library(sensor_core STATIC sensor_core.c)
target_include_directories(sensor_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
