/**
 ******************************************************************************
 * @file    app_main.c
 * @author  Bootloader Example
 * @brief   Example application for STM32F103C8T6 bootloader
 * @note    This application demonstrates:
 *          - Proper vector table relocation
 *          - LED blinking functionality
 *          - Bootloader entry via magic number
 *          - UART communication
 ******************************************************************************
 */

#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_gpio_ex.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Private defines */
#define BOOTLOADER_MAGIC_ADDR 0x20000000
#define BOOTLOADER_MAGIC 0xDEADBEEF

/* Private variables */
UART_HandleTypeDef huart1;
uint32_t tick_counter = 0;
bool bootloader_request = false;

/* Private function prototypes */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART1_UART_Init(void);
void Error_Handler(void);
void App_Print(const char *message);
void Check_Button(void);
void Enter_Bootloader(void);

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  // SCB->VTOR = 0x08004000;
  /* MCU Configuration */
  HAL_Init();
  SystemClock_Config();

  /* Initialize peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);

  /* Application banner */
  App_Print("\r\n========================================");
  App_Print("STM32F103C8T6 Example Application");
  App_Print("Built for Bootloader Integration");
  App_Print("Version: 1.0.0");
  App_Print("Build: " __DATE__ " " __TIME__);
  App_Print("========================================");
  App_Print("Commands:");
  App_Print("  - Press USER button to enter bootloader");
  App_Print("  - LED will blink every second");
  App_Print("  - Send 'B' via UART to enter bootloader");
  App_Print("========================================\r\n");

  /* Main application loop */
  while (1) {
    /* Check for bootloader entry request */
    Check_Button();

    /* Handle UART input */
    uint8_t rx_data;
    if (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
      if (rx_data == 'B' || rx_data == 'b') {
        App_Print("Bootloader entry requested via UART!");
        bootloader_request = true;
      } else if (rx_data == 'S' || rx_data == 's') {
        char status_msg[100];
        sprintf(status_msg, "App Status: Running for %lu seconds",
                tick_counter / 1000);
        App_Print(status_msg);
      } else if (rx_data == 'H' || rx_data == 'h') {
        App_Print("Available commands:");
        App_Print("  B - Enter bootloader");
        App_Print("  S - Show status");
        App_Print("  H - Show help");
      }
    }

    /* Enter bootloader if requested */
    if (bootloader_request) {
      Enter_Bootloader();
    }

    /* Blink LED every second */
    static uint32_t last_blink = 0;
    if (HAL_GetTick() - last_blink >= 500) {
      HAL_GPIO_TogglePin(LED_2_GPIO_Port, LED_2_Pin);
      last_blink = HAL_GetTick();
      tick_counter = HAL_GetTick();

      /* Print heartbeat every 10 seconds */
      if ((tick_counter / 1000) % 10 == 0 && (tick_counter % 1000) < 50) {
        char heartbeat_msg[50];
        sprintf(heartbeat_msg, "Heartbeat: %lu seconds", tick_counter / 1000);
        App_Print(heartbeat_msg);
      }
    }

    HAL_Delay(10);
  }
}

/**
 * @brief Check if button is pressed for bootloader entry
 */
void Check_Button(void) {
  static uint32_t button_press_time = 0;
  static bool button_was_pressed = false;

  bool button_pressed =
      (HAL_GPIO_ReadPin(KEY_2_GPIO_Port, KEY_2_Pin) == GPIO_PIN_SET);

  if (button_pressed && !button_was_pressed) {
    button_press_time = HAL_GetTick();
    button_was_pressed = true;
    App_Print("Button pressed - hold for 2 seconds to enter bootloader");
  } else if (!button_pressed && button_was_pressed) {
    button_was_pressed = false;
    App_Print("Button released");
  } else if (button_pressed && button_was_pressed) {
    if (HAL_GetTick() - button_press_time >= 2000) {
      App_Print("Button held for 2 seconds - entering bootloader!");
      bootloader_request = true;
    }
  }
}

/**
 * @brief Enter bootloader by setting magic number and resetting
 */
void Enter_Bootloader(void) {
  HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
  App_Print("Setting bootloader magic number...");
  App_Print("System will reset and enter bootloader mode");
  App_Print("You can now send firmware via Y-modem");

  HAL_Delay(3000); // Allow UART transmission to complete

  /* Disable interrupts */
  __disable_irq();

  /* Set magic number at specific RAM location */
  *((uint32_t *)BOOTLOADER_MAGIC_ADDR) = BOOTLOADER_MAGIC;

  /* Reset the system */
  HAL_NVIC_SystemReset();
}

/**
 * @brief Print message via UART
 * @param message: Message to print
 */
void App_Print(const char *message) {
  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer), "[APP] %s\r\n", message);
  HAL_UART_Transmit(&huart1, (uint8_t *)buffer, len, 1000);
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_ClkInitTypeDef clkinitstruct = {0};
  RCC_OscInitTypeDef oscinitstruct = {0};

  oscinitstruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  oscinitstruct.HSEState = RCC_HSE_ON;
  oscinitstruct.LSEState = RCC_LSE_OFF;
  oscinitstruct.HSIState = RCC_HSI_OFF;
  oscinitstruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  oscinitstruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  oscinitstruct.PLL.PLLState = RCC_PLL_ON;
  oscinitstruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  oscinitstruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&oscinitstruct) != HAL_OK) {
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  clkinitstruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                             RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  clkinitstruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clkinitstruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
  clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}
void MX_USART1_UART_Init(void) {

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
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
void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_PIN */
  GPIO_InitStruct.Pin = LED_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BUTTON_PIN */
  GPIO_InitStruct.Pin = KEY_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY_2_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* User can add his own implementation to report the HAL error return state */
  while (1) {
    /* Error: Fast blink LED */
    HAL_GPIO_TogglePin(LED_2_GPIO_Port, LED_2_Pin);
    HAL_Delay(100);
  }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
}
#endif /* USE_FULL_ASSERT */
