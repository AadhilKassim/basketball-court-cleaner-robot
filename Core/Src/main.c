/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for Autonomous Basketball Court Cleaner
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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "usbd_cdc_if.h"
#include "stm32f4xx_hal_tim.h"  // <--- Required for __HAL_TIM_GET_COUNTER, __HAL_TIM_SET_COMPARE, HAL_TIM_PWM_Start
#include "stm32f4xx_hal_uart.h" // <--- Required for HAL_UART_Receive_IT, UART_HandleTypeDef
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BATTERY_CUTOFF_VOLTAGE 9.6f  // 3.2V per cell minimum threshold
#define CURRENT_JAM_THRESHOLD  2800  // ADC threshold for ~15A stall spike on 775 motor
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint8_t rx_byte;
volatile uint32_t echo1_t1 = 0, echo1_t2 = 0;
volatile uint32_t echo2_t1 = 0, echo2_t2 = 0;
volatile float dist1_cm = 0.0f, dist2_cm = 0.0f;
volatile float battery_voltage = 12.0f;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void Motor_Set_Left(int16_t speed);
void Motor_Set_Right(int16_t speed);
void Motor_Set_Speed(int16_t left, int16_t right);
void Process_Bluetooth_Command(uint8_t cmd);
float Read_ADC_Channel(uint32_t channel);
float Read_Battery_Voltage(void);
void Check_Roller_Stall(void);
void Enter_Stop_Mode(void);
void Trigger_Ultrasonic_Sensors(void);
void USB_Print(const char *str);
void USB_Send_Initial_Boot_Log(void);
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
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  // 1. Start TIM3 counter base for ultrasonic echo pulse timing
  HAL_TIM_Base_Start(&htim3);

  // 2. Enable Motor Driver Logic Gates via PA15
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);

  // 3. Start PWM Timer Channels for Left Motor Driver (TIM3 CH3 & CH4)
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); // PB0 - Left RPWM
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); // PB1 - Left LPWM

  // 4. Start PWM Timer Channels for Right Motor Driver (TIM4 CH3 & CH4)
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3); // PB8 - Right RPWM
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4); // PB9 - Right LPWM

  // 5. Arm Bluetooth Interrupt Listener
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

  HAL_Delay(2000);
  USB_Send_Initial_Boot_Log();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
 while (1)
  {
    // 1. Safety Check: Disabled for USB testing without battery attached
    battery_voltage = Read_Battery_Voltage();
    /*
    if (battery_voltage > 5.0f && battery_voltage < BATTERY_CUTOFF_VOLTAGE) {
        Motor_Set_Speed(0, 0);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
        Enter_Stop_Mode();
    }
    */

    Check_Roller_Stall();
    Trigger_Ultrasonic_Sensors();

    char stream_buf[96];
    snprintf(stream_buf, sizeof(stream_buf), "Loop Running | Batt: %.2fV | D1: %.1fcm | D2: %.1fcm\r\n", battery_voltage, dist1_cm, dist2_cm);
             
    USB_Print(stream_buf);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Flash onboard blue LED
    HAL_Delay(500);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
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
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 9;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 9;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 999;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE(); // <--- 1. Enable GPIOC Clock

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_8, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // <--- 2. Set PC13 High (LED OFF)

  /*Configure GPIO pins : PA0 PA2 PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PC13 (Onboard LED) */
  GPIO_InitStruct.Pin = GPIO_PIN_13; // <--- 3. Configure PC13 as Push-Pull Output
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
/* USER CODE BEGIN 4 */

// Left Motor Driver Control (TIM3 CH3 / CH4 on PB0 / PB1)
void Motor_Set_Left(int16_t speed) {
    if (speed > 0) {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, speed);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
    } else if (speed < 0) {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, -speed);
    } else {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
    }
}

// Right Motor Driver Control (TIM4 CH3 / CH4 on PB8 / PB9)
void Motor_Set_Right(int16_t speed) {
    if (speed > 0) {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, speed);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);
    } else if (speed < 0) {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, -speed);
    } else {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);
    }
}

void Motor_Set_Speed(int16_t left, int16_t right) {
    Motor_Set_Left(left);
    Motor_Set_Right(right);
}

// Bluetooth Communication Callback
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        Process_Bluetooth_Command(rx_byte);
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

void Process_Bluetooth_Command(uint8_t cmd) {
    switch (cmd) {
        case 'F': Motor_Set_Speed(700, 700); break;   // Forward
        case 'B': Motor_Set_Speed(-700, -700); break; // Reverse
        case 'L': Motor_Set_Speed(-500, 500); break;  // Spin Left
        case 'R': Motor_Set_Speed(500, -500); break;  // Spin Right
        case 'S': Motor_Set_Speed(0, 0); break;       // Stop
        case 'C':                                     // Toggle Crusher Motor
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);
            break;
        case 'Z':                                     // Enter Sleep Mode
            Enter_Stop_Mode();
            break;
        default: break;
    }
}

// Single-Channel ADC Polling Utility
float Read_ADC_Channel(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES; // Increased for input stabilization
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t raw = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
        return (float)raw;
    }
    HAL_ADC_Stop(&hadc1);
    return 0.0f;
}

