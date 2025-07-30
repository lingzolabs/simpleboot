#ifndef __YMODEM_H__
#define __YMODEM_H__

#include <stdbool.h>
#include <stdint.h>

/* Y-modem Protocol Constants */
#define YMODEM_SOH 0x01   /* Start of Header (128 bytes) */
#define YMODEM_STX 0x02   /* Start of Header (1024 bytes) */
#define YMODEM_EOT 0x04   /* End of Transmission */
#define YMODEM_ACK 0x06   /* Acknowledge */
#define YMODEM_NAK 0x15   /* Not Acknowledge */
#define YMODEM_CAN 0x18   /* Cancel */
#define YMODEM_CTRLZ 0x1A /* Ctrl+Z */
#define YMODEM_C 0x43     /* C */

#define YMODEM_PACKET_SIZE_128 128
#define YMODEM_PACKET_SIZE_1024 1024
#define YMODEM_PACKET_HEADER_SIZE 3
#define YMODEM_PACKET_TRAILER_SIZE 2
#define YMODEM_CRC_SIZE 2

#define YMODEM_MAX_ERRORS 10
#define YMODEM_TIMEOUT_MS 1000
#define YMODEM_LONG_TIMEOUT_MS 10000

/* Y-modem packet structure */
typedef struct {
  uint8_t header;                        /* SOH or STX */
  uint8_t packet_num;                    /* Packet number */
  uint8_t packet_num_inv;                /* Inverted packet number */
  uint8_t data[YMODEM_PACKET_SIZE_1024]; /* Data payload */
  uint16_t crc;                          /* CRC16 checksum */
} ymodem_packet_t;

/* Y-modem transfer states */
typedef enum {
  YMODEM_STATE_IDLE,
  YMODEM_STATE_RECEIVING_HEADER,
  YMODEM_STATE_RECEIVING_DATA,
  YMODEM_STATE_COMPLETE,
  YMODEM_STATE_ERROR,
  YMODEM_STATE_CANCELLED
} ymodem_state_t;

/* Y-modem file information */
typedef struct {
  char filename[128];
  uint32_t file_size;
  uint32_t received_size;
  uint32_t packet_count;
  ymodem_state_t state;
  uint8_t error_count;
} ymodem_file_info_t;

/* Y-modem result codes */
typedef enum {
  YMODEM_OK = 0,
  YMODEM_ERROR,
  YMODEM_TIMEOUT,
  YMODEM_CANCELLED,
  YMODEM_CRC_ERROR,
  YMODEM_PACKET_ERROR,
  YMODEM_FILE_ERROR,
  YMODEM_FLASH_ERROR
} ymodem_result_t;

/* Callback function type for packet processing */
typedef bool (*ymodem_packet_callback_t)(const uint8_t *data,
                                         uint16_t data_size,
                                         uint32_t packet_num, void *user_data);

/* Function prototypes */
ymodem_result_t ymodem_receive_init(void);
ymodem_result_t ymodem_receive_packet(ymodem_packet_t *packet);
bool ymodem_wait_receive_header(ymodem_file_info_t *file_info, int times);

ymodem_result_t
ymodem_receive_file_with_callback(ymodem_file_info_t *file_info,
                                  ymodem_packet_callback_t callback,
                                  void *user_data);
ymodem_result_t ymodem_send_response(uint8_t response);
ymodem_result_t ymodem_parse_header_packet(const ymodem_packet_t *packet,
                                           ymodem_file_info_t *file_info);

/* CRC calculation functions */
uint16_t ymodem_crc16(const uint8_t *data, uint16_t length);
bool ymodem_verify_crc(const ymodem_packet_t *packet, uint16_t data_size);

/* Utility functions */
void ymodem_reset_state(ymodem_file_info_t *file_info);
bool ymodem_is_packet_valid(const ymodem_packet_t *packet,
                            uint8_t expected_packet_num);

#endif /* __YMODEM_H__ */
