#include "sht31.h"

#define SHT31_CMD_SOFT_RESET_MSB       0x30U
#define SHT31_CMD_SOFT_RESET_LSB       0xA2U
#define SHT31_CMD_SINGLE_HIGH_MSB      0x24U
#define SHT31_CMD_SINGLE_HIGH_LSB      0x00U
#define SHT31_MEASUREMENT_DELAY_MS     16U
#define SHT31_RESPONSE_LENGTH          6U

static uint8_t sht31_crc8(const uint8_t *data, uint32_t length)
{
    uint8_t crc = 0xFFU;

    for (uint32_t i = 0U; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x80U) != 0U
                ? (uint8_t)((crc << 1U) ^ 0x31U)
                : (uint8_t)(crc << 1U);
        }
    }
    return crc;
}

static sht31_status_t sht31_from_hal(HAL_StatusTypeDef status)
{
    if (status == HAL_TIMEOUT) {
        return SHT31_ERROR_TIMEOUT;
    }
    return (status == HAL_OK) ? SHT31_OK : SHT31_ERROR_I2C;
}

static sht31_status_t sht31_write_command(sht31_t *sensor, uint8_t msb, uint8_t lsb)
{
    uint8_t command[2] = {msb, lsb};
    return sht31_from_hal(HAL_I2C_Master_Transmit(sensor->i2c,
                                                  sensor->address,
                                                  command,
                                                  sizeof(command),
                                                  sensor->timeout_ms));
}

sht31_status_t sht31_init(sht31_t *sensor,
                          I2C_HandleTypeDef *i2c,
                          uint16_t address,
                          uint32_t timeout_ms)
{
    if ((sensor == NULL) || (i2c == NULL) || (timeout_ms == 0U)) {
        return SHT31_ERROR_ARGUMENT;
    }

    sensor->i2c = i2c;
    sensor->address = address;
    sensor->timeout_ms = timeout_ms;
    return sht31_soft_reset(sensor);
}

sht31_status_t sht31_soft_reset(sht31_t *sensor)
{
    if ((sensor == NULL) || (sensor->i2c == NULL)) {
        return SHT31_ERROR_ARGUMENT;
    }

    sht31_status_t status = sht31_write_command(sensor,
                                                 SHT31_CMD_SOFT_RESET_MSB,
                                                 SHT31_CMD_SOFT_RESET_LSB);
    if (status == SHT31_OK) {
        HAL_Delay(2U);
    }
    return status;
}

sht31_status_t sht31_read(sht31_t *sensor, sht31_measurement_t *measurement)
{
    uint8_t response[SHT31_RESPONSE_LENGTH];

    if ((sensor == NULL) || (sensor->i2c == NULL) || (measurement == NULL)) {
        return SHT31_ERROR_ARGUMENT;
    }

    sht31_status_t status = sht31_write_command(sensor,
                                                 SHT31_CMD_SINGLE_HIGH_MSB,
                                                 SHT31_CMD_SINGLE_HIGH_LSB);
    if (status != SHT31_OK) {
        return status;
    }

    HAL_Delay(SHT31_MEASUREMENT_DELAY_MS);
    status = sht31_from_hal(HAL_I2C_Master_Receive(sensor->i2c,
                                                   sensor->address,
                                                   response,
                                                   sizeof(response),
                                                   sensor->timeout_ms));
    if (status != SHT31_OK) {
        return status;
    }

    if ((sht31_crc8(&response[0], 2U) != response[2]) ||
        (sht31_crc8(&response[3], 2U) != response[5])) {
        return SHT31_ERROR_CRC;
    }

    const uint16_t raw_temperature = (uint16_t)(((uint16_t)response[0] << 8U) | response[1]);
    const uint16_t raw_humidity = (uint16_t)(((uint16_t)response[3] << 8U) | response[4]);

    measurement->temperature_c = -45.0f + (175.0f * (float)raw_temperature / 65535.0f);
    measurement->humidity_rh = 100.0f * (float)raw_humidity / 65535.0f;

    if ((measurement->temperature_c < -40.0f) ||
        (measurement->temperature_c > 125.0f) ||
        (measurement->humidity_rh < 0.0f) ||
        (measurement->humidity_rh > 100.0f)) {
        return SHT31_ERROR_RANGE;
    }

    return SHT31_OK;
}

const char *sht31_status_string(sht31_status_t status)
{
    switch (status) {
    case SHT31_OK:             return "ok";
    case SHT31_ERROR_ARGUMENT: return "invalid argument";
    case SHT31_ERROR_I2C:      return "i2c failure";
    case SHT31_ERROR_TIMEOUT:  return "communication timeout";
    case SHT31_ERROR_CRC:      return "crc mismatch";
    case SHT31_ERROR_RANGE:    return "measurement out of range";
    default:                   return "unknown error";
    }
}

