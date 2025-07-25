#include "mini_print.h"
#include <stdarg.h>
#include <stdint.h>

// 将整数转成字符串（支持十进制/十六进制）
static void int_to_str(int64_t value, char *buf, int base) {
  char tmp[16];
  const char *digits = "0123456789ABCDEF";
  int i = 0, neg = 0;

  if (value == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }

  if (value < 0 && base == 10) {
    neg = 1;
    value = -value;
  }
  if (base == 10) {
    while (value && i < (int)sizeof(tmp) - 1) {
      tmp[i++] = digits[value % base];
      value /= base;
    }
  } else if (base == 16) {
    while (value && i < 8) {
      tmp[i++] = digits[value & 0x0F];
      value >>= 4;
    }
  }

  if (neg)
    tmp[i++] = '-';

  // 反转 tmp 到 buf
  for (int j = 0; j < i; ++j) {
    buf[j] = tmp[i - 1 - j];
  }
  buf[i] = '\0';
}

// 简化版 printf，写入目标 buffer
int mini_printf(char *buffer, size_t max, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char *p = buffer;
  char num_buf[32];

  while (*fmt) {
    if (p - buffer >= max - 1) // keep one byte for null-terminator
      break;
    if (*fmt != '%') {
      *p++ = *fmt++;
      continue;
    }

    fmt++; // skip '%'

    switch (*fmt) {
    case 's': {
      const char *s = va_arg(args, const char *);
      while (*s && p - buffer < max - 1) {
        *p++ = *s++;
      }
      break;
    }
    case 'd': {
      int64_t val = va_arg(args, int);
      int_to_str(val, num_buf, 10);
      const char *s = num_buf;
      while (*s && p - buffer < max - 1) {
        *p++ = *s++;
      }
      break;
    }
    case 'x':
    case 'X': {
      int64_t val = va_arg(args, int);
      int_to_str(val, num_buf, 16);
      const char *s = num_buf;
      while (*s && p - buffer < max - 1) {
        *p++ = *s++;
      }
      break;
    }
    case 'c': {
      char c = (char)va_arg(args, int);
      *p++ = c;
      break;
    }
    default:
      *p++ = '%';
      *p++ = *fmt;
      break;
    }
    fmt++;
  }

  *p = '\0'; // null-terminate
  va_end(args);
  return p - buffer;
}
