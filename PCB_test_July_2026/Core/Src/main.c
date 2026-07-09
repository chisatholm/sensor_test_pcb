/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tmp117.h"
#include "sht45.h"
#include "mlx.h"
#include "bmp585_hardware.h"
#include "fs4001e.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//#define BMP5_REG_CHIP_ID      0x01
//#define BMP5_REG_INT_STATUS   0x27
//#define BMP5_REG_STATUS       0x28
//#define BMP5_REG_CMD          0x7E

#define BMP5_CMD_SOFT_RESET   0xB6
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
float current_temperature = 0.0f; 	// for TMP117
float temp = 0.0f; 					// for SHT45
float hum = 0.0f; 				// for SHT45
float hum_arr[100][2] = {{0}, {0}};
uint16_t hum_arr_head = 0;
float mov_av_h = 0;
float mov_av_t = 0;

float leaf_temperature_celsius = 0.0f;

float bmp_temperature_c = 0.0f;
float bmp_pressure_hpa = 0.0f;

volatile uint8_t test_chip_id    = 0;
volatile uint8_t test_status     = 0;
volatile uint8_t test_int_status = 0;
HAL_StatusTypeDef spi_err;


volatile uint32_t raw_pressure = 0;
volatile int32_t  raw_temperature = 0;

volatile float BMP585_temp = 0;
volatile float BMP585_pressure = 0;

volatile float MFM_flow = 123.4;


uint8_t tx_data[8] = {0}; // 1 Address byte + 1 Dummy byte + 6 Data bytes
uint8_t rx_data[8] = {0};

uint16_t debug_x = 0;
int16_t debug_xi = 0;
int16_t debug_xii =0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
//int8_t BMP585_AJC_Init(struct bmp5_dev *dev);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  // API based BMP585 initialisation sequence
  int8_t init_status = BMP585_Hardware_Init(&bmp585_device);

  if(0){
  if (init_status == BMP5_OK)
  {
      test_chip_id = bmp585_device.chip_id; // Catches 0x51
  }
  else
  {
      test_chip_id = 0xEE;
      while(1); // Trap for debugging
  }
  }

  // hand-made code to initialise BMP585
  if (0){
  /* --- STEP 1: POWER-UP STABILIZATION --- */
HAL_Delay(5);

/* --- STEP 2: INTERFACE SELECTION WORKAROUND --- */
// Force sensor out of I2C mode into 4-wire SPI mode via a dummy CS toggle/write
uint8_t dummy_byte = 0x00;
  HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, &dummy_byte, 1, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);

  HAL_Delay(5);

  /* --- STEP 3: NATIVE HARDWARE SOFT-RESET --- */
    // Direct register write to completely refresh the chip from its stuck state
    uint8_t tx_reset[2];
    tx_reset[0] = BMP5_REG_CMD & 0x7F; // Clear Bit 7 for Write operations
    tx_reset[1] = BMP5_CMD_SOFT_RESET;

    HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
    spi_err = HAL_SPI_Transmit(&hspi1, tx_reset, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);

    HAL_Delay(3);

    /* --- STEP 4: READ BACK THE POST-POWER-UP CRITERIA --- */
      uint8_t tx_buf[3];
      uint8_t rx_buf[3];

      // =====================================================================
	  // NEW: POST-RESET DUMMY WAKE-UP READ
	  // The first transaction after a soft-reset is swallowed by the sensor
	  // to synchronize the SPI interface. We perform a quick dummy read here.
	  // =====================================================================
	  tx_buf[0] = 0x80; // Dummy read address
	  tx_buf[1] = 0x00;
	  HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, 2, HAL_MAX_DELAY);
	  HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);

	  HAL_Delay(1); // Tiny gap to let the sensor serial interface settle

      // 1. Read CHIP_ID (Register 0x01)
      tx_buf[0] = BMP5_REG_CHIP_ID | 0x80; // Set Bit 7 for Read operations
      tx_buf[1] = 0x00;                    // Dummy Byte
      tx_buf[2] = 0x00;

      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
      HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, 2, HAL_MAX_DELAY);
      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);
      test_chip_id = rx_buf[1];            // Extract data after dummy byte

      // 2. Read STATUS (Register 0x28)
      tx_buf[0] = BMP5_REG_STATUS | 0x80;
      tx_buf[1] = 0x00;
      tx_buf[2] = 0x00;

      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
      HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, 2, HAL_MAX_DELAY);
      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);
      test_status = rx_buf[1];

      // 3. Read INT_STATUS (Register 0x27)
      tx_buf[0] = BMP5_REG_INT_STATUS | 0x80;
      tx_buf[1] = 0x00;
      tx_buf[2] = 0x00;

      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
      HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, 2, HAL_MAX_DELAY);
      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);
      test_int_status = rx_buf[1];

      /* --- STEP 5: SENSOR CONFIGURATION --- */
      uint8_t tx_cfg[2];

      // 1. Enable Pressure & Temperature Channels (ODR_CONFIG register 0x37)
      // We'll set ODR to 10Hz (0x03) and keep PWR_MODE in standby for now
      tx_cfg[0] = 0x37 & 0x7F; // Write mode
      tx_cfg[1] = 0x03; // 0x0 stndby, 0x1 normal, 0x2 forced, 0x3 non-stop
      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
      HAL_SPI_Transmit(&hspi1, tx_cfg, 2, HAL_MAX_DELAY);
      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);
      HAL_Delay(1);

      // 2. Set Oversampling (OSR_CONFIG register 0x36)
      // Standard resolution for both Temp and Press (0x02 << 3 | 0x02) -> 0x12
      tx_cfg[0] = 0x36 & 0x7F; // Write mode
      tx_cfg[1] = 0x52; // b7: reserved, b6: pressure on, b5-3: oversampling rate pressure, b2-0: OSR temp
      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
      HAL_SPI_Transmit(&hspi1, tx_cfg, 2, HAL_MAX_DELAY);
      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);
      HAL_Delay(1);

      // 3. Switch Power Mode to Normal Mode (ODR_CONFIG register 0x37, bits [1:0] = 0x02)
      // This starts continuous conversions. (10Hz ODR + Normal Mode -> 0x03 | 0x02 = 0x05)
      tx_cfg[0] = 0x37 & 0x7F; // Write mode
      tx_cfg[1] = 0x5D;
      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
      HAL_SPI_Transmit(&hspi1, tx_cfg, 2, HAL_MAX_DELAY);
      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);
      HAL_Delay(10); // Let the first conversion cycle finish
  }


  if (0){
  if (MLX_Application_Init() != 0)
    {
        Error_Handler();
    }
    HAL_Delay(100);
  }


  /* USER CODE END 2 */




  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // inconsequential change to test ability to commit
	  // and another
	  //if (TMP117_Read_Temperature(&hi2c1, &current_temperature) == TMP117_OK)
	  //{
	  //    // successfully read into current_temperature
	  //}

