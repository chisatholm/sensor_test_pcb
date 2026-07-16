/*
 * mfm_fs_4001e.c
 *
 * Created on: 9 Jul 2026
 * Author: Andrew
 */
#include "fs4001e.h"

extern char MFM_serial_number[11];


/**
  * @brief  Reads the raw high and low 16-bit registers for the flow rate.
  * @param  hi2c: Pointer to the STM32 I2C handle.
  * @param  out_val_high: Pointer to store the high 16-bit word (from 0x003A)
  * @param  out_val_low: Pointer to store the low 16-bit word (from 0x003B)
  * @return HAL status of the transaction.
  */
HAL_StatusTypeDef FS4001E_Read_Flow_Raw(I2C_HandleTypeDef *hi2c, uint16_t *out_val_high, uint16_t *out_val_low)
{
    uint16_t dev_addr = (FS4001E_I2C_ADDR_7BIT << 1); // 0x02
    uint8_t reg_addr[2] = {0x00, 0x3A};               // Base register address

    // We read 6 bytes: [High_MSB, High_LSB, CRC_High, Low_MSB, Low_LSB, CRC_Low]
    uint8_t rx_buf[6] = {0};

    // 1. Set the register pointer
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, dev_addr, reg_addr, 2, 100);
    if (status != HAL_OK) return status;

    // Use the 1ms timing delay
    HAL_Delay(1);

    // 2. Read the 6-byte chunk
    status = HAL_I2C_Master_Receive(hi2c, dev_addr, rx_buf, 6, 100);
    if (status == HAL_OK)
    {
        // Reconstruct the 16-bit words while skipping the CRC bytes at index 2 and 5
        *out_val_high = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
        *out_val_low  = ((uint16_t)rx_buf[3] << 8) | rx_buf[4];
    }

    return status;
}

float FS4001E_Get_Flow_Rate(I2C_HandleTypeDef *hi2c)
{
    uint16_t val_high = 0;
    uint16_t val_low = 0;

    if (FS4001E_Read_Flow_Raw(hi2c, &val_high, &val_low) == HAL_OK)
    {
        // Recombine the words into a 32-bit unsigned integer
        uint32_t raw_flow = ((uint32_t)val_high << 16) | val_low;

        // Scale by 1000 to return sccm as specified by datasheet
        return (float)raw_flow / 1000.0f;
    }

    return -1.0f; // Return an error state
}

/**
  * @brief  Reads the first 4 characters of the serial number, stripping the CRC bytes.
  * @param  hi2c: Pointer to the STM32 I2C handle
  * @param  out_str: Pointer to a 5-byte char array to hold the null-terminated string (e.g. "B3V1")
  */
void FS4001E_Read_SN(I2C_HandleTypeDef *hi2c, char *out_str)
{
	uint16_t dev_addr = (FS4001E_I2C_ADDR_7BIT << 1); // 0x02
	    uint8_t reg_addr[2] = {0x00, 0x30};               // Serial number base register (0x0030)

	    // We must read 18 bytes: 12 data bytes + 6 CRC bytes
	    uint8_t rx_buf[18] = {0};

	    // 1. Send register address pointer
	    HAL_I2C_Master_Transmit(hi2c, dev_addr, reg_addr, 2, 100);

	    // Keep the working 1ms timing delay
	    HAL_Delay(1);

	    // 2. Read the entire 18-byte block
	    if (HAL_I2C_Master_Receive(hi2c, dev_addr, rx_buf, 18, 100) == HAL_OK)
	    {
	        // Extract data bytes, skipping the CRC byte at every 3rd index (2, 5, 8, 11, 14, 17)
	        out_str[0] = (char)rx_buf[0];  // '*' (0x2A)
	        out_str[1] = (char)rx_buf[1];  // '*' (0x2A)
	        // rx_buf[2] is CRC-8 for [0, 1] - skip

	        out_str[2] = (char)rx_buf[3];  // 'B' (0x42)
	        out_str[3] = (char)rx_buf[4];  // '3' (0x33)
	        // rx_buf[5] is CRC-8 for [3, 4] - skip

	        out_str[4] = (char)rx_buf[6];  // 'V' (0x56)
	        out_str[5] = (char)rx_buf[7];  // '1' (0x31)
	        // rx_buf[8] is CRC-8 for [6, 7] - skip

	        out_str[6] = (char)rx_buf[9];  // '7' (0x37)
	        out_str[7] = (char)rx_buf[10]; // '0' (0x30)
	        // rx_buf[11] is CRC-8 for [9, 10] - skip

	        out_str[8] = (char)rx_buf[12]; // '0' (0x30)
	        out_str[9] = (char)rx_buf[13]; // '9' (0x39)
	        // rx_buf[14] is CRC-8 for [12, 13] - skip

	        // If the sensor transmits trailing padding (e.g. 0x2A), it will be captured here
	        // out_str[10] = (char)rx_buf[15];
	        // out_str[11] = (char)rx_buf[16];
	        // rx_buf[17] is CRC-8 for [15, 16] - skip

	        out_str[10] = '\0'; // Ensure safe string termination
	    }
}





