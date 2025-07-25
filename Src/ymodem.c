#include "ymodem.h"
#include "common.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External UART handle */
extern UART_HandleTypeDef huart1;

/* Static functions */
static ymodem_result_t ymodem_receive_byte(uint8_t *byte, uint32_t timeout_ms);
static ymodem_result_t ymodem_send_byte(uint8_t byte);
static void ymodem_flush_input_buffer(void);

/**
 * @brief Verify packet CRC
 * @param packet: Pointer to Y-modem packet
 * @param data_size: Size of data in packet
 * @return true if CRC is valid, false otherwise
 */
bool ymodem_verify_crc(const ymodem_packet_t *packet, uint16_t data_size) {
  uint16_t calculated_crc = crc16_update(0x0000, packet->data, data_size);
  return (calculated_crc == packet->crc);
}

/**
 * @brief Check if packet is valid
 * @param packet: Pointer to Y-modem packet
 * @param expected_packet_num: Expected packet number
 * @return true if packet is valid, false otherwise
 */
bool ymodem_is_packet_valid(const ymodem_packet_t *packet,
                            uint8_t expected_packet_num) {
  // /* Check packet number and its inverse */
  // if (packet->packet_num != expected_packet_num) {
  //   return false;
  // }

  if (packet->packet_num + packet->packet_num_inv != 0xFF) {
    return false;
  }

  return true;
}

/**
 * @brief Reset Y-modem state
 * @param file_info: Pointer to file info structure
 */
void ymodem_reset_state(ymodem_file_info_t *file_info) {
  memset(file_info, 0, sizeof(ymodem_file_info_t));
  file_info->state = YMODEM_STATE_IDLE;
}

/**
 * @brief Initialize Y-modem receiver
 * @return Y-modem result code
 */
ymodem_result_t ymodem_receive_init(void) {
  /* Flush any pending data in UART buffer */
  ymodem_flush_input_buffer();

  /* Send initial NAK to request transmission start */
  return ymodem_send_response(YMODEM_C);
}

/**
 * @brief Send response byte
 * @param response: Response byte (ACK, NAK, CAN)
 * @return Y-modem result code
 */
ymodem_result_t ymodem_send_response(uint8_t response) {
  return ymodem_send_byte(response);
}

/**
 * @brief Receive a single byte with timeout
 * @param byte: Pointer to store received byte
 * @param timeout_ms: Timeout in milliseconds
 * @return Y-modem result code
 */
static ymodem_result_t ymodem_receive_byte(uint8_t *byte, uint32_t timeout_ms) {
  HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, byte, 1, timeout_ms);

  switch (status) {
  case HAL_OK:
    return YMODEM_OK;
  case HAL_TIMEOUT:
    return YMODEM_TIMEOUT;
  default:
    return YMODEM_ERROR;
  }
}

/**
 * @brief Send a single byte
 * @param byte: Byte to send
 * @return Y-modem result code
 */
static ymodem_result_t ymodem_send_byte(uint8_t byte) {
  HAL_StatusTypeDef status =
      HAL_UART_Transmit(&huart1, &byte, 1, YMODEM_TIMEOUT_MS);

  return (status == HAL_OK) ? YMODEM_OK : YMODEM_ERROR;
}

/**
 * @brief Flush input buffer
 */
static void ymodem_flush_input_buffer(void) {
  uint8_t dummy;

  /* Read all pending bytes with short timeout */
  while (ymodem_receive_byte(&dummy, 10) == YMODEM_OK) {
    /* Continue reading until timeout */
  }
}

/**
 * @brief Receive a Y-modem packet
 * @param packet: Pointer to packet structure
 * @return Y-modem result code
 */