// Battery Voltage Calculation via PA5 Resistor Divider
float Read_Battery_Voltage(void) {
    float raw_adc = Read_ADC_Channel(ADC_CHANNEL_5);
    float pin_v = (raw_adc * 3.3f) / 4095.0f;
    // Divider multiplier using 3x330 ohm top / 1x330 ohm bottom: ratio 4.0
    // Normal Ratio without divider = 1.0f
    return pin_v * 4.251f; 
}

// Automatic Crusher Jam Detection Strategy
void Check_Roller_Stall(void) {
    float current_raw = Read_ADC_Channel(ADC_CHANNEL_4);
    if (current_raw > CURRENT_JAM_THRESHOLD) { // Over-current spike detected
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET); // Turn OFF Crusher Motor
        Motor_Set_Speed(-600, -600); // Back up briefly to clear debris
        HAL_Delay(800);
        Motor_Set_Speed(0, 0);
    }
}

// Low-Power STOP Mode Execution
void Enter_Stop_Mode(void) {
    Motor_Set_Speed(0, 0);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); // Disable BTS7960 Drivers
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);  // Turn OFF Crusher

    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    // --- MCU Sleeps Here Until Woken By Interrupt ---

    SystemClock_Config();
    HAL_ResumeTick();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);   // Re-enable Drivers
}

// Ultrasonic Echo Pulse Distance Calculation
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_1) { // Echo 1 (PA1)
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) {
            echo1_t1 = __HAL_TIM_GET_COUNTER(&htim3);
        } else {
            echo1_t2 = __HAL_TIM_GET_COUNTER(&htim3);
            uint32_t diff = (echo1_t2 >= echo1_t1) ? (echo1_t2 - echo1_t1) : ((1000 - echo1_t1) + echo1_t2);
            dist1_cm = (float)diff * 0.017f;
        }
    }
    if (GPIO_Pin == GPIO_PIN_3) { // Echo 2 (PA3)
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET) {
            echo2_t1 = __HAL_TIM_GET_COUNTER(&htim3);
        } else {
            echo2_t2 = __HAL_TIM_GET_COUNTER(&htim3);
            uint32_t diff = (echo2_t2 >= echo2_t1) ? (echo2_t2 - echo2_t1) : ((1000 - echo2_t2) + echo2_t2);
            dist2_cm = (float)diff * 0.017f;
        }
    }
}

void Trigger_Ultrasonic_Sensors(void) {
    // Pulse PA0 (Trigger 1)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    for (volatile int i = 0; i < 100; i++);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

    // Pulse PA2 (Trigger 2)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
    for (volatile int i = 0; i < 100; i++);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
}

// Non-blocking USB Print Utility
void USB_Print(const char *str) {
    // Try to transmit once. If busy, skip this frame to prevent locking up the main loop.
    uint8_t result = CDC_Transmit_FS((uint8_t *)str, strlen(str));
    if (result == USBD_BUSY) {
        // Buffer is full (host terminal not reading). Drop packet gracefully.
    }
}

// Diagnostics Dump Executed On Boot
void USB_Send_Initial_Boot_Log(void) {
    char log_buf[128];

    USB_Print("\r\n================================================\r\n");
    USB_Print("  AUTONOMOUS BASKETBALL COURT CLEANER - BOOT LOG \r\n");
    USB_Print("================================================\r\n");

    snprintf(log_buf, sizeof(log_buf), "SYSCLK Target Speed : %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000UL);
    USB_Print(log_buf);

    snprintf(log_buf, sizeof(log_buf), "HCLK Speed          : %lu MHz\r\n", HAL_RCC_GetHCLKFreq() / 1000000UL);
    USB_Print(log_buf);

    snprintf(log_buf, sizeof(log_buf), "PCLK1 Bus Speed     : %lu MHz\r\n", HAL_RCC_GetPCLK1Freq() / 1000000UL);
    USB_Print(log_buf);

    snprintf(log_buf, sizeof(log_buf), "PCLK2 Bus Speed     : %lu MHz\r\n", HAL_RCC_GetPCLK2Freq() / 1000000UL);
    USB_Print(log_buf);

    USB_Print("------------------------------------------------\r\n");
    USB_Print("Peripheral Status Setup:\r\n");
    USB_Print(" [OK] TIM3 CH3/CH4 PWM Motor Driver (Left)\r\n");
    USB_Print(" [OK] TIM4 CH3/CH4 PWM Motor Driver (Right)\r\n");
    USB_Print(" [OK] USART1 Bluetooth Listener (9600 Baud)\r\n");
    USB_Print(" [OK] EXTI Line 1 / Line 3 Ultrasonic Echo Subsystem\r\n");

    float batt = Read_Battery_Voltage();
    snprintf(log_buf, sizeof(log_buf), " [OK] ADC Channel 5 LiPo Battery Rail: %.2f V\r\n", batt);
    USB_Print(log_buf);

    USB_Print("================================================\r\n");
    USB_Print("System operational. Entering autonomous loop...\r\n\r\n");
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
