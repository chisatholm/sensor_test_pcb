/*
 * bmp585_hardware.c
 *
 * Created on: 3 Jul 2026
 * Author: andrewchisholm
 */
#include "bmp585_hardware.h"
#include "bmp5.h"
#include "main.h"

/* Reference the auto-generated hardware handle from main.c */
extern SPI_HandleTypeDef hspi1;

/* Allocate the global device structure tracking context required by the Bosch API */
struct bmp5_dev bmp585_device;
extern int16_t debug_xii;

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

/**
 * @brief Bosch API compliant SPI Read function
 */
BMP5_INTF_RET_TYPE bmp5_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    // The Bosch API handles the Read bit (0x80) internally, but let's guarantee it here
    uint8_t tx_buf[2] = { reg_addr | 0x80, 0x00 };
    HAL_StatusTypeDef status;

    BMP585_CS_Select();

    // Transmit the register address + dummy byte (2 bytes)
    status = HAL_SPI_Transmit(&hspi1, tx_buf, 2, HAL_MAX_DELAY);

    if (status == HAL_OK) {
        // Clock out the data bytes directly into the Bosch data pointer
        status = HAL_SPI_Receive(&hspi1, reg_data, len, HAL_MAX_DELAY);
    }

    BMP585_CS_Deselect();

    return (status == HAL_OK) ? BMP5_OK : BMP5_E_COM_FAIL;
}

/**
 * @brief Bosch API compliant SPI Write function
 */
BMP5_INTF_RET_TYPE bmp5_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t tx_addr = reg_addr & 0x7F; // Ensure write bit is cleared
    HAL_StatusTypeDef status;

    BMP585_CS_Select();

    // Transmit the target register address
    status = HAL_SPI_Transmit(&hspi1, &tx_addr, 1, HAL_MAX_DELAY);

    if (status == HAL_OK) {
        // Transmit the data payload directly from the Bosch data pointer
        status = HAL_SPI_Transmit(&hspi1, (uint8_t*)reg_data, len, HAL_MAX_DELAY);
    }

    BMP585_CS_Deselect();

    return (status == HAL_OK) ? BMP5_OK : BMP5_E_COM_FAIL;
}

/**
 * @brief Bosch API compliant microsecond delay function
 */
void bmp5_delay_us(uint32_t period, void *intf_ptr)
{
    // Convert microseconds to milliseconds for standard HAL_Delay.
    // Add 1 to round up and prevent 0ms delays.
    uint32_t ms = (period / 1000) + 1;
    HAL_Delay(ms);
}

/*
 * High-Level Application Control Functions
 */

/**
 * @brief Initializes and configures the BMP585 sensor using the Bosch API
 * @param[out] dev: Pointer to a bmp5_dev instance that will be configured
 * @return BMP5_OK on success, or an error code on failure
 */
int8_t BMP585_Hardware_Init(struct bmp5_dev *dev)
{
    int8_t rslt = BMP5_OK;
    struct bmp5_osr_odr_press_config osr_odr_press_cfg = {0};

    if (dev == NULL) {
        return BMP5_E_NULL_PTR;
    }

    // =====================================================================
	// SPI BUS SYNCHRONIZATION (The "Dummy Wake-up")
	// The very first transaction after a reset is used by the sensor core
	// to switch interfaces and synchronize. We issue it here so it doesn't
	// break the subsequent bmp5_init() call.
	// =====================================================================
	uint8_t dummy_tx[2] = { 0x80, 0x00 };
	uint8_t dummy_rx[2] = { 0x00, 0x00 };

	HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi1, dummy_tx, dummy_rx, 2, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);

	HAL_Delay(5); // Small settling gap before starting official API communications

    // 1. Assign your working interface functions to the API pointers
    dev->read = bmp5_spi_read;
    dev->write = bmp5_spi_write;
    dev->delay_us = bmp5_delay_us;

    // 2. Define the communication interface protocol type
    dev->intf = BMP5_SPI_INTF;

    // 3. Optional pointer for an interface handle (not strictly needed for your simple setup)
    dev->intf_ptr = NULL;

    // 4. Run the core Bosch initialization API.
    // This automatically verifies the Chip ID on the wire!
    rslt = bmp5_init(dev);
    if (rslt != BMP5_OK) {
        return rslt;
    }

    // 5. Retrieve default register structures to overwrite parameters cleanly
    rslt = bmp5_get_osr_odr_press_config(&osr_odr_press_cfg, dev);
    if (rslt != BMP5_OK) {
        return rslt;
    }

    // 6. Define OSR configurations matching your confirmed '0x52' structure
    osr_odr_press_cfg.osr_p = BMP5_OVERSAMPLING_8X;   // Pressure OSR x8
    osr_odr_press_cfg.osr_t = BMP5_OVERSAMPLING_1X;   // Temperature OSR x1
    osr_odr_press_cfg.odr = BMP5_ODR_10_HZ;           // Output Data Rate 10Hz
    osr_odr_press_cfg.press_en = BMP5_ENABLE;         // Explicitly enable pressure processing

    // 7. Write data structures into configuration registers
    rslt = bmp5_set_osr_odr_press_config(&osr_odr_press_cfg, dev);
    if (rslt != BMP5_OK) {
        return rslt;
    }

    // 8. Place sensor inside continuous Normal Mode sampling execution loop
    rslt = bmp5_set_power_mode(BMP5_POWERMODE_NORMAL, dev);

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