ymodem_result_t ymodem_receive_packet(ymodem_packet_t *packet) {
  ymodem_result_t result;
  uint16_t data_size;
  uint8_t crc_bytes[2];

  /* Receive packet header */
  result = ymodem_receive_byte(&packet->header, YMODEM_TIMEOUT_MS);
  if (result != YMODEM_OK) {
    return result;
  }

  /* Check if it's EOT */
  if (packet->header == YMODEM_EOT) {
    return YMODEM_OK;
  }

  /* Check if it's CAN */
  if (packet->header == YMODEM_CAN) {
    return YMODEM_CANCELLED;
  }

  /* Determine packet size based on header */
  if (packet->header == YMODEM_SOH) {
    data_size = YMODEM_PACKET_SIZE_128;
  } else if (packet->header == YMODEM_STX) {
    data_size = YMODEM_PACKET_SIZE_1024;
  } else {
    return YMODEM_PACKET_ERROR;
  }

  /* Receive packet number */
  result = ymodem_receive_byte(&packet->packet_num, YMODEM_TIMEOUT_MS);
  if (result != YMODEM_OK) {
    return result;
  }

  /* Receive inverted packet number */
  result = ymodem_receive_byte(&packet->packet_num_inv, YMODEM_TIMEOUT_MS);
  if (result != YMODEM_OK) {
    return result;
  }

  /* Receive data */
  for (uint16_t i = 0; i < data_size; i++) {
    result = ymodem_receive_byte(&packet->data[i], YMODEM_TIMEOUT_MS);
    if (result != YMODEM_OK) {
      return result;
    }
  }

  /* Receive CRC */
  result = ymodem_receive_byte(&crc_bytes[0], YMODEM_TIMEOUT_MS);
  if (result != YMODEM_OK) {
    return result;
  }

  result = ymodem_receive_byte(&crc_bytes[1], YMODEM_TIMEOUT_MS);
  if (result != YMODEM_OK) {
    return result;
  }

  /* Convert CRC bytes to 16-bit value (big-endian) */
  packet->crc = (crc_bytes[0] << 8) | crc_bytes[1];

  /* Verify CRC */
  if (!ymodem_verify_crc(packet, data_size)) {
    return YMODEM_CRC_ERROR;
  }

  return YMODEM_OK;
}

/**
 * @brief Parse header packet to extract file information
 * @param packet: Pointer to header packet
 * @param file_info: Pointer to file info structure
 * @return Y-modem result code
 */
ymodem_result_t ymodem_parse_header_packet(const ymodem_packet_t *packet,
                                           ymodem_file_info_t *file_info) {
  char *ptr = (char *)packet->data;
  char *size_str;

  /* Reset file info */
  ymodem_reset_state(file_info);

  /* Check if this is the end-of-batch packet (empty filename) */
  if (packet->data[0] == 0) {
    file_info->state = YMODEM_STATE_COMPLETE;
    return YMODEM_OK;
  }

  /* Extract filename */
  strncpy(file_info->filename, ptr, sizeof(file_info->filename) - 1);
  file_info->filename[sizeof(file_info->filename) - 1] = '\0';

  /* Move to file size field */
  ptr += strlen(file_info->filename) + 1;

  /* Extract file size */
  size_str = ptr;
  if (*size_str != '\0') {
    file_info->file_size = (uint32_t)atol(size_str);
  }

  file_info->state = YMODEM_STATE_RECEIVING_DATA;
  file_info->packet_count = 1;

  return YMODEM_OK;
}

/* Handle header packet (packet 0) */
bool ymodem_wait_receive_header(ymodem_file_info_t *file_info, int times) {
  ymodem_result_t result = YMODEM_ERROR;
  ymodem_packet_t packet;
  /* Initialize file info */
  ymodem_reset_state(file_info);
  int i = 0;
  while (i++ < times) {
    result = ymodem_receive_packet(&packet);
    if (result == YMODEM_OK) {
      result = ymodem_parse_header_packet(&packet, file_info);
      if (result != YMODEM_OK) {
        ymodem_send_response(YMODEM_NAK);
        return false;
      }
      ymodem_send_response(YMODEM_ACK);
      return true;
    } else {
      ymodem_send_response(YMODEM_C);
    }
  }
  return false;
}

/**
 * @brief Receive a complete file using Y-modem protocol with packet callback
 * @param file_info: Pointer to file info structure
 * @param callback: Callback function to process each data packet
 * @param user_data: User data passed to callback function
 * @return Y-modem result code
 */
