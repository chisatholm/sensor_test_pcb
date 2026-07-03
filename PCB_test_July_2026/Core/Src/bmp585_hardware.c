/*
 * bmp585_hardware.c
 *
 *  Created on: 3 Jul 2026
 *      Author: andrewchisholm
 */
#include "bmp585_hardware.h"

/* Reference the auto-generated hardware handle from main.c */
extern SPI_HandleTypeDef hspi1;

/* Pin configuration constants based on your schematic assignments */
#define BMP585_CS_PORT  GPIOA
#define BMP585_CS_PIN   GPIO_PIN_8

/* Allocate the global device structure tracking context required by the Bosch API */
struct bmp5_dev bmp585_device;

/*
 * Private Helper Functions for Software Chip Select Control
 */
static void BMP585_CS_Select(void)
{
    HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
}

static void BMP585_CS_Deselect(void)
{
    /* Pitfall Prevention: At 312kHz, wait until the SPI hardware FIFO shifts out
     * the final bits completely before pulling the physical CSn line high. */
    while (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY) == SET);
    HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);
}

/*
 * Mandatory Interface Callback Functions for the Bosch API
 */

/**
 * @brief SPI read interface callback matching Bosch API requirements.
 * Handles the 0x80 read bit mask and clocks the mandatory dummy byte frame.
 */
BMP5_INTF_RET_TYPE bmp5_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    HAL_StatusTypeDef status;

    // Allocate local buffers to handle the full length of the transaction:
    // 1 Byte (Address) + 1 Byte (Dummy) + 'len' Bytes (Data payload)
    uint8_t tx_buf[32] = { 0 };
    uint8_t rx_buf[32] = { 0 };

    // Safety check to ensure we don't overrun our local stack arrays
    if ((len + 2) > 32) {
        return BMP5_E_COM_FAIL;
    }

    // Prepare the transmission stream:
    // Byte 0: Target address with the read bit (Bit 7) set high
    tx_buf[0] = reg_addr | 0x80;
    // Byte 1: 0x00 (The dummy clock frame)
    // Bytes 2 onwards: Left at 0x00 to provide clean clock pulses for reading data

    // Pull CS low once to frame the entire combined sequence
    BMP585_CS_Select();

    // Clock out everything simultaneously in one continuous hardware transaction frame
    status = HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, (uint16_t)(len + 2), HAL_MAX_DELAY);

    // Release CS back high now that the entire operation is physically complete
    BMP585_CS_Deselect();

    if (status == HAL_OK) {
        // Unpack the results:
        // rx_buf[0] and rx_buf[1] contain the trash data captured during the address/dummy clocks.
        // The real register payload bytes start precisely at index 2.
        for (uint32_t i = 0; i < len; i++) {
            reg_data[i] = rx_buf[i + 2];
        }
    }

    return (status == HAL_OK) ? BMP5_OK : BMP5_E_COM_FAIL;
}

/**
 * @brief SPI write interface callback matching Bosch API requirements.
 */
BMP5_INTF_RET_TYPE bmp5_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    HAL_StatusTypeDef status;

    /* SPI Write Protocol: Ensure MSB (Bit 7) is clear (0) */
    uint8_t tx_addr = reg_addr & 0x7F;

    BMP585_CS_Select();

    /* Step 1: Deliver target write register address */
    status = HAL_SPI_Transmit(&hspi1, &tx_addr, 1, HAL_MAX_DELAY);
    if (status == HAL_OK) {
        /* Step 2: Push configuration values down to the chip */
        status = HAL_SPI_Transmit(&hspi1, (uint8_t*)reg_data, (uint16_t)len, HAL_MAX_DELAY);
    }

    BMP585_CS_Deselect();

    return (status == HAL_OK) ? BMP5_OK : BMP5_E_COM_FAIL;
}

/**
 * @brief Time-delay interface callback matching Bosch API microsecond units.
 */
