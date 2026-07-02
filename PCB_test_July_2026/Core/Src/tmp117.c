/*
 * tmp117.c
 *
 *  Created on: 2 Jul 2026
 *      Author: andrewchisholm
 */
/**
 ******************************************************************************
 * @file           : tmp117.c
 * @brief          : Source for tmp117.c module for high-accuracy temperature
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "tmp117.h"

/* Private Functions ---------------------------------------------------------*/

/**
  * @brief  Reads a 16-bit register from the TMP117 (Handles Big-Endian swap)
  */
static TMP117_Status_t TMP117_Read_Register(I2C_HandleTypeDef *hi2c, uint8_t reg_addr, uint16_t *reg_value)
{
    uint8_t buffer[2] = {0};
    HAL_StatusTypeDef hal_status;

    // Read 2 bytes from the specified register address
    hal_status = HAL_I2C_Mem_Read(hi2c, TMP117_I2C_ADDRESS, reg_addr, I2C_MEMADD_SIZE_8BIT, buffer, 2, 100);

    if (hal_status != HAL_OK)
    {
        return TMP117_ERROR;
    }

    // TMP117 transmits MSB first, LSB second. Reconstruct the 16-bit word.
    *reg_value = (uint16_t)((buffer[0] << 8) | buffer[1]);

    return TMP117_OK;
}

/* Exported Functions --------------------------------------------------------*/

/**
  * @brief  Initializes the TMP117 sensor and verifies Device ID.
  */
TMP117_Status_t TMP117_Init(I2C_HandleTypeDef *hi2c)
{
    uint16_t device_id = 0;

    // Read the Device ID register to ensure wiring and address are correct
    if (TMP117_Read_Register(hi2c, TMP117_REG_DEVICE_ID, &device_id) != TMP117_OK)
    {
        return TMP117_ERROR;
    }

    // Mask off the revision bits if necessary; standard TMP117 ID is 0x0117
    if ((device_id & 0x0FFF) != TMP117_DEVICE_ID_VALUE)
    {
        return TMP117_ERROR; // ID mismatch
    }

    return TMP117_OK;
}

/**
  * @brief  Reads the raw temperature register and converts it to Celsius.
  */
TMP117_Status_t TMP117_Read_Temperature(I2C_HandleTypeDef *hi2c, float *temperature)
{
    uint16_t raw_reg_value = 0;

    if (TMP117_Read_Register(hi2c, TMP117_REG_TEMP_RESULT, &raw_reg_value) != TMP117_OK)
    {
        return TMP117_ERROR;
    }

    // Convert the unsigned 16-bit register to a signed 16-bit integer
    // This properly handles negative numbers using 2's complement.
    int16_t signed_raw = (int16_t)raw_reg_value;

    // Multiply by the resolution factor (0.0078125 C per LSB)
    *temperature = (float)signed_raw * TMP117_RESOLUTION;

    return TMP117_OK;
}

