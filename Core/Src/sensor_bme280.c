/* SPDX-License-Identifier: MIT
 *
 * Compact BME280 driver over I2C (temperature, pressure, humidity) using the
 * STM32 HAL. The I2C peripheral itself is configured by CubeMX; this file only
 * talks to the device.
 *
 * Compensation uses the Bosch integer equations from the BME280 datasheet.
 * They are well-established arithmetic but have NOT been validated against a
 * calibrated reference on real hardware here - treat as experimental until
 * you log real data. Simulated mode is the default.
 */
#include <string.h>

#include "app_config.h"
#include "console.h"
#include "sensor_driver.h"
#include "stm32l4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;

#define BME280_ADDR_PRIMARY   0x76U
#define BME280_ADDR_SECONDARY 0x77U

#define BME280_REG_ID         0xD0U
#define BME280_REG_CALIB_TP   0x88U /* 0x88..0xA1 (26 bytes) */
#define BME280_REG_CALIB_H1   0xA1U
#define BME280_REG_CALIB_H2   0xE1U /* 0xE1..0xE7 (7 bytes) */
#define BME280_REG_CTRL_HUM   0xF2U
#define BME280_REG_CTRL_MEAS  0xF4U
#define BME280_REG_CONFIG     0xF5U
#define BME280_REG_DATA       0xF7U /* 0xF7..0xFE (8 bytes) */

typedef struct {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} bme280_calib_t;

static bme280_calib_t s_calib;
static uint8_t s_addr;
static int32_t s_t_fine;

static sensor_status_t bme_read(uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(s_addr << 1),
                                            reg, I2C_MEMADD_SIZE_8BIT,
                                            data, len, APP_I2C_TIMEOUT_MS);
    return (st == HAL_OK) ? SENSOR_OK : SENSOR_ERR;
}

static sensor_status_t bme_write(uint8_t reg, uint8_t value)
{
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(s_addr << 1),
                                             reg, I2C_MEMADD_SIZE_8BIT,
                                             &value, 1, APP_I2C_TIMEOUT_MS);
    return (st == HAL_OK) ? SENSOR_OK : SENSOR_ERR;
}

static sensor_status_t read_calibration(void)
{
    uint8_t tp[26];
    uint8_t h2[7];
    uint8_t h1;

    if (bme_read(BME280_REG_CALIB_TP, tp, sizeof(tp)) != SENSOR_OK) {
        return SENSOR_ERR;
    }
    if (bme_read(BME280_REG_CALIB_H2, h2, sizeof(h2)) != SENSOR_OK) {
        return SENSOR_ERR;
    }
    if (bme_read(BME280_REG_CALIB_H1, &h1, 1) != SENSOR_OK) {
        return SENSOR_ERR;
    }

    s_calib.dig_T1 = (uint16_t)((tp[1] << 8) | tp[0]);
    s_calib.dig_T2 = (int16_t)((tp[3] << 8) | tp[2]);
    s_calib.dig_T3 = (int16_t)((tp[5] << 8) | tp[4]);

    s_calib.dig_P1 = (uint16_t)((tp[7] << 8) | tp[6]);
    s_calib.dig_P2 = (int16_t)((tp[9] << 8) | tp[8]);
    s_calib.dig_P3 = (int16_t)((tp[11] << 8) | tp[10]);
    s_calib.dig_P4 = (int16_t)((tp[13] << 8) | tp[12]);
    s_calib.dig_P5 = (int16_t)((tp[15] << 8) | tp[14]);
    s_calib.dig_P6 = (int16_t)((tp[17] << 8) | tp[16]);
    s_calib.dig_P7 = (int16_t)((tp[19] << 8) | tp[18]);
    s_calib.dig_P8 = (int16_t)((tp[21] << 8) | tp[20]);
    s_calib.dig_P9 = (int16_t)((tp[23] << 8) | tp[22]);

    s_calib.dig_H1 = h1;
    s_calib.dig_H2 = (int16_t)((h2[1] << 8) | h2[0]);
    s_calib.dig_H3 = h2[2];
    s_calib.dig_H4 = (int16_t)((h2[3] << 4) | (h2[4] & 0x0F));
    s_calib.dig_H5 = (int16_t)((h2[5] << 4) | (h2[4] >> 4));
    s_calib.dig_H6 = (int8_t)h2[6];

    return SENSOR_OK;
}

