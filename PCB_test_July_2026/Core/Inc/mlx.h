/*
 * mlx.h
 *
 *  Created on: 2 Jul 2026
 *      Author: andrewchisholm
 */

/**
 ******************************************************************************
 * @file           : mlx90632.h
 * @brief          : Header for mlx90632 connection test
 ******************************************************************************
 */

#ifndef __MLX90632_H
#define __MLX90632_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Exported Constants --------------------------------------------------------*/
#define MLX90632_I2C_ADDR_7BIT    0x3A
#define MLX90632_I2C_ADDRESS      (MLX90632_I2C_ADDR_7BIT << 1)

/* 16-bit Register Addresses */
#define MLX90632_REG_EE_VERSION   0x2405

typedef enum {
    MLX90632_OK    = 0x00,
    MLX90632_ERROR = 0x01
} MLX90632_Status_t;

/* Exported Functions Prototypes ---------------------------------------------*/

/**
  * @brief  Pings the MLX90632 to see if it responds with an ACK.
  * @param  hi2c: Pointer to an I2C_HandleTypeDef structure.
  * @retval MLX90632 Status
  */
MLX90632_Status_t MLX90632_Ping(I2C_HandleTypeDef *hi2c);

#ifdef __cplusplus
}
#endif

#endif /* __MLX90632_H */