/*
	SHT45_Read_Data(&hi2c1, &temp, &hum );
	hum_arr[hum_arr_head][0] = hum;
	hum_arr[hum_arr_head][1] = temp;
	if (hum_arr_head == 99){
		float dummy_h = 0;
		float dummy_t = 0;
		for (int i = 0; i < 100; i++){
			dummy_h += hum_arr[i][0];
			dummy_t += hum_arr[i][1];
		}
		dummy_h = dummy_h / 100;
		dummy_t = dummy_t / 100;
		mov_av_h = dummy_h;
		mov_av_t = dummy_t;
	}

	hum_arr_head ++;
	hum_arr_head = hum_arr_head % 100;
*/


	  /*
	int32_t mlx_status = MLX_Application_Read_Temperature(&leaf_temperature_celsius);
		if (mlx_status == 0)
		{
			// A fresh, valid object temperature reading is ready in leaf_temperature_celsius!
			// You can place a breakpoint or a watch expression here to inspect the value.
		}
		else if (mlx_status < 0)
		{
			// Optional: Handle I2C bus error states here
		}

	HAL_Delay(5);
*/
if(0){
	  // Start burst read at TEMP_XLSB (Register 0x1D)
	      // This will stream out: TEMP_XLSB, TEMP_LSB, TEMP_MSB, PRESS_XLSB, PRESS_LSB, PRESS_MSB
	      tx_data[0] = 0x1D | 0x80; // Read mode

	      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_RESET);
	      // Transmit address/dummy and clock out all 6 data bytes in one single CS window
	      HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 8, HAL_MAX_DELAY);
	      HAL_GPIO_WritePin(BMP585_CS_PORT, BMP585_CS_PIN, GPIO_PIN_SET);

	      // Reconstruction based on data indexing (rx_data[0]=SPI Status, rx_data[1]=Dummy)
	      // Temperature: 24-bit signed integer
	      raw_temperature = (int32_t)((uint32_t)rx_data[3] << 16 |
	                                  (uint32_t)rx_data[2] << 8  |
	                                  (uint32_t)rx_data[1]);

	      // Sign-extend 24-bit to 32-bit if negative
	      if (raw_temperature & 0x00800000) {
	          raw_temperature |= 0xFF000000;
	      }

	      BMP585_temp = raw_temperature / 65536.0f;


	      // Pressure: 24-bit unsigned integer
	      raw_pressure = (uint32_t)((uint32_t)rx_data[6] << 16 |
	                                (uint32_t)rx_data[5] << 8  |
	                                (uint32_t)rx_data[4]);

	      BMP585_pressure = raw_pressure / 64;


	      HAL_Delay(100); // Sample at roughly 10Hz}


  }

	if(1){

	    	  FS4001E_Life_Sign(&hi2c1);
	    	  HAL_Delay(1);
	    	  MFM_flow = FS4001E_Flow_Read(&hi2c1);
	    	  HAL_Delay(30);
	      }
  /* USER CODE END 3 */
}
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10D19CE4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SMPS_EN_Pin|SMPS_V1_Pin|SMPS_SW_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CSn_GPIO_Port, CSn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SMPS_EN_Pin SMPS_V1_Pin SMPS_SW_Pin CSn_Pin */
  GPIO_InitStruct.Pin = SMPS_EN_Pin|SMPS_V1_Pin|SMPS_SW_Pin|CSn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SMPS_PG_Pin */
  GPIO_InitStruct.Pin = SMPS_PG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SMPS_PG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD4_Pin */
  GPIO_InitStruct.Pin = LD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD4_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
