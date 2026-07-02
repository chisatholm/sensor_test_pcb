/**
 ******************************************************************************
 * @file           : mlx90632.c
 * @brief          : Source for mlx90632 connection test
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "mlx.h"

/* Exported Functions --------------------------------------------------------*/

MLX90632_Status_t MLX90632_Ping(I2C_HandleTypeDef *hi2c)
{
    uint8_t buffer[2] = {0};
    HAL_StatusTypeDef hal_status;

    // Read 2 bytes from the 16-bit EE_VERSION register address
    hal_status = HAL_I2C_Mem_Read(hi2c,
                                  MLX90632_I2C_ADDRESS,
                                  MLX90632_REG_EE_VERSION,
                                  I2C_MEMADD_SIZE_16BIT,
                                  buffer,
                                  2,
                                  100);

    if (hal_status == HAL_OK)
    {
        return MLX90632_OK;
    }
    else
    {
        return MLX90632_ERROR;
    }
}