ymodem_result_t
ymodem_receive_file_with_callback(ymodem_file_info_t *file_info,
                                  ymodem_packet_callback_t callback,
                                  void *user_data) {
  ymodem_packet_t packet;
  ymodem_result_t result;
  uint8_t expected_packet_num = 1;

  while (file_info->state != YMODEM_STATE_COMPLETE &&
         file_info->state != YMODEM_STATE_ERROR &&
         file_info->state != YMODEM_STATE_CANCELLED) {

    /* Receive packet */
    result = ymodem_receive_packet(&packet);

    if (result == YMODEM_TIMEOUT) {
      file_info->error_count++;
      if (file_info->error_count >= YMODEM_MAX_ERRORS) {
        file_info->state = YMODEM_STATE_ERROR;
        ymodem_send_response(YMODEM_CAN);
        return YMODEM_ERROR;
      }
      ymodem_send_response(YMODEM_NAK);
      continue;
    }

    if (result == YMODEM_CANCELLED) {
      file_info->state = YMODEM_STATE_CANCELLED;
      return YMODEM_CANCELLED;
    }

    if (result != YMODEM_OK) {
      file_info->error_count++;
      if (file_info->error_count >= YMODEM_MAX_ERRORS) {
        file_info->state = YMODEM_STATE_ERROR;
        ymodem_send_response(YMODEM_CAN);
        return result;
      }
      ymodem_send_response(YMODEM_NAK);
      continue;
    }

    /* Handle EOT (End of Transmission) */
    if (packet.header == YMODEM_EOT) {
      ymodem_send_response(YMODEM_ACK);
      ymodem_send_response(YMODEM_C);

      /* After first EOT, expect second EOT or next file header */
      result = ymodem_receive_packet(&packet);
      if (result == YMODEM_OK && packet.header == YMODEM_EOT) {
        ymodem_send_response(YMODEM_ACK);
        /* Check if file is complete */
        if (file_info->received_size >= file_info->file_size) {
          file_info->state = YMODEM_STATE_COMPLETE;
        } else {
          file_info->state = YMODEM_STATE_ERROR;
        }
        break;
      } else {
        /* This might be the start of end-of-batch packet */
        if (packet.data[0] == 0) {
          ymodem_send_response(YMODEM_ACK);
          file_info->state = YMODEM_STATE_COMPLETE;
          break;
        }
      }
      continue;
    }

    /* Validate packet number */
    if (!ymodem_is_packet_valid(&packet, expected_packet_num)) {
      file_info->error_count++;
      if (file_info->error_count >= YMODEM_MAX_ERRORS) {
        file_info->state = YMODEM_STATE_ERROR;
        ymodem_send_response(YMODEM_CAN);
        return YMODEM_PACKET_ERROR;
      }
      ymodem_send_response(YMODEM_NAK);
      continue;
    }

    uint16_t packet_data_size = (packet.header == YMODEM_SOH)
                                    ? YMODEM_PACKET_SIZE_128
                                    : YMODEM_PACKET_SIZE_1024;

    /* Calculate actual data size (may be less than packet size for last
     * packet) */
    uint32_t remaining_bytes = file_info->file_size - file_info->received_size;
    uint16_t actual_data_size = (remaining_bytes < packet_data_size)
                                    ? remaining_bytes
                                    : packet_data_size;

    /* Call the callback function to process the data */
    if (callback != NULL) {
      bool ret = callback(packet.data, actual_data_size, expected_packet_num,
                          user_data);
      if (!ret) {
        file_info->state = YMODEM_STATE_ERROR;
        ymodem_send_response(YMODEM_CAN);
        return YMODEM_FLASH_ERROR;
      }
    }

    ymodem_send_response(YMODEM_ACK);
    expected_packet_num++;

    file_info->received_size += actual_data_size;
    file_info->packet_count++;

    /* Reset error count on successful packet */
    file_info->error_count = 0;
  }

  return YMODEM_OK;
}
