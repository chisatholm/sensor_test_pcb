/*
 * sht45.h
 *
 *  Created on: 2 Jul 2026
 *      Author: andrewchisholm
 */

/**
 ******************************************************************************
 * @file           : sht45.h
 * @brief          : Header for sht45.c module
 ******************************************************************************
 */

#ifndef __SHT45_H
#define __SHT45_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Exported Constants --------------------------------------------------------*/
#define SHT45_I2C_ADDR_7BIT       0x44
#define SHT45_I2C_ADDRESS         (SHT45_I2C_ADDR_7BIT << 1)

/* SHT45 Commands */
#define SHT45_CMD_MEASURE_HIGH    0xFD  // High precision, no clock stretching

/* Exported Types ------------------------------------------------------------*/
typedef enum {
    SHT45_OK      = 0x00,
    SHT45_ERROR   = 0x01
} SHT45_Status_t;

/* Exported Functions Prototypes ---------------------------------------------*/

/**
  * @brief  Triggers a measurement and reads temperature and relative humidity.
  * @param  hi2c: Pointer to an I2C_HandleTypeDef structure.
  * @param  temperature: Pointer to float for Celsius result.
  * @param  humidity: Pointer to float for %RH result.
  * @retval SHT45 Status
  */
SHT45_Status_t SHT45_Read_Data(I2C_HandleTypeDef *hi2c, float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif

#endif /* __SHT45_H */
