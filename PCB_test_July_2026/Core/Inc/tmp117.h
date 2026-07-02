/*
 * tmp117.h
 *
 *  Created on: 2 Jul 2026
 *      Author: andrewchisholm
 */
/**
 ******************************************************************************
 * @file           : tmp117.h
 * @brief          : Header for tmp117.c module for high-accuracy temperature
 ******************************************************************************
 */

#ifndef __TMP117_H
#define __TMP117_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Exported Constants --------------------------------------------------------*/
/**
 * @brief I2C Shifting Note: STM32 HAL requires 8-bit addresses (7-bit shifted left by 1)
 * Change TMP117_I2C_ADDR_7BIT if your ADD0 pin is not tied to GND.
 */
#define TMP117_I2C_ADDR_7BIT      0x48
#define TMP117_I2C_ADDRESS        (TMP117_I2C_ADDR_7BIT << 1)

/* Register Map */
#define TMP117_REG_TEMP_RESULT    0x00
#define TMP117_REG_CONFIGURATION  0x01
#define TMP117_REG_DEVICE_ID      0x0F

/* Device ID Constant */
#define TMP117_DEVICE_ID_VALUE    0x0117

/* Conversion Factor: 1 LSB = 0.0078125 degrees C */
#define TMP117_RESOLUTION         0.0078125f

/* Exported Types ------------------------------------------------------------*/
typedef enum {
    TMP117_OK      = 0x00,
    TMP117_ERROR   = 0x01,
    TMP117_TIMEOUT = 0x02
} TMP117_Status_t;

/* Exported Functions Prototypes ---------------------------------------------*/

/**
  * @brief  Initializes the TMP117 sensor and checks its device ID.
  * @param  hi2c: Pointer to a I2C_HandleTypeDef structure that contains
  * the configuration information for the specified I2C.
  * @retval TMP117 Status
  */
TMP117_Status_t TMP117_Init(I2C_HandleTypeDef *hi2c);

/**
  * @brief  Reads the raw temperature register and converts it to Celsius.
  * @param  hi2c: Pointer to a I2C_HandleTypeDef structure.
  * @param  temperature: Pointer to float where the result will be stored.
  * @retval TMP117 Status
  */
TMP117_Status_t TMP117_Read_Temperature(I2C_HandleTypeDef *hi2c, float *temperature);

#ifdef __cplusplus
}
#endif

#endif /* __TMP117_H */
