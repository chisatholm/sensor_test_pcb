/*
 * mfm_fs_4001e.h
 *
 *  Created on: 9 Jul 2026
 *      Author: Andrew
 */

#ifndef FS4001E_H
#define FS4001E_H

#include "stm32l4xx_hal.h"

/* Exported Constants --------------------------------------------------------*/
#define FS4001E_I2C_ADDR_7BIT      	0x01
#define FS4001E_I2C_SER_NUM_REG		0x0030
#define FS4001E_Flow_Rt_Reg      	0x3A




/* Exported Types ------------------------------------------------------------*/
typedef enum {
    FS4001E_OK      = 0x00,
    FS4001E_ERROR   = 0x01
} FS4001E_Status_t;

HAL_StatusTypeDef FS4001E_Read_Flow_Raw(I2C_HandleTypeDef *hi2c, uint16_t *out_val_high, uint16_t *out_val_low);

float FS4001E_Get_Flow_Rate(I2C_HandleTypeDef *hi2c);

void FS4001E_Read_SN(I2C_HandleTypeDef *hi2c, char *out_str);

void FS4001E_Read_Serial_First_Bytes(I2C_HandleTypeDef *hi2c);

uint8_t FS4001E_Life_Sign(I2C_HandleTypeDef *hi2c, uint8_t *out_ascii_12bytes);

float FS4001E_Flow_Read(I2C_HandleTypeDef *hi2c);

void FS4001E_Test_Raw_Read(I2C_HandleTypeDef *hi2c);


#endif /* FS4001E_H */
