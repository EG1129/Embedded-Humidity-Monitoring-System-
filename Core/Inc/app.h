#ifndef APP_H
#define APP_H

#include "stm32f4xx_hal.h"

typedef struct {
    I2C_HandleTypeDef *sensor_i2c;
    UART_HandleTypeDef *debug_uart;
} app_config_t;

void app_init(const app_config_t *config);
void app_process(void);
void app_sample_timer_callback(void);

#endif