void bmp5_delay_us(uint32_t period, void *intf_ptr)
{
    /* Convert microseconds to system millisecond ticks */
    uint32_t ms = period / 1000;
    if (ms == 0 && period > 0) {
        ms = 1; /* Clamp to a minimum of 1ms delay for low-bound parameters */
    }
    HAL_Delay(ms);
}

/*
 * High-Level Application Control Functions
 */

/**
 * @brief Binds callbacks, verifies CHIP_ID communication, and boots the sensor.
 */
/**
 * @brief Binds callbacks, wakes up 4-wire SPI interface, verifies CHIP_ID, and boots the sensor.
 */
int8_t BMP585_Hardware_Init(void)
{
    int8_t rslt;
    struct bmp5_osr_odr_press_config osr_odr_press_cfg = {0};

    /* === WORKAROUND: Force BMP585 out of I2C mode into 4-Wire SPI Mode === */
    uint8_t dummy_wake = 0x00;
    BMP585_CS_Select();
    HAL_SPI_Transmit(&hspi1, &dummy_wake, 1, HAL_MAX_DELAY);
    BMP585_CS_Deselect();

    // Give the chip's internal digital multiplexer a brief moment to toggle states
    HAL_Delay(2);
    /* ===================================================================== */

    /* 1. Map physical hardware contexts to the Bosch device structure model */
    bmp585_device.intf = BMP5_SPI_INTF;
    bmp585_device.read = bmp5_spi_read;
    bmp585_device.write = bmp5_spi_write;
    bmp585_device.delay_us = bmp5_delay_us;
    bmp585_device.intf_ptr = &hspi1;

    /* 2. Run the internal API init sequence. This triggers a soft-reset and
     * automatically validates the CHIP_ID (0x50) to authenticate SPI health. */
    rslt = bmp5_init(&bmp585_device);
    if (rslt != BMP5_OK) {
        return rslt;
    }

    /* 3. Configure physical sampling setups (Oversampling & Data Output Frequency) */
    osr_odr_press_cfg.osr_p = BMP5_OVERSAMPLING_4X;   /* Pressure oversampling */
    osr_odr_press_cfg.osr_t = BMP5_OVERSAMPLING_1X;   /* Temperature oversampling */
    osr_odr_press_cfg.odr = BMP5_ODR_10_HZ;           /* Output data rate set to 10 Hz */
    osr_odr_press_cfg.press_en = BMP5_ENABLE;         /* Explicitly enable pressure sensing */

    rslt = bmp5_set_osr_odr_press_config(&osr_odr_press_cfg, &bmp585_device);
    if (rslt != BMP5_OK) {
        return rslt;
    }

    /* 4. Switch from STANDBY to NORMAL mode to begin ongoing sampling cycles */
    rslt = bmp5_set_power_mode(BMP5_POWERMODE_NORMAL, &bmp585_device);

    return rslt;
}


/**
 * @brief Extracts fresh uncompensated metrics and translates them into physical units.
 */
int8_t BMP585_Get_Data(float *temperature_c, float *pressure_hpa)
{
    int8_t rslt;
    struct bmp5_sensor_data data = {0};
    struct bmp5_osr_odr_press_config osr_odr_press_cfg = {0};

    /* 1. Retrieve the active ODR/OSR register configurations from the chip
     * so the API knows whether pressure_en is enabled */
    rslt = bmp5_get_osr_odr_press_config(&osr_odr_press_cfg, &bmp585_device);
    if (rslt != BMP5_OK) {
        return rslt;
    }

    /* 2. Call the function with all THREE arguments */
    rslt = bmp5_get_sensor_data(&data, &osr_odr_press_cfg, &bmp585_device);
    if (rslt == BMP5_OK) {
        *temperature_c = (float)data.temperature;

        /* Bosch API outputs pressure in standard Pascals. Scale by 100 for hPa */
        *pressure_hpa = (float)data.pressure / 100.0f;
    }

    return rslt;
}
