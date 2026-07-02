/*
 * sht45.c
 *
 *  Created on: 2 Jul 2026
 *      Author: andrewchisholm
 */

/**
 ******************************************************************************
 * @file           : sht45.c
 * @brief          : Source for sht45.c module
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "sht45.h"

/* Exported Functions --------------------------------------------------------*/

SHT45_Status_t SHT45_Read_Data(I2C_HandleTypeDef *hi2c, float *temperature, float *humidity)
{
    uint8_t cmd = SHT45_CMD_MEASURE_HIGH;
    uint8_t buffer[6] = {0};

    // 1. Send the measurement command
    if (HAL_I2C_Master_Transmit(hi2c, SHT45_I2C_ADDRESS, &cmd, 1, 100) != HAL_OK)
    {
        return SHT45_ERROR;
    }

    // 2. Wait for the sensor to complete the measurement (Max ~8.3ms for SHT4x high precision)
    HAL_Delay(10);

    // 3. Read back 6 bytes of data
    if (HAL_I2C_Master_Receive(hi2c, SHT45_I2C_ADDRESS, buffer, 6, 100) != HAL_OK)
    {
        return SHT45_ERROR;
    }

    // 4. Reconstruct raw values
    uint16_t raw_temp = (buffer[0] << 8) | buffer[1];
    uint16_t raw_humid = (buffer[3] << 8) | buffer[4];

    // (Optional: You can add CRC checking on buffer[2] and buffer[5] here later)

    // 5. Convert raw data to physical units using datasheet formulas
    *temperature = -45.0f + 175.0f * ((float)raw_temp / 65535.0f);
    *humidity = -6.0f + 125.0f * ((float)raw_humid / 65535.0f);

    // Clamp humidity to realistic bounds
    if (*humidity < 0.0f) *humidity = 0.0f;
    if (*humidity > 100.0f) *humidity = 100.0f;

    return SHT45_OK;
}