static sensor_status_t probe_address(void)
{
#if APP_BME280_ADDRESS != 0
    uint8_t id = 0;
    s_addr = (uint8_t)APP_BME280_ADDRESS;
    if (bme_read(BME280_REG_ID, &id, 1) == SENSOR_OK && id == 0x60U) {
        return SENSOR_OK;
    }
    return SENSOR_ERR;
#else
    const uint8_t candidates[] = { BME280_ADDR_PRIMARY, BME280_ADDR_SECONDARY };
    for (size_t i = 0; i < sizeof(candidates); i++) {
        uint8_t id = 0;
        s_addr = candidates[i];
        if (bme_read(BME280_REG_ID, &id, 1) == SENSOR_OK && id == 0x60U) {
            return SENSOR_OK;
        }
    }
    return SENSOR_ERR;
#endif
}

static sensor_status_t bme280_init(void)
{
    if (probe_address() != SENSOR_OK) {
        console_write("[bme280] no device found on I2C\r\n");
        return SENSOR_ERR;
    }
    if (read_calibration() != SENSOR_OK) {
        console_write("[bme280] calibration read failed\r\n");
        return SENSOR_ERR;
    }

    /* humidity x1, temperature x1, pressure x1, normal mode, t_sb 0.5 ms. */
    if (bme_write(BME280_REG_CTRL_HUM, 0x01U) != SENSOR_OK ||
        bme_write(BME280_REG_CONFIG, 0xA0U) != SENSOR_OK ||
        bme_write(BME280_REG_CTRL_MEAS, 0x27U) != SENSOR_OK) {
        console_write("[bme280] configuration failed\r\n");
        return SENSOR_ERR;
    }

    console_printf("[bme280] ready at 0x%02X\r\n", (unsigned)s_addr);
    return SENSOR_OK;
}

static int32_t compensate_temperature(int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)s_calib.dig_T1 << 1)))
                    * ((int32_t)s_calib.dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)s_calib.dig_T1))
                      * ((adc_T >> 4) - ((int32_t)s_calib.dig_T1))) >> 12)
                    * ((int32_t)s_calib.dig_T3)) >> 14;
    s_t_fine = var1 + var2;
    return (s_t_fine * 5 + 128) >> 8; /* 0.01 degC */
}

static uint32_t compensate_pressure(int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)s_t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)s_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)s_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)s_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)s_calib.dig_P3) >> 8)
           + ((var1 * (int64_t)s_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)s_calib.dig_P1) >> 33;

    if (var1 == 0) {
        return 0;
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)s_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)s_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)s_calib.dig_P7) << 4);
    return (uint32_t)p; /* Pa */
}

static uint32_t compensate_humidity(int32_t adc_H)
{
    int32_t v_x1 = s_t_fine - ((int32_t)76800);

    v_x1 = (((((adc_H << 14) - (((int32_t)s_calib.dig_H4) << 20)
               - (((int32_t)s_calib.dig_H5) * v_x1)) + ((int32_t)16384)) >> 15)
            * (((((((v_x1 * ((int32_t)s_calib.dig_H6)) >> 10)
                   * (((v_x1 * ((int32_t)s_calib.dig_H3)) >> 11)
                      + ((int32_t)32768))) >> 10)
                 + ((int32_t)2097152)) * ((int32_t)s_calib.dig_H2)
                + 8192) >> 14));

    v_x1 = v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7)
                    * ((int32_t)s_calib.dig_H1)) >> 4);
    v_x1 = (v_x1 < 0) ? 0 : v_x1;
    v_x1 = (v_x1 > 419430400) ? 419430400 : v_x1;
    return (uint32_t)(v_x1 >> 12); /* 1024 == 100 %RH */
}

static sensor_status_t bme280_read(sensor_sample_t *out)
{
    if (out == NULL) {
        return SENSOR_ERR;
    }

    uint8_t d[8];
    if (bme_read(BME280_REG_DATA, d, sizeof(d)) != SENSOR_OK) {
        return SENSOR_ERR;
    }

    int32_t adc_P = (int32_t)((((uint32_t)d[0]) << 12)
                              | (((uint32_t)d[1]) << 4)
                              | (((uint32_t)d[2]) >> 4));
    int32_t adc_T = (int32_t)((((uint32_t)d[3]) << 12)
                              | (((uint32_t)d[4]) << 4)
                              | (((uint32_t)d[5]) >> 4));
    int32_t adc_H = (int32_t)((((uint32_t)d[6]) << 8) | d[7]);

    out->temperature_c = (float)compensate_temperature(adc_T) / 100.0f;
    out->pressure_hpa = (float)compensate_pressure(adc_P) / 100.0f;
    out->humidity_pct = (float)compensate_humidity(adc_H) / 1024.0f;
    return SENSOR_OK;
}

const sensor_driver_t sensor_driver_bme280 = {
    .name = "BME280",
    .init = bme280_init,
    .read = bme280_read,
};
