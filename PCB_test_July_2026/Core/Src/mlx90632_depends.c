/**
 ******************************************************************************
 * @file           : mlx90632_depends.c
 * @brief          : Concrete hardware dependency implementations for MLX90632
 ******************************************************************************
 */
#include <stdint.h>
#include "mlx90632_depends.h"
#include "main.h"

// Reference to your STM32 I2C peripheral handle declared in main.c
extern I2C_HandleTypeDef hi2c1;

// The standard 7-bit I2C address for the MLX90632 is usually 0x3A.
// In STM32 HAL, we must shift the address left by 1 bit: (0x3A << 1) = 0x74
#define MLX90632_I2C_ADDR   (0x3A << 1)

/**
 * @brief Platform-specific I2C read implementation
 */
int32_t mlx90632_i2c_read(int16_t register_address, uint16_t *value)
{
    uint8_t data[2];
    
    // HAL_I2C_Mem_Read automatically formats the 16-bit register address,
    // transmits it, issues a REPEATED START, and reads the resulting data bytes.
    if (HAL_I2C_Mem_Read(&hi2c1,
                         MLX90632_I2C_ADDR,
                         (uint16_t)register_address,
                         I2C_MEMADD_SIZE_16BIT,
                         data,
                         2,
                         HAL_MAX_DELAY) != HAL_OK)
    {
        return -1; // Communication error
    }
    
    // Reconstruct the 16-bit word (Big-Endian format)
    *value = (uint16_t)((data[0] << 8) | data[1]);
    
    return 0; // Success
}

/**
 * @brief Platform-specific I2C write implementation
 */
int32_t mlx90632_i2c_write(int16_t register_address, uint16_t value)
{
    uint8_t data[4];
    
    // Register address (MSB first)
    data[0] = (uint8_t)(register_address >> 8);
    data[1] = (uint8_t)(register_address & 0xFF);
    
    // Data word to write (MSB first)
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)(value & 0xFF);
    
    if (HAL_I2C_Master_Transmit(&hi2c1, MLX90632_I2C_ADDR, data, 4, HAL_MAX_DELAY) != HAL_OK)
    {
        return -1; // Communication error
    }
    
    return 0; // Success
}

/**
 * @brief Platform-specific microsecond sleep wrapper
 */
void usleep(int min_range, int max_range)
{
    // Convert microseconds target to milliseconds for basic HAL compatibility.
    // An address reset wait requires ~150us; 1ms safely covers this bounds condition.
    uint32_t ms = min_range / 1000;
    if (ms == 0) 
    {
        ms = 1;
    }
    HAL_Delay(ms);
}
