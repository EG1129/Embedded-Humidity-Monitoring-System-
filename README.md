# Embedded Humidity Monitoring System

Embedded C firmware for an **STM32F407** that samples a Sensirion **SHT31** humidity/temperature sensor over I²C and streams validated readings over UART. A 1 Hz timer interrupt schedules acquisition without performing blocking bus work inside the ISR, while an LED provides immediate hardware status.

## Highlights

- Datasheet-based SHT31 command sequencing and raw-value conversion
- CRC-8 validation for both temperature and humidity words
- I²C timeout, bus-error, CRC, and range-error reporting
- Automatic sensor soft reset after three consecutive read failures
- TIM2 interrupt-driven 1 Hz sampling schedule
- USART2 diagnostics at 115200 baud
- Modular driver/application separation suitable for reuse and unit testing
- Lightweight cooperative main loop; peripheral work stays outside the ISR

## Hardware

| Signal | STM32F407 pin | Connection |
|---|---:|---|
| I²C1 SCL | PB6 | SHT31 SCL |
| I²C1 SDA | PB7 | SHT31 SDA |
| USART2 TX | PA2 | USB-to-UART RX |
| USART2 RX | PA3 | USB-to-UART TX (optional) |
| Status LED | PD12 | STM32F4-Discovery green LED |
| Sensor power | 3.3 V / GND | SHT31 VIN / GND |

The SHT31 address is `0x44` when ADDR is low. I²C requires pull-ups; many breakout boards include them. If yours does not, add approximately 4.7 kΩ from SCL and SDA to 3.3 V. All grounds must be common.

## Software architecture

```text
TIM2 ISR (1 Hz)
      |
      v  sets flag only
app_process() ----> SHT31 driver ----> STM32 HAL I2C
      |
      +-----------> UART log
      +-----------> PD12 status LED
```

```text
Core/Inc/main.h             Board-level declarations
Core/Inc/app.h              Application interface
Core/Src/main.c             Clock, peripherals, event loop
Core/Src/app.c              Scheduling, logging, recovery policy
Core/Src/stm32f4xx_hal_msp.c GPIO alternate functions and TIM2 IRQ
Drivers/SHT31/sht31.[ch]     Reusable sensor driver
```

## Build and flash with STM32CubeIDE

This repository contains the application-owned source files. STM32CubeIDE supplies the device startup file, CMSIS, and STM32F4 HAL for the selected MCU.

1. Create a new STM32 project for **STM32F407VGT6** (or select the STM32F4-Discovery board).
2. Enable I2C1 on PB6/PB7, USART2 asynchronous on PA2/PA3, TIM2, and PD12 output. Select HSE and configure the system clock for 168 MHz.
3. Copy `Core` and `Drivers/SHT31` into the project. Add `Drivers/SHT31` to the compiler include paths.
4. Ensure these HAL modules are enabled in `stm32f4xx_hal_conf.h`: GPIO, RCC, CORTEX, I2C, UART, TIM, PWR, and FLASH.
5. Enable float formatting for `printf`: project properties → MCU Settings → **Use float with printf from newlib-nano**, or add linker option `-u _printf_float`.
6. Build, flash through ST-LINK, and open a serial terminal at **115200 8-N-1**.

The provided clock setup assumes an 8 MHz HSE. Update PLL parameters if your board uses a different crystal. If CubeMX generates the project skeleton, keep its startup/linker/CMSIS files and use the peripheral settings and application modules from this repository.

## Expected output

```text
STM32F407 humidity monitor starting
SHT31 ready at 7-bit address 0x44
[1] temperature=23.61 C, humidity=44.27 %RH
[2] temperature=23.62 C, humidity=44.31 %RH
```

On a failed transaction, the firmware reports a specific reason such as `communication timeout`, `crc mismatch`, or `measurement out of range`. After three consecutive failures, it issues the SHT31 soft-reset command and reports the recovery result.

## Design notes

- The ISR only sets a `volatile` flag; I²C and UART calls remain in thread context.
- The SHT31 uses a 7-bit address, while STM32 HAL expects it shifted left by one.
- High-repeatability single-shot mode uses command `0x2400` and a 16 ms conversion delay.
- CRC uses polynomial `0x31` and initialization value `0xFF`, per the sensor datasheet.
- UART logging is intentionally blocking for simple bring-up. A DMA/ring-buffer logger is a natural production extension.

## License

MIT — see [LICENSE](LICENSE).
