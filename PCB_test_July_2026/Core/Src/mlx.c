/**
 ******************************************************************************
 * @file           : mlx.c
 * @brief          : Source for mlx90632 connection test
 ******************************************************************************
 */

/**
 ******************************************************************************
 * @file           : mlx.c
 * @brief          : Application source code for MLX90632 temperature tracking
 ******************************************************************************
 */

#include "mlx.h"
#include "mlx90632.h"
#include "mlx90632_depends.h" // <-- ADD THIS to expose mlx90632_i2c_read

/* Global structures or variables to cache the factory calibration constants */
static int32_t ee_P_R, ee_P_G, ee_P_T, ee_P_O;
static int32_t ee_Ea, ee_Eb, ee_Ga, ee_Fa, ee_Fb;
static int16_t ee_Gb, ee_Ka, ee_Ha, ee_Hb;

/* Internal helper function to read a 32-bit value from two 16-bit EEPROM registers */
static int32_t read_ee_32bit(uint16_t reg_addr) {
    uint16_t low = 0, high = 0;
    mlx90632_i2c_read(reg_addr, &low);
    mlx90632_i2c_read(reg_addr + 1, &high);
    return (int32_t)((high << 16) | low);
}

int32_t MLX_Application_Init(void)
{
    int32_t ret;

    // 1. Initialize the driver hardware state
    ret = mlx90632_init();
    if (ret < 0)
    {
        return ret;
    }

    // 2. Read and cache all factory calibration coefficients
    ee_P_R = read_ee_32bit(MLX90632_EE_P_R);
    ee_P_G = read_ee_32bit(MLX90632_EE_P_G);
    ee_P_T = read_ee_32bit(MLX90632_EE_P_T);
    ee_P_O = read_ee_32bit(MLX90632_EE_P_O);

    ee_Ea  = read_ee_32bit(MLX90632_EE_Ea);
    ee_Eb  = read_ee_32bit(MLX90632_EE_Eb);
    ee_Ga  = read_ee_32bit(MLX90632_EE_Ga);
    ee_Fa  = read_ee_32bit(MLX90632_EE_Fa);
    ee_Fb  = read_ee_32bit(MLX90632_EE_Fb);

    mlx90632_i2c_read(MLX90632_EE_Gb, (uint16_t *)&ee_Gb);
    mlx90632_i2c_read(MLX90632_EE_Ka, (uint16_t *)&ee_Ka);
    mlx90632_i2c_read(MLX90632_EE_Ha, (uint16_t *)&ee_Ha);
    mlx90632_i2c_read(MLX90632_EE_Hb, (uint16_t *)&ee_Hb);

    mlx90632_set_emissivity(1.0);

    return 0;
}

int32_t MLX_Application_Read_Temperature(float *p_leaf_temp)
{
    int32_t ret;

    int16_t ambient_new_raw = 0;
    int16_t ambient_old_raw = 0;
    int16_t object_new_raw = 0;
    int16_t object_old_raw = 0;

    double pre_ambient = 0.0;
    double pre_object = 0.0;
    double object_temp_milli_celsius = 0.0;

    // 1. Fetch raw ADC values from the sensor RAM registers
    ret = mlx90632_read_temp_raw(&ambient_new_raw, &ambient_old_raw,
                                 &object_new_raw, &object_old_raw);

    if (ret == -11) // -EAGAIN: Data not ready yet
    {
        return 1;
    }
    else if (ret < 0)
    {
        return ret;
    }

    // 2. Pre-process the raw ambient and raw object values
    pre_ambient = mlx90632_preprocess_temp_ambient(ambient_new_raw, ambient_old_raw, ee_Gb);
    pre_object  = mlx90632_preprocess_temp_object(object_new_raw, object_old_raw,
                                                  ambient_new_raw, ambient_old_raw, ee_Ka);

    // 3. Calculate Object (Leaf) Temperature in milliCelsius
    object_temp_milli_celsius = mlx90632_calc_temp_object((int32_t)pre_object, (int32_t)pre_ambient,
                                                          ee_Ea, ee_Eb, ee_Ga, ee_Fa, ee_Fb,
                                                          ee_Ha, ee_Hb);

    // 4. Convert milliCelsius from the driver to standard float Celsius for your application
    *p_leaf_temp = (float)(object_temp_milli_celsius / 1000.0);

    return 0;
}
