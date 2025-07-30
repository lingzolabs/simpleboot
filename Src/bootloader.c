#include "bootloader.h"
#include "common.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "ymodem.h"
#include <stdint.h>
#include <string.h>

/* Global bootloader context */
bootloader_context_t g_bootloader_context;
ymodem_file_info_t g_file_info;

/* UART handle (defined in main.c) */
extern UART_HandleTypeDef huart1;

/* Private function prototypes */
static void bootloader_set_application_vector_table(void);
static bool bootloader_packet_callback(const uint8_t *data, uint16_t data_size,
                                       uint32_t packet_num, void *user_data);

#define WAIT_HERE(x)                                                           \
  do {                                                                         \
    bootloader_led_toggle();                                                   \
    HAL_Delay(x);                                                              \
  } while (1)
/**
 * @brief Initialize bootloader
 */
void bootloader_init(void) {
  /* Initialize bootloader context */
  memset(&g_bootloader_context, 0, sizeof(bootloader_context_t));
  g_bootloader_context.state = BOOTLOADER_STATE_INIT;

  /* Print banner */
  bootloader_print_banner();
  BOOTLOADER_LOG("Bootloader initialized");
}

/**
 * @brief Main bootloader execution loop
 * @return Bootloader result code
 */
bootloader_result_t bootloader_run(void) {
  bootloader_result_t result = BOOTLOADER_OK;

  g_bootloader_context.state = BOOTLOADER_STATE_CHECK_CONDITIONS;

  while (1) {
    switch (g_bootloader_context.state) {
    case BOOTLOADER_STATE_CHECK_CONDITIONS:
      if (bootloader_should_enter()) {
        BOOTLOADER_LOG("Entering bootloader mode");
        g_bootloader_context.state = BOOTLOADER_STATE_WAIT_FOR_FIRMWARE;
      } else {
        BOOTLOADER_LOG("Jumping to application");
        g_bootloader_context.state = BOOTLOADER_STATE_JUMP_TO_APP;
      }
      break;

    case BOOTLOADER_STATE_WAIT_FOR_FIRMWARE:
      BOOTLOADER_LOG("Waiting for firmware... Send file using Y-modem");
      g_bootloader_context.state = BOOTLOADER_STATE_RECEIVING_FIRMWARE;
      break;

    case BOOTLOADER_STATE_RECEIVING_FIRMWARE:
      bootloader_led_set(false);
      result = bootloader_receive_firmware();
      HAL_Delay(1000);
      BOOTLOADER_LOG(
          "firmware_info.size: %d, received: %d, packets: %d, ret: %d",
          g_file_info.file_size, g_file_info.received_size,
          g_file_info.packet_count, result);
      BOOTLOADER_LOG("file_info: %s, %d, %d", g_file_info.filename,
                     g_file_info.state, g_file_info.error_count);
      bootloader_led_toggle();
      if (result == BOOTLOADER_OK) {
        g_bootloader_context.firmware_info.magic = APPLICATION_META_MAGIC;
        bootloader_result_t ret = bootloader_program_flash(
            APPLICATION_META_ADDR,
            (uint8_t *)&g_bootloader_context.firmware_info,
            sizeof(firmware_info_t));
        g_bootloader_context.state = BOOTLOADER_STATE_VERIFYING_FIRMWARE;
      } else {
        BOOTLOADER_LOG("Firmware reception failed!");
        g_bootloader_context.state = BOOTLOADER_STATE_ERROR;
      }
      break;

    case BOOTLOADER_STATE_VERIFYING_FIRMWARE:
      BOOTLOADER_LOG("Verifying firmware...");
      result = bootloader_verify_firmware(&g_bootloader_context.firmware_info);

      if (result == BOOTLOADER_OK) {
        BOOTLOADER_LOG("Firmware verification successful!");
        g_bootloader_context.state = BOOTLOADER_STATE_JUMP_TO_APP;
      } else {
        BOOTLOADER_LOG("Firmware verification failed!");
        g_bootloader_context.state = BOOTLOADER_STATE_ERROR;
      }
      break;

    case BOOTLOADER_STATE_JUMP_TO_APP:
      if (bootloader_is_application_valid()) {
        BOOTLOADER_LOG("Starting application...");
        bootloader_jump_to_application();
      } else {
        BOOTLOADER_LOG("No valid application found!");
        g_bootloader_context.state = BOOTLOADER_STATE_WAIT_FOR_FIRMWARE;
      }
      break;

    case BOOTLOADER_STATE_ERROR:
      BOOTLOADER_LOG("Bootloader error occurred!");
      bootloader_led_toggle();
      bootloader_delay_ms(100);
      /* Reset to wait for firmware after error */
      g_bootloader_context.state = BOOTLOADER_STATE_WAIT_FOR_FIRMWARE;
      g_bootloader_context.error_count++;

      if (g_bootloader_context.error_count > 5) {
        bootloader_system_reset();
      }
      break;

    default:
      g_bootloader_context.state = BOOTLOADER_STATE_ERROR;
      break;
    }
  }

  return result;
}

