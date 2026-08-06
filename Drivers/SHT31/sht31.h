#ifndef SHT31_H
#define SHT31_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define SHT31_I2C_ADDRESS_DEFAULT  (0x44U << 1U)

typedef enum {
    SHT31_OK = 0,
    SHT31_ERROR_ARGUMENT,
    SHT31_ERROR_I2C,
    SHT31_ERROR_TIMEOUT,
    SHT31_ERROR_CRC,
    SHT31_ERROR_RANGE
} sht31_status_t;

typedef struct {
    float temperature_c;
    float humidity_rh;
} sht31_measurement_t;

typedef struct {
    I2C_HandleTypeDef *i2c;
    uint16_t address;
    uint32_t timeout_ms;
} sht31_t;

sht31_status_t sht31_init(sht31_t *sensor,
                          I2C_HandleTypeDef *i2c,
                          uint16_t address,
                          uint32_t timeout_ms);
sht31_status_t sht31_soft_reset(sht31_t *sensor);
sht31_status_t sht31_read(sht31_t *sensor, sht31_measurement_t *measurement);
const char *sht31_status_string(sht31_status_t status);

#endif

