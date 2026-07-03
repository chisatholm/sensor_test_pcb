/*
 * bmp585_hardware.h
 *
 *  Created on: 3 Jul 2026
 *      Author: andrewchisholm
 */

#ifndef BMP585_HARDWARE_H_
#define BMP585_HARDWARE_H_

#include "stm32l4xx_hal.h" // Adjust if you are using a different STM32 series (e.g., L4, G4, etc.)
#include "bmp5.h"

/* * The Bosch API requires us to declare a central device tracking object.
 * We use 'extern' here so your main.c file can easily see it later.
 */
extern struct bmp5_dev bmp585_device;

/*
 * Mandatory Interface Functions matching the Bosch API callback signatures
 */
BMP5_INTF_RET_TYPE bmp5_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
BMP5_INTF_RET_TYPE bmp5_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
void bmp5_delay_us(uint32_t period, void *intf_ptr);

/*
 * High-Level Application Control Functions
 */
int8_t BMP585_Hardware_Init(void);
int8_t BMP585_Get_Data(float *temperature_c, float *pressure_hpa);

#endif /* BMP585_HARDWARE_H_ */
