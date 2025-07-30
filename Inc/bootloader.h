#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#include "mini_print.h"
#include "stm32f1xx_hal.h"
#include "ymodem.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

/* Bootloader Configuration */
#define BOOTLOADER_VERSION "1.0.0"
#define BOOTLOADER_START_ADDR 0x08000000
#define VECT_TAB_OFFSET 0x4000 /* Vector table offset */
#define APPLICATION_START_ADDR                                                 \
  (BOOTLOADER_START_ADDR + VECT_TAB_OFFSET) /* 16KB offset for bootloader */
#define FLASH_END_ADDR 0x0800FFFF           /* 64KB Flash end */

#define BOOTLOADER_SIZE 0x4000 /* 16KB for bootloader */

/* Bootloader settings */
#define BOOTLOADER_TIMEOUT_MS 5000
#define APPLICATION_META_ADDR (APPLICATION_START_ADDR - 0x30)
#define APPLICATION_META_MAGIC 0x424F4F54 // BOOT

/* Magic numbers for bootloader control */
#define BOOTLOADER_MAGIC_ADDR 0x20000000 /* RAM address for magic number */
#define BOOTLOADER_ENTER_MAGIC 0xDEADBEEF

/* UART Configuration */
#define BOOTLOADER_UART_BAUDRATE 115200
#define BOOTLOADER_UART_TIMEOUT 1000

/* Bootloader states */
typedef enum {
  BOOTLOADER_STATE_INIT,
  BOOTLOADER_STATE_CHECK_CONDITIONS,
  BOOTLOADER_STATE_WAIT_FOR_FIRMWARE,
  BOOTLOADER_STATE_RECEIVING_FIRMWARE,
  BOOTLOADER_STATE_PROGRAMMING_FLASH,
  BOOTLOADER_STATE_VERIFYING_FIRMWARE,
  BOOTLOADER_STATE_JUMP_TO_APP,
  BOOTLOADER_STATE_ERROR
} bootloader_state_t;

/* Bootloader result codes */
typedef enum {
  BOOTLOADER_OK = 0,
  BOOTLOADER_ERROR,
  BOOTLOADER_TIMEOUT,
  BOOTLOADER_FLASH_ERROR,
  BOOTLOADER_VERIFY_ERROR,
  BOOTLOADER_NO_APPLICATION,
  BOOTLOADER_INVALID_APPLICATION
} bootloader_result_t;

/* Firmware information structure */
typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t size;
  uint32_t crc32;
} firmware_info_t;

/* Bootloader context */
typedef struct {
  bootloader_state_t state;
  firmware_info_t firmware_info;
  bool force_update;
  uint32_t error_count;
} bootloader_context_t;

/* Main bootloader functions */
void bootloader_init(void);
bootloader_result_t bootloader_run(void);
void bootloader_jump_to_application(void);

/* Condition checking functions */
bool bootloader_should_enter(void);
bool bootloader_is_button_pressed(void);
bool bootloader_check_magic_number(void);
bool bootloader_is_application_valid(void);

/* Flash operations */
bootloader_result_t bootloader_erase_application_flash(void);
bootloader_result_t
bootloader_program_flash(uint32_t address, const uint8_t *data, uint32_t size);
bootloader_result_t
bootloader_verify_firmware(const firmware_info_t *firmware_info);

/* Communication functions */
bootloader_result_t bootloader_receive_firmware(void);
void bootloader_print_banner(void);

void bootloader_delay_ms(uint32_t delay);
void bootloader_led_toggle(void);
void bootloader_led_set(bool state);

/* System functions */
void bootloader_system_reset(void);
void bootloader_disable_interrupts(void);
void bootloader_deinit_peripherals(void);

/* Flash sector mapping functions */
uint32_t bootloader_get_sector_from_address(uint32_t address);
uint32_t bootloader_get_sector_size(uint32_t sector);

#if 1
/* Debug and logging */
#define BOOTLOADER_LOG(fmt, ...)                                               \
  do {                                                                         \
    uint8_t buf[128];                                                          \
    int size = mini_printf((char *)buf, sizeof(buf) - 1, fmt, ##__VA_ARGS__);  \
    buf[size] = '\n';                                                          \
    size += 1;                                                                 \
    HAL_UART_Transmit(&huart1, buf, size, BOOTLOADER_UART_TIMEOUT);            \
  } while (0)
#else
#define BOOTLOADER_LOG(fmt, ...)
#endif
/* Error handling macros */
#define BOOTLOADER_ASSERT(condition)                                           \
  do {                                                                         \
    if (!(condition)) {                                                        \
      BOOTLOADER_LOG("ASSERTION FAILED: %s:%d", __FILE__, __LINE__);           \
      while (1)                                                                \
        ;                                                                      \
    }                                                                          \
  } while (0)

/* External variables */
extern UART_HandleTypeDef huart1;
extern bootloader_context_t g_bootloader_context;

#endif /* __BOOTLOADER_H__ */