/**
 * @brief Check if bootloader should enter update mode
 * @return true if bootloader should enter, false otherwise
 */
bool bootloader_should_enter(void) {
  /* Check if button is pressed */
  if (bootloader_is_button_pressed()) {
    BOOTLOADER_LOG("Button pressed - entering bootloader");
    return true;
  }

  /* Check magic number in RAM */
  if (bootloader_check_magic_number()) {
    BOOTLOADER_LOG("Magic number detected - entering bootloader");
    /* Clear magic number */
    *((uint32_t *)BOOTLOADER_MAGIC_ADDR) = 0;
    return true;
  }

  /* Check if valid application exists */
  if (!bootloader_is_application_valid()) {
    BOOTLOADER_LOG("No valid application - entering bootloader");
    return true;
  }

  return false;
}

/**
 * @brief Check if user button is pressed
 * @return true if pressed, false otherwise
 */
bool bootloader_is_button_pressed(void) {
  return (HAL_GPIO_ReadPin(KEY_2_GPIO_Port, KEY_2_Pin) == GPIO_PIN_SET);
}

/**
 * @brief Check magic number in RAM
 * @return true if magic number is present, false otherwise
 */
bool bootloader_check_magic_number(void) {
  uint32_t magic = *((uint32_t *)BOOTLOADER_MAGIC_ADDR);
  return (magic == BOOTLOADER_ENTER_MAGIC);
}

/**
 * @brief Check if application in flash is valid
 * @return true if valid, false otherwise
 */
bool bootloader_is_application_valid(void) {
  uint32_t app_stack_ptr = *((uint32_t *)APPLICATION_START_ADDR);
  uint32_t app_reset_vector = *((uint32_t *)(APPLICATION_START_ADDR + 4));
  uint32_t app_meta_magic = *((uint32_t *)(APPLICATION_META_ADDR));

  /* Check if stack pointer is in RAM range */
  if ((app_stack_ptr & 0xFFF00000) != 0x20000000) {
    return false;
  }

  /* Check if reset vector is in flash range and thumb bit is set */
  if ((app_reset_vector & 0xFF000000) != 0x08000000 ||
      (app_reset_vector & 0x01) == 0) {
    return false;
  }

  /* Check if application meta is valid */
  if (app_meta_magic != APPLICATION_META_MAGIC) {
    BOOTLOADER_LOG("Invalid application meta, %x, %x", app_meta_magic,
                   APPLICATION_META_MAGIC);
    return false;
  }

  return true;
}

/* Structure for packet callback context */
typedef struct {
  uint8_t buffer[FLASH_PAGE_SIZE];
  uint32_t current_flash_address;
  uint32_t total_written;
  uint32_t file_crc32;
  bool flash_unlocked;
} packet_context_t;
/**
 * @brief Packet processing callback for real-time flash writing
 * @param data: Packet data
 * @param data_size: Size of packet data
 * @param packet_num: Packet number
 * @param user_data: User context data
 * @return Y-modem result code
 */
static bool bootloader_packet_callback(const uint8_t *data, uint16_t data_size,
                                       uint32_t packet_num, void *user_data) {
  packet_context_t *ctx = (packet_context_t *)user_data;
  bootloader_result_t ret = bootloader_program_flash(
      ctx->current_flash_address, (const char *)data, data_size);
  if (ret != BOOTLOADER_OK) {
    return false;
  }
  ctx->current_flash_address += data_size;
  ctx->total_written += data_size;
  ctx->file_crc32 = crc32_update(ctx->file_crc32, data, data_size);
  return true;
}