void FS4001E_Read_Serial_First_Bytes(I2C_HandleTypeDef *hi2c)
{
    // 7-bit Address 0x01 shifted left -> Write: 0x02, Read: 0x03
    uint16_t dev_addr = (0x01 << 1);

    // Explicitly define the two bytes in Big-Endian order
    uint8_t reg_pointer[2] = {0x00, 0x30};

    // We want to read the first 2 data bytes + 1 CRC byte
    uint8_t rx_buffer[3] = {0, 0, 0};

    // Phase 1: Tell the sensor we want to read register 0x0030
    // This sends: [START] -> [0x02] -> [0x00] -> [0x30] -> [STOP]
    HAL_I2C_Master_Transmit(hi2c, dev_addr, reg_pointer, 2, 100);

    // Phase 2: Immediately read 3 bytes back from the sensor
    // This sends: [START] -> [0x03] -> reads 3 bytes -> [STOP]
    HAL_I2C_Master_Receive(hi2c, dev_addr, rx_buffer, 3, 100);
}





/**
  * @brief  Raw read of 2 data bytes + 1 CRC byte from the Serial Number register.
  * This is stripped of all logic to act purely as a signal generator.
  */
void FS4001E_Test_Raw_Read(I2C_HandleTypeDef *hi2c)
{
	uint16_t dev_addr = (FS4001E_I2C_ADDR_7BIT << 1); // 0x02
	//dev_addr = 0x34;
	    uint8_t reg_addr[2] = {0x00, 0x30};
	    uint8_t rx_buf[3] = {0};

	    // 1. Write command with a STOP condition (No Repeated Start)
	    HAL_I2C_Master_Transmit(hi2c, dev_addr, reg_addr, 2, 100);

	    // 2. Read immediately afterwards (No delay!)
	    HAL_I2C_Master_Receive(hi2c, dev_addr, rx_buf, 3, 100);
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


/**
  * @brief  Calculates the CRC-8 for 2 bytes of data as required by the FS4001E.
  * @param  data: Pointer to the array containing the 2 data bytes.
  * @return Calculated 8-bit CRC.
  */
uint8_t FS4001E_Calculate_CRC(uint8_t *data)
{
    uint8_t crc = 0x00; // Initialization is 0x00

    for (int i = 0; i < 2; i++) // Protects 2 data bytes per CRC chunk
    {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x07; // Polynomial is 0x07
            }
            else
            {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

uint8_t FS4001E_Life_Sign(I2C_HandleTypeDef *hi2c, uint8_t *out_ascii_12bytes)
{
    // 6 words means: [DataH, DataL, CRC] repeated 6 times = 18 bytes total
    uint8_t raw_buf[18] = {0};
    uint8_t cmd[2] = {0x00, 0x30}; // Serial number command
    uint16_t dev_addr = (0x01 << 1);

    // 1. Send the 16-bit command
    if (HAL_I2C_Master_Transmit(hi2c, dev_addr, cmd, 2, 100) == HAL_OK)
    {
        // 2. Read back the interleaved 18-byte stream
        if (HAL_I2C_Master_Receive(hi2c, dev_addr, raw_buf, 18, 100) == HAL_OK)
        {
            uint8_t ascii_idx = 0;
            uint8_t crc_errors = 0;

            // 3. Parse the interleaved blocks
            for (uint8_t i = 0; i < 18; i += 3)
            {
                uint8_t data_block[2] = { raw_buf[i], raw_buf[i+1] };
                uint8_t received_crc  = raw_buf[i+2];

                // Fixed variable name mismatch here
                uint8_t calculated_crc = FS4001E_Calculate_CRC(data_block);

                if (calculated_crc != received_crc) {
                    crc_errors++;
                } else {
                    // Extract data bytes if CRC matches
                    out_ascii_12bytes[ascii_idx++] = raw_buf[i];
                    out_ascii_12bytes[ascii_idx++] = raw_buf[i+1];
                }
            }

            // 4. Return status based on parsing verification
            if (crc_errors == 0) {
                out_ascii_12bytes[12] = '\0'; // Ensure proper string termination
                return 1; // Success! Data is ready in your passed buffer
            }
        }
    }

    return 0; // Failed I2C transfer or CRC mismatch
}
