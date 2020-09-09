/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "GSBP_Basic_Config.h"
#include "GSBP_Basic.h"
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
  MX_USART2_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  /* Initialise variables
   **********************************************************************************************/
  mcuState = preInit;
  uint32_t ledCounter = HAL_GetTick() + LED_PERIOD_MS;
  uint32_t dataCounter = 0;
  uint32_t dataCounterPeriodMS = 0;
  uint16_t dataSize = 0;
  uint8_t  dataIncrement = 0;
  int16_t  dataLastValue = 0;

  /* Initialise the communication system
   *********************************************************************************************/
  	GSBP_Init();
  	gsbpDebugMSG(3, "\n\n\nGSBP Development System started...\n##############################\n\n");


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		/*
		 *  run the low frequency LED code
		 */
		if (HAL_GetTick() >= ledCounter) {
			ledCounter = HAL_GetTick() + LED_PERIOD_MS;
			HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		}

		/*
		 *  run the GSBP callback to check for new packages
		 */
		if (HAL_GetTick() >= gCOM.NextCallbackTimer) {
			gCOM.NextCallbackTimer = HAL_GetTick() +GSBP_SETUP__CALLBACK_PERIOD_IN_MS;
			GSBP_CheckAndEvaluatePackages();
		}

		/*
		 *  execute the global state machine
		 */
		switch (mcuState){
		case preInit:
			// do noting -> wait for the init command
			break;

		case doInit:{
			// simple initialisation for this example

			// get the data of the initalisation CMD stored in gCOM.CMD
			initCMD_t *cmd = (initCMD_t*)gCOM.CMD.Data;
			// update the variables with the values from the CMD
			dataCounterPeriodMS = cmd->dataPeriodMS;
			dataSize = cmd->dataSize;
			if (dataSize > GSBP_MEASURMENT_DATA_SIZE_MAX){
				dataSize = GSBP_MEASURMENT_DATA_SIZE_MAX;
			}
			dataIncrement = cmd->increment;
			dataLastValue = 0;

			// prepare the init ACK
			gCOM.ACK.CommandID = InitACK;				// set the correct command ID
			gCOM.ACK.RequestID = gCOM.CMD.RequestID; 	// set the request id from the command, so the ACK is associated correctly
			initACK_t *ack = (initACK_t*)gCOM.ACK.Data;	// connect the data storage of the ACK with a initACK_t struct for ease of use
			if (dataSize != 0) {
				ack->success = true;
			} else {
				ack->success = false;
			}
			ack->dataPeriodMS = dataCounterPeriodMS;
			ack->dataSize = dataSize;
			ack->increment = dataIncrement;

			gCOM.ACK.DataSize = sizeof(initACK_t);
			// send the ACK to the PC
			GSBP_SendPackage(gCOM.CMD.HandleOfThisCMD, &gCOM.ACK);

			// update the state machine
			mcuState = postInit;
			}
			break;

		case postInit:
			// do noting -> wait for the start command
			HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
			break;

		case startMeasurement:
			// start the measurement
			if (dataSize != 0){
				dataCounter = HAL_GetTick() + dataCounterPeriodMS;
				// send the ACK
				GSBP_SendUniversalACK(gCOM.CMD.HandleOfThisCMD, StartApplicationCMD, true);
			} else {
				// dataSize is 0 because the was no successful initialisation
				GSBP_SendUniversalACK(gCOM.CMD.HandleOfThisCMD, StartApplicationCMD, false);
			}
			// update the state machine
			mcuState = measurementActive;
			break;

		case measurementActive:
			// check if a new dummy measurement data package needs to be send
			if (HAL_GetTick() >= dataCounter){
				dataCounter = HAL_GetTick() + dataCounterPeriodMS;

				// prepare the measurement data ACK
				gCOM.ACK.CommandID = ApplicationDataACK;							// set the correct command ID
				gCOM.ACK.RequestID = GSBP__REQUEST_ID_MEASUREMENT_DATA; 			// set the request id for measurement data ACKs
				measurementDataACK_t *ack = (measurementDataACK_t*)gCOM.ACK.Data;	// connect the data storage of the ACK
				for (uint16_t i=0; i<dataSize; i++){
					ack->data[i] = dataLastValue;
					dataLastValue += dataIncrement;
				}
				ack->numberOfValues = dataSize;
				gCOM.ACK.DataSize = sizeof(int16_t)*ack->numberOfValues +sizeof(uint16_t);
				// send the ACK to the PC
				GSBP_SendPackage(gCOM.CMD.HandleOfThisCMD, &gCOM.ACK);
			}
			break;

		case stopMeasurement:
			// stop the measurement
			// send the ACK
			GSBP_SendUniversalACK(gCOM.CMD.HandleOfThisCMD, StopApplicationCMD, true);
			// update the state machine
			mcuState = postInit;
			break;

		case doDeInit:
			// simple deinitialisation for this example
			dataSize = 0;
			// send the ACK
			GSBP_SendUniversalACK(gCOM.CMD.HandleOfThisCMD, DeInitCMD, true);
			// update the state machine
			mcuState = preInit;
			break;

		default:
			// update the state machine
			mcuState = preInit;
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USB;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