/**
 * @brief Receive firmware via Y-modem protocol
 * @return Bootloader result code
 */
bootloader_result_t bootloader_receive_firmware(void) {
  ymodem_result_t ymodem_result;
  bootloader_result_t result;
  packet_context_t ctx;

  /* Initialize packet context */
  memset(&ctx, 0, sizeof(packet_context_t));
  ctx.current_flash_address = APPLICATION_START_ADDR;
  ctx.file_crc32 = 0xFFFFFFFF;
  ctx.flash_unlocked = false;
  /* Initialize Y-modem receiver */
  ymodem_result = ymodem_receive_init();
  if (ymodem_result != YMODEM_OK) {
    return BOOTLOADER_ERROR;
  }

  if (!ymodem_wait_receive_header(&g_file_info, 10)) {
    BOOTLOADER_LOG("Timeout wait file");
    return BOOTLOADER_ERROR;
  }

  /* Erase application flash sectors first */
  result = bootloader_erase_application_flash();
  if (result != BOOTLOADER_OK) {
    return result;
  }
  /* Receive file with callback for real-time processing */
  ymodem_result = ymodem_receive_file_with_callback(
      &g_file_info, bootloader_packet_callback, &ctx);

  if (ymodem_result != YMODEM_OK) {
    return BOOTLOADER_ERROR;
  }

  /* Update firmware info */
  g_bootloader_context.firmware_info.start_address = APPLICATION_START_ADDR;
  g_bootloader_context.firmware_info.size = g_file_info.file_size;
  g_bootloader_context.firmware_info.crc32 = ctx.file_crc32;
  g_bootloader_context.firmware_info.is_valid = true;

  return BOOTLOADER_OK;
}

/**
 * @brief Erase application flash pages
 * @return Bootloader result code
 */
bootloader_result_t bootloader_erase_application_flash(void) {
  FLASH_EraseInitTypeDef erase_init;
  uint32_t page_error;
  HAL_StatusTypeDef status;

  /* Unlock flash */
  status = HAL_FLASH_Unlock();
  if (status != HAL_OK) {
    return BOOTLOADER_FLASH_ERROR;
  }

  /* Configure erase - erase all pages from application start to end */
  erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
  erase_init.PageAddress = APPLICATION_META_ADDR;
  erase_init.NbPages =
      (FLASH_END_ADDR - APPLICATION_META_ADDR + 1) / FLASH_PAGE_SIZE;

  /* Perform erase */
  status = HAL_FLASHEx_Erase(&erase_init, &page_error);

  /* Lock flash */
  HAL_FLASH_Lock();

  if (status != HAL_OK) {
    BOOTLOADER_LOG("Flash erase failed, page error: 0x%x", page_error);
    return BOOTLOADER_FLASH_ERROR;
  }

  return BOOTLOADER_OK;
}

/**
 * @brief Program flash memory
 * @param address: Start address to program
 * @param data: Data buffer
 * @param size: Data size
 * @return Bootloader result code
 */
bootloader_result_t
bootloader_program_flash(uint32_t address, const uint8_t *data, uint32_t size) {
  HAL_StatusTypeDef status;
  uint32_t current_address = address;
  uint32_t data_index = 0;

  /* Unlock flash */
  status = HAL_FLASH_Unlock();
  if (status != HAL_OK) {
    return BOOTLOADER_FLASH_ERROR;
  }

  /* Program flash halfword by halfword (16-bit) for STM32F1xx */
  while (data_index < size) {
    uint16_t halfword_data = 0;

    /* Prepare 16-bit halfword from byte array */
    halfword_data = data[data_index];
    if ((data_index + 1) < size) {
      halfword_data |= ((uint16_t)data[data_index + 1]) << 8;
    }

    /* Program halfword */
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, current_address,
                               halfword_data);
    if (status != HAL_OK) {
      HAL_FLASH_Lock();
      return BOOTLOADER_FLASH_ERROR;
    }

    /* Verify programmed data */
    if (*((uint16_t *)current_address) != halfword_data) {
      HAL_FLASH_Lock();
      return BOOTLOADER_FLASH_ERROR;
    }

    current_address += 2;
    data_index += 2;

    /* Toggle LED to show progress */
    if ((data_index % 128) == 0) {
      bootloader_led_toggle();
    }
  }

  /* Lock flash */
  HAL_FLASH_Lock();

  return BOOTLOADER_OK;
}

