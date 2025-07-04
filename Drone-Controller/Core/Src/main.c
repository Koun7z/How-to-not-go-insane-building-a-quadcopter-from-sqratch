/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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

// Data acusition
#include "CRSF_Connection.h"
#include "MPU6050.h"

// FC
#include "FC_Config.h"
#include "FlightController.h"

// ST Middleware
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include <sys\stat.h>

#include <float.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define THROTTLE CRSF_Channels[2]
#define PITCH    CRSF_Channels[1]
#define ROLL     CRSF_Channels[0]
#define YAW      CRSF_Channels[3]

#define CCR_MAX 2400.0f
#define CCR_MIN 480.0f

#define MIN_VOLTAGE 20.0f
#define REF_VOLTAGE 20.0f

#define VOLTAGE_DIV_RATIO 10.375f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c1_tx;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */
volatile bool NewIMUData            = false;
volatile bool NewRCData             = false;
volatile bool ConnectionEstablished = false;

volatile float BatteryVoltage = FLT_MAX;

uint32_t IMU_Rate = 0;

CRSF_BatteryData BatteryData;
MPU_Instance IMUData;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */

int _write(int file, char* ptr, int len)
{
	CDC_Transmit_FS(ptr, len);
	return len;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size)
{
	NewRCData = true;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
	printf("CPLT\r\n");
	NewRCData = true;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
	CRSF_HandleErr();
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c)
{
	MPU_HandleRX(&IMUData);
	NewIMUData = true;
}

void CRSF_OnChannelsPacked()
{
	ConnectionEstablished = true;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	BatteryVoltage = (float)HAL_ADC_GetValue(&hadc1) * 3.3f * VOLTAGE_DIV_RATIO / 4096.0f;

	BatteryData.Voltage = (int16_t)(BatteryVoltage * 10.0f);

	// TODO: Debug info for tuning
	const float Ti           = FC_RC_Data.Aux2 * 4 / 100.0f + 0.05f;
	BatteryData.UsedCapacity = (int)(Ti * 1000.0f);
}

void FC_OnDebugLog(const char* msg, size_t len)
{
	printf("%s", msg);
}

