#ifndef MAIN_H
#define MAIN_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STATUS_LED_Pin       GPIO_PIN_12
#define STATUS_LED_GPIO_Port GPIOD

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif

