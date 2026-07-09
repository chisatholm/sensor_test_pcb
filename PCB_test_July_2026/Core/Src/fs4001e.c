/*
 * mfm_fs_4001e.c
 *
 * Created on: 9 Jul 2026
 * Author: Andrew
 */
#include "fs4001e.h"

void FS4001E_Life_Sign(I2C_HandleTypeDef *hi2c)
{
    uint8_t rx_buf[12] = {0};

    /* * Uses HAL_I2C_Mem_Read to issue:
     * [Start] -> [Addr+W] -> [0x00] -> [0x30] -> [Repeated Start] -> [Addr+R] -> [Read 12 bytes] -> [Stop]
     */
    HAL_I2C_Mem_Read(hi2c,
                     (FS4001E_I2C_ADDR_7BIT << 1),
                     0x0030,
                     I2C_MEMADD_SIZE_16BIT,
                     rx_buf,
                     12,
                     100);
}

float FS4001E_Flow_Read(I2C_HandleTypeDef *hi2c)
{
    uint8_t rx_buf[4] = {0};

    /* * Uses HAL_I2C_Mem_Read to issue:
     * [Start] -> [Addr+W] -> [0x00] -> [0x3A] -> [Repeated Start] -> [Addr+R] -> [Read 4 bytes] -> [Stop]
     */
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(hi2c,
                                                (FS4001E_I2C_ADDR_7BIT << 1),
                                                0x003A,
                                                I2C_MEMADD_SIZE_16BIT,
                                                rx_buf,
                                                4,
                                                100);

    if (status != HAL_OK)
    {
        return -9999.0f; // Clear indication of communication breakdown
    }

    // Reconstruct the 32-bit big-endian register payload
    int32_t raw_flow = (rx_buf[0] << 24) | (rx_buf[1] << 16) | (rx_buf[2] << 8) | rx_buf[3];
    float actual_flow_sccm = (float)raw_flow / 1000.0f;

    return actual_flow_sccm;
}
