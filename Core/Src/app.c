#include "app.h"
#include "main.h"
#include "sht31.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define APP_SENSOR_TIMEOUT_MS  100U
#define APP_UART_TIMEOUT_MS    100U
#define APP_MAX_FAILURES       3U

static sht31_t sensor;
static UART_HandleTypeDef *uart;
static volatile uint8_t sample_due;
static uint32_t sample_count;
static uint32_t consecutive_failures;

static void app_log(const char *format, ...)
{
    char buffer[128];
    va_list arguments;

    va_start(arguments, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, arguments);
    va_end(arguments);

    if ((uart != NULL) && (length > 0)) {
        uint16_t tx_length = (uint16_t)((length < (int)sizeof(buffer))
            ? length
            : ((int)sizeof(buffer) - 1));
        (void)HAL_UART_Transmit(uart, (uint8_t *)buffer, tx_length, APP_UART_TIMEOUT_MS);
    }
}

void app_init(const app_config_t *config)
{
    if ((config == NULL) || (config->sensor_i2c == NULL) || (config->debug_uart == NULL)) {
        Error_Handler();
    }

    uart = config->debug_uart;
    app_log("\r\nSTM32F407 humidity monitor starting\r\n");

    sht31_status_t status = sht31_init(&sensor,
                                       config->sensor_i2c,
                                       SHT31_I2C_ADDRESS_DEFAULT,
                                       APP_SENSOR_TIMEOUT_MS);
    if (status != SHT31_OK) {
        app_log("SHT31 initialization failed: %s\r\n", sht31_status_string(status));
    } else {
        app_log("SHT31 ready at 7-bit address 0x44\r\n");
    }

    sample_due = 1U;
}

void app_sample_timer_callback(void)
{
    sample_due = 1U;
}

void app_process(void)
{
    if (sample_due == 0U) {
        return;
    }
    sample_due = 0U;

    sht31_measurement_t measurement;
    sht31_status_t status = sht31_read(&sensor, &measurement);

    if (status == SHT31_OK) {
        consecutive_failures = 0U;
        ++sample_count;
        HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
        app_log("[%lu] temperature=%.2f C, humidity=%.2f %%RH\r\n",
                (unsigned long)sample_count,
                (double)measurement.temperature_c,
                (double)measurement.humidity_rh);
        return;
    }

    ++consecutive_failures;
    HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_RESET);
    app_log("Sensor read failed (%lu/%u): %s\r\n",
            (unsigned long)consecutive_failures,
            APP_MAX_FAILURES,
            sht31_status_string(status));

    if (consecutive_failures >= APP_MAX_FAILURES) {
        app_log("Attempting SHT31 recovery\r\n");
        status = sht31_soft_reset(&sensor);
        app_log("Recovery result: %s\r\n", sht31_status_string(status));
        consecutive_failures = 0U;
    }
}

