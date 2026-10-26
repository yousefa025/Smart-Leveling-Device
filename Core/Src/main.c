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
#include <stdio.h>
#include <math.h>
#include "oled_text.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

int16_t accelX;
int16_t accelY;
int16_t accelZ;
char uartBuffer[160];
float pitch;
float roll;
float accelMagnitude;
float vibration;
char *stabilityStatus;
float rawPitch;
float rawRoll;
float pitchOffset = 0.0f;
float rollOffset = 0.0f;
GPIO_PinState lastCalButtonState = GPIO_PIN_SET;
float heldPitch = 0.0f;
float heldRoll = 0.0f;
float displayedPitch;
float displayedRoll;

uint8_t holdActive = 0;
GPIO_PinState lastHoldButtonState = GPIO_PIN_SET;
uint8_t warningActive = 0;
float filteredPitch = 0.0f;
float filteredRoll = 0.0f;

float rawVibration = 0.0f;
float filteredVibration = 0.0f;

uint8_t filterInitialized = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define MPU6050_ADDR (0x68 << 1)

void MPU6050_Init(void)
{
  uint8_t data = 0x00;

  // Write 0 to PWR_MGMT_1 to wake the MPU6050
  HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, 0x6B,
                    I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

void MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az)
{
  uint8_t data[6];

  HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, 0x3B,
                   I2C_MEMADD_SIZE_8BIT, data, 6, 100);

  *ax = (int16_t)(data[0] << 8 | data[1]);
  *ay = (int16_t)(data[2] << 8 | data[3]);
  *az = (int16_t)(data[4] << 8 | data[5]);
}

#define OLED_ADDR (0x3C << 1)

void OLED_Command(uint8_t command)
{
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x00,
                    I2C_MEMADD_SIZE_8BIT, &command, 1, 100);
}

void OLED_Init(void)
{
  HAL_Delay(100);

  OLED_Command(0xAE); // Display off
  OLED_Command(0x20); // Memory addressing mode
  OLED_Command(0x00); // Horizontal addressing
  OLED_Command(0xB0);
  OLED_Command(0xC8);
  OLED_Command(0x00);
  OLED_Command(0x10);
  OLED_Command(0x40);
  OLED_Command(0x81); // Contrast
  OLED_Command(0xFF);
  OLED_Command(0xA1);
  OLED_Command(0xA6); // Normal display
  OLED_Command(0xA8);
  OLED_Command(0x3F);
  OLED_Command(0xA4);
  OLED_Command(0xD3);
  OLED_Command(0x00);
  OLED_Command(0xD5);
  OLED_Command(0xF0);
  OLED_Command(0xD9);
  OLED_Command(0x22);
  OLED_Command(0xDA);
  OLED_Command(0x12);
  OLED_Command(0xDB);
  OLED_Command(0x20);
  OLED_Command(0x8D); // Charge pump
  OLED_Command(0x14);
  OLED_Command(0xAF); // Display on
}