void BlinkLedNTimes(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t n);

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
	MX_DMA_Init();
	MX_I2C1_Init();
	MX_TIM3_Init();
	MX_USART1_UART_Init();
	MX_USB_DEVICE_Init();
	MX_ADC1_Init();
	MX_CRC_Init();
	/* USER CODE BEGIN 2 */

	HAL_Delay(500);

	CRSF_Init(&huart1);

	while(!MPU_Init(&IMUData, &hi2c1, MPU_ADDRESS_1))
	{
	}
	HAL_Delay(2000);
	MPU_CalibrateAll(&IMUData,1000);
	FC_Init();

	static float a[] = {-1.5610f, 0.6414f};
	static float b[] = {0.0201f, 0.0402f, 0.020f};

	FC_RC_SetFilter(b, a);
	FC_IMU_SetGyroFilter(b, a);
	FC_IMU_SetAccelFilter(b, a);


	TIM3->CCR1 = 0;
	TIM3->CCR2 = 0;
	TIM3->CCR3 = 0;
	TIM3->CCR4 = 0;
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while(1)
	{
		// Connection loss protection
		if(HAL_GetTick() - CRSF_LastChannelsPacked > 200 && ConnectionEstablished)
		{
			FC_EmergencyDisarm(FC_ConnectionLost);
		}

		// Low battery protection
		static uint32_t lowVCtr = 0;
		if(BatteryVoltage < MIN_VOLTAGE && BatteryVoltage > 6.0f && ConnectionEstablished)
		{
			if(HAL_GetTick() - lowVCtr > 1000)
			{
				//TODO:
				// FC_EmergencyDisarm(FC_BatteryLow);
			}
		}
		else
		{
			lowVCtr = HAL_GetTick();
		}

		if(NewRCData)
		{
			NewRCData = false;
			CRSF_HandleRX();
			// FC_RC_UpdateArmStatus(CRSF_ArmStatus);
			// FC_RC_UpdateAxesChannels(THROTTLE, ROLL, PITCH, YAW);
			// FC_RC_UpdateAuxChannels(CRSF_Channels.Ch5, CRSF_Channels.Ch6, CRSF_Channels.Ch7, CRSF_Channels.Ch8);
		}


		if(NewIMUData)
		{
			NewIMUData = false;
			FC_IMU_UpdateGyro(IMUData.GyroX, IMUData.GyroY, IMUData.GyroZ, 0.005f);
			FC_IMU_UpdateAccel(IMUData.AccelX, IMUData.AccelY, IMUData.AccelZ, 0.005f);

			/*float angles[3];
			DSP_QT_EulerAngles_f32(angles, &FC_IMU_Data.Attitude);*/

			/*printf("%f,%f,%f,%f,%f,%f\n\r", IMUData.GyroX, IMUData.GyroY, IMUData.GyroZ, IMUData.AccelX,
			   IMUData.AccelY, IMUData.AccelZ);*/

			IMU_Rate++;

			// printf("%f,%f,%f,%f,%f,%f,%f,%f\r\n", FC_IMU_Data.GyroX, FC_IMU_Data.GyroY, FC_IMU_Data.GyroZ,
			// FC_IMU_Data.AccelX, FC_IMU_Data.AccelY, FC_IMU_Data.AccelZ, angles[0], angles[1]);
		}


		static uint32_t lastIMU = 0;
		if(HAL_GetTick() - lastIMU >= 5)
		{
			lastIMU = HAL_GetTick();
			MPU_RequestAllDMA(&IMUData);
		}


		static uint32_t lastTick5 = 0;
		if(HAL_GetTick() - lastTick5 >= (uint32_t)(FC_PID_Ts * 1000))
		{
			lastTick5 = HAL_GetTick();
			FC_Update(FC_PID_Ts);

			// printf("%f,%f,%f,%f\n", FC_GlobalThrust.Motor1, FC_GlobalThrust.Motor2, FC_GlobalThrust.Motor3,
			//        FC_GlobalThrust.Motor4);


			const float voltage_comp = REF_VOLTAGE / BatteryVoltage;

			TIM3->CCR1 = (uint32_t)(FC_GlobalThrust.Motor1 * (CCR_MAX - CCR_MIN) * voltage_comp / 100.0f + CCR_MIN);
			TIM3->CCR2 = (uint32_t)(FC_GlobalThrust.Motor2 * (CCR_MAX - CCR_MIN) * voltage_comp / 100.0f + CCR_MIN);
			TIM3->CCR3 = (uint32_t)(FC_GlobalThrust.Motor3 * (CCR_MAX - CCR_MIN) * voltage_comp / 100.0f + CCR_MIN);
			TIM3->CCR4 = (uint32_t)(FC_GlobalThrust.Motor4 * (CCR_MAX - CCR_MIN) * voltage_comp / 100.0f + CCR_MIN);
		}
		else
		{
			HAL_ADC_Start_IT(&hadc1);

			CRSF_QueueBatteryData(&BatteryData);
		}
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		static uint32_t lastTick1000 = 0;

		const FC_StatusTypeDef errStatus = FC_GetStatus();

		switch(errStatus)
		{
			case FC_BatteryLow:
				BlinkLedNTimes(INT_LED_GPIO_Port, INT_LED_Pin, 2);
				break;
			case FC_ConnectionLost:
				BlinkLedNTimes(INT_LED_GPIO_Port, INT_LED_Pin, 3);
				break;
			default:
				break;
		}

		if(HAL_GetTick() - lastTick1000 >= 1000)
		{
			lastTick1000 = HAL_GetTick();

			// printf("%u\n\r", IMU_Rate);
			IMU_Rate = 0;
			if (!errStatus)
				HAL_GPIO_TogglePin(INT_LED_GPIO_Port, INT_LED_Pin);
		}
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
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
	RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM       = 25;
	RCC_OscInitStruct.PLL.PLLN       = 192;
	RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ       = 4;
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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
	hadc1.Instance                   = ADC1;
	hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode          = DISABLE;
	hadc1.Init.ContinuousConvMode    = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion       = 1;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
	if(HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel      = ADC_CHANNEL_0;
	sConfig.Rank         = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if(HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */
}

/**
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void)
{
	/* USER CODE BEGIN CRC_Init 0 */

	/* USER CODE END CRC_Init 0 */

	/* USER CODE BEGIN CRC_Init 1 */

	/* USER CODE END CRC_Init 1 */
	hcrc.Instance = CRC;
	if(HAL_CRC_Init(&hcrc) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN CRC_Init 2 */

	/* USER CODE END CRC_Init 2 */
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
	hi2c1.Instance             = I2C1;
	hi2c1.Init.ClockSpeed      = 400000;
	hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1     = 0;
	hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2     = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
	if(HAL_I2C_Init(&hi2c1) != HAL_OK)
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
	TIM_OC_InitTypeDef sConfigOC          = {0};

	/* USER CODE BEGIN TIM3_Init 1 */

	/* USER CODE END TIM3_Init 1 */
	htim3.Instance               = TIM3;
	htim3.Init.Prescaler         = 0;
	htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim3.Init.Period            = 4799;
	htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if(HAL_TIM_PWM_Init(&htim3) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
	if(HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sConfigOC.OCMode     = TIM_OCMODE_PWM1;
	sConfigOC.Pulse      = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if(HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	if(HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
	if(HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
	if(HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM3_Init 2 */

	/* USER CODE END TIM3_Init 2 */
	HAL_TIM_MspPostInit(&htim3);
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
	huart1.Instance          = USART1;
	huart1.Init.BaudRate     = 420000;
	huart1.Init.WordLength   = UART_WORDLENGTH_8B;
	huart1.Init.StopBits     = UART_STOPBITS_1;
	huart1.Init.Parity       = UART_PARITY_NONE;
	huart1.Init.Mode         = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if(HAL_UART_Init(&huart1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{
	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_DMA2_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Stream0_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
	/* DMA1_Stream1_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
	/* DMA2_Stream2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
	/* DMA2_Stream7_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
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
	HAL_GPIO_WritePin(INT_LED_GPIO_Port, INT_LED_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : INT_LED_Pin */
	GPIO_InitStruct.Pin   = INT_LED_Pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(INT_LED_GPIO_Port, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// Yeah, ChatGPT wrote this one, I needed it for a quick test
void BlinkLedNTimes(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t n)
{
	typedef enum { LED_STATE_IDLE, LED_STATE_BLINK_ON, LED_STATE_BLINK_OFF, LED_STATE_WAIT } LedState;

	static LedState state       = LED_STATE_IDLE;
	static uint32_t lastTick    = 0;
	static uint8_t blinkCount   = 0;
	static uint8_t targetBlinks = 0;

	switch(state)
	{
		case LED_STATE_IDLE:
			if(n > 0)
			{
				targetBlinks = n;
				blinkCount   = 0;
				state        = LED_STATE_BLINK_ON;
				lastTick     = HAL_GetTick();
				HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
			}
			break;

		case LED_STATE_BLINK_ON:
			if(HAL_GetTick() - lastTick >= 100)
			{
				HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
				lastTick = HAL_GetTick();
				state    = LED_STATE_BLINK_OFF;
			}
			break;

		case LED_STATE_BLINK_OFF:
			if(HAL_GetTick() - lastTick >= 100)
			{
				blinkCount++;
				if(blinkCount < targetBlinks)
				{
					HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
					lastTick = HAL_GetTick();
					state    = LED_STATE_BLINK_ON;
				}
				else
				{
					HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
					state    = LED_STATE_WAIT;
					lastTick = HAL_GetTick();
				}
			}
			break;

		case LED_STATE_WAIT:
			if(HAL_GetTick() - lastTick >= 1000)
			{
				state = LED_STATE_IDLE;
			}
			break;
	}
}
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
	TIM3->CCR1 = 0;
	TIM3->CCR2 = 0;
	TIM3->CCR3 = 0;
	TIM3->CCR4 = 0;
	while(1)
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
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
