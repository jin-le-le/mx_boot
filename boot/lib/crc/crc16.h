/**
 * @file crc16.h
 * @brief CRC16 implementation
 */

#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc16(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC16_H */