void OLED_TestPattern(void)
{
  uint8_t pattern[128];

  for (uint8_t page = 0; page < 8; page++)
  {
    OLED_Command(0xB0 + page);
    OLED_Command(0x00);
    OLED_Command(0x10);

    for (uint8_t column = 0; column < 128; column++)
    {
      pattern[column] = (column % 2 == 0) ? 0xAA : 0x55;
    }

    HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x40,
                      I2C_MEMADD_SIZE_8BIT, pattern, 128, 100);
  }
}
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Text_Init(&hi2c1);
  MPU6050_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  MPU6050_ReadAccel(&accelX, &accelY, &accelZ);

	    // Calculate uncalibrated pitch
	    rawPitch = atan2f(
	        -(float)accelX,
	        sqrtf(
	            (float)accelY * accelY +
	            (float)accelZ * accelZ
	        )
	    ) * 57.2958f;

	    // Calculate uncalibrated roll
	    rawRoll = atan2f(
	        (float)accelY,
	        (float)accelZ
	    ) * 57.2958f;

	    // Read calibration button on PA0/A0
	    GPIO_PinState calButtonState =
	        HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

	    // Calibrate once when button is pressed
	    if ((calButtonState == GPIO_PIN_RESET) &&
	        (lastCalButtonState == GPIO_PIN_SET))
	    {
	      pitchOffset = rawPitch;
	      rollOffset = rawRoll;

	      // Immediately reset the filtered angles to zero
	      filteredPitch = 0.0f;
	      filteredRoll = 0.0f;
	      filterInitialized = 1;

	      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
	    }

	    if (calButtonState == GPIO_PIN_SET)
	    {
	      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
	    }

	    lastCalButtonState = calButtonState;

	    // Apply calibration
	    float measuredPitch = rawPitch - pitchOffset;
	    float measuredRoll = rawRoll - rollOffset;

	    if (!filterInitialized)
	    {
	      // Use the first measurement directly
	      filteredPitch = measuredPitch;
	      filteredRoll = measuredRoll;
	      filterInitialized = 1;
	    }
	    else
	    {
	      // 85% previous value and 15% new measurement
	      filteredPitch =
	          (0.70f * filteredPitch) + (0.30f * measuredPitch);

	      filteredRoll =
	          (0.70f * filteredRoll) + (0.30f * measuredRoll);
	    }

	    pitch = filteredPitch;
	    roll = filteredRoll;

	    // Read Hold button on PA1/A1
	    GPIO_PinState holdButtonState =
	        HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);

	    // Toggle Hold mode once per button press
	    if ((holdButtonState == GPIO_PIN_RESET) &&
	        (lastHoldButtonState == GPIO_PIN_SET))
	    {
	      holdActive = !holdActive;

	      // Capture the current angles when Hold is activated
	      if (holdActive)
	      {
	        heldPitch = pitch;
	        heldRoll = roll;
	      }
	    }

	    lastHoldButtonState = holdButtonState;

	    // Select live or held values for the future OLED display
	    if (holdActive)
	    {
	      displayedPitch = heldPitch;
	      displayedRoll = heldRoll;
	    }
	    else
	    {
	      displayedPitch = pitch;
	      displayedRoll = roll;
	    }

	    // Calculate total acceleration
	    accelMagnitude = sqrtf(
	        (float)accelX * accelX +
	        (float)accelY * accelY +
	        (float)accelZ * accelZ
	    ) / 16384.0f;

	    // Calculate vibration
	    rawVibration = fabsf(accelMagnitude - 1.0f);

	    // Smooth the displayed vibration value
	    filteredVibration =
	        (0.70f * filteredVibration) +
	        (0.30f * rawVibration);

	    vibration = filteredVibration;

	    // Determine stability status
	    if (vibration < 0.08f)
	    {
	      stabilityStatus = "STABLE";
	    }
	    else if (vibration < 0.20f)
	    {
	      stabilityStatus = "MOVING";
	    }
	    else
	    {
	      stabilityStatus = "HIGH VIBRATION";
	    }

	    // Activate warning for excessive live tilt or high vibration
	    if ((fabsf(measuredPitch) > 4.0f) ||
	        (fabsf(measuredRoll) > 4.0f) ||
	        (rawVibration >= 0.20f))
	    {
	      warningActive = 1;

	      // Active-low buzzer: LOW turns it on
	      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	    }
	    else
	    {
	      warningActive = 0;

	      // Active-low buzzer: HIGH turns it off
	      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	    }

	    int vibrationMg = (int)(vibration * 1000.0f);
	    // Create the OLED screen
	    char displayLine[22];

	    OLED_Text_Clear();

	    // Pitch line
	    OLED_Text_SetCursor(0, 0);
	    snprintf(displayLine, sizeof(displayLine),
	             "PITCH:%4d DEG", (int)displayedPitch);
	    OLED_Text_WriteString(displayLine);

	    // Roll line
	    OLED_Text_SetCursor(0, 1);
	    snprintf(displayLine, sizeof(displayLine),
	             "ROLL :%4d DEG", (int)displayedRoll);
	    OLED_Text_WriteString(displayLine);

	    // Vibration line
	    OLED_Text_SetCursor(0, 2);
	    snprintf(displayLine, sizeof(displayLine),
	             "VIB  :%4d MG", vibrationMg);
	    OLED_Text_WriteString(displayLine);

	    // Stability line
	    OLED_Text_SetCursor(0, 4);
	    OLED_Text_WriteString(stabilityStatus);

	    // Hold status
	    OLED_Text_SetCursor(0, 5);

	    if (holdActive)
	    {
	      OLED_Text_WriteString("HOLD : ON");
	    }
	    else
	    {
	      OLED_Text_WriteString("HOLD : OFF");
	    }


	    OLED_Text_SetCursor(0, 6);

	    if (warningActive)
	    {
	      OLED_Text_WriteString("ALARM: ON");
	    }
	    else
	    {
	      OLED_Text_WriteString("ALARM: OFF");
	    }
	    // Button reminder
	    OLED_Text_SetCursor(0, 7);
	    OLED_Text_WriteString("CAL:A0 HOLD:A1");

	    // Transfer the completed screen to the OLED
	    OLED_Text_Update();

	    // UART shows both live and held/displayed measurements
	    int length = sprintf(
	        uartBuffer,
	        "Live P=%d R=%d | Display P=%d R=%d | Vib=%d mg | %s | Hold=%s\r\n",
	        (int)pitch,
	        (int)roll,
	        (int)displayedPitch,
	        (int)displayedRoll,
	        vibrationMg,
	        stabilityStatus,
	        holdActive ? "ON" : "OFF"
	    );

	    HAL_UART_Transmit(
	        &huart2,
	        (uint8_t *)uartBuffer,
	        length,
	        100
	    );

	    HAL_Delay(50);
	  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
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
  hi2c1.Init.Timing = 0x00201D2B;
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
  huart2.Init.BaudRate = 38400;
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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pins : cal_button_Pin hold_button_Pin */
  GPIO_InitStruct.Pin = cal_button_Pin|hold_button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : buzzer_Pin */
  GPIO_InitStruct.Pin = buzzer_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(buzzer_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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

#ifdef  USE_FULL_ASSERT
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