/**
 * @brief Verify programmed firmware
 * @param firmware_info: Firmware information
 * @return Bootloader result code
 */
bootloader_result_t
bootloader_verify_firmware(const firmware_info_t *firmware_info) {
  uint32_t calculated_crc;
  uint8_t *flash_data = (uint8_t *)firmware_info->start_address;

  /* Calculate CRC32 of flash contents */
  calculated_crc = crc32_update(0xffffffff, flash_data, firmware_info->size);

  BOOTLOADER_LOG("CRC verification: expected 0x%x, got 0x%x",
                 firmware_info->crc32, calculated_crc);
  /* Compare with expected CRC */
  if (calculated_crc != firmware_info->crc32) {
    return BOOTLOADER_VERIFY_ERROR;
  }

  /* Check if application looks valid */
  if (!bootloader_is_application_valid()) {
    BOOTLOADER_LOG("Invalid firmware");
    return BOOTLOADER_INVALID_APPLICATION;
  }

  return BOOTLOADER_OK;
}

/**
 * @brief Jump to application
 */
void bootloader_jump_to_application(void) {
  uint32_t app_stack_ptr = *((uint32_t *)APPLICATION_START_ADDR);
  uint32_t app_reset_vector = *((uint32_t *)(APPLICATION_START_ADDR + 4));

  /* Function pointer for application reset handler */
  void (*app_reset_handler)(void) = (void (*)(void))(app_reset_vector);

  /* Disable all interrupts */
  bootloader_disable_interrupts();

  /* Deinitialize peripherals */
  bootloader_deinit_peripherals();

  /* Set vector table to application */
  bootloader_set_application_vector_table();

  /* Set main stack pointer */
  __set_MSP(app_stack_ptr);

  /* Jump to application */
  app_reset_handler();
}

/**
 * @brief Set vector table to application address
 */
static void bootloader_set_application_vector_table(void) {
  SCB->VTOR = APPLICATION_START_ADDR;
}

/**
 * @brief Print bootloader banner
 */
void bootloader_print_banner(void) {
  const char *banner = "\r\n"
                       "================================\r\n"
                       "           SimpleBoot           \r\n"
                       "================================\r\n"
                       "Version: " BOOTLOADER_VERSION "\r\n"
                       "Build:   " __DATE__ " " __TIME__ "\r\n"
                       "================================\r\n";
  BOOTLOADER_LOG("%s", banner);
}

/**
 * @brief Calculate CRC32
 * @param data: Data buffer
 * @param size: Data size
 * @return CRC32 value
 */
uint32_t bootloader_calculate_crc32(uint8_t *data, uint32_t size) {
  uint32_t crc = 0xFFFFFFFF;

  for (uint32_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xEDB88320;
      } else {
        crc >>= 1;
      }
    }
  }

  return ~crc;
}

/**
 * @brief Toggle LED
 */
void bootloader_led_toggle(void) {
  HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);
}

/**
 * @brief Set LED state
 * @param state: true for on, false for off
 */
void bootloader_led_set(bool state) {
  HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin,
                    state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Delay in milliseconds
 * @param delay: Delay in milliseconds
 */
void bootloader_delay_ms(uint32_t delay) { HAL_Delay(delay); }

/**
 * @brief System reset
 */
void bootloader_system_reset(void) { HAL_NVIC_SystemReset(); }

/**
 * @brief Disable all interrupts
 */
void bootloader_disable_interrupts(void) {
  __disable_irq();
  __set_PRIMASK(0);
}

/**
 * @brief Deinitialize peripherals
 */
void bootloader_deinit_peripherals(void) {
  HAL_UART_DeInit(&huart1);
  HAL_DeInit();
}
