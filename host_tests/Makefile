# SPDX-License-Identifier: MIT
CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O2
CORE     = ../components/sensor_core
INCLUDES = -I$(CORE)/include

.PHONY: test clean

test: test_sensor_core
	./test_sensor_core

test_sensor_core: test_sensor_core.c $(CORE)/sensor_core.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lm

clean:
	rm -f test_sensor_core
