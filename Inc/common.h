#pragma once
#include <stdint.h>

uint16_t crc16_update(uint16_t crc, const uint8_t *data, uint16_t length);
uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t length);
