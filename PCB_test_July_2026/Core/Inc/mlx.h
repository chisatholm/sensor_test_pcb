/*
 * mlx.h
 *
 *  Created on: 2 Jul 2026
 *      Author: andrewchisholm
 */

/**
 ******************************************************************************
 * @file           : mlx90632.h
 * @brief          : Header for MLX90632SLD-DCB-000-SP connection test
 ******************************************************************************
 */


#ifndef __MLX_H
#define __MLX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Exported Functions Prototypes ---------------------------------------------*/

/**
  * @brief  Initializes the MLX90632 sensor by checking its connection,
  * reading calibration parameters, and starting the state machine.
  * @retval 0 on success, negative error code on failure.
  */
int32_t MLX_Application_Init(void);

/**
  * @brief  Polls the MLX90632 for a new data frame, extracts raw data,
  * and calculates both sensor and object temperatures.
  * @param  p_leaf_temp: Pointer to a float where the calculated object (leaf)
  * temperature in Celsius will be stored.
  * @retval 0 if a new temperature was calculated successfully,
  * 1 if no new data is available yet, negative on error.
  */
int32_t MLX_Application_Read_Temperature(float *p_leaf_temp);

#ifdef __cplusplus
}
#endif

#endif /* __MLX_H */
