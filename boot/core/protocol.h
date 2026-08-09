/**
 * @file protocol.h
 * @brief Bootloader protocol state machine
 */

#ifndef CORE_PROTOCOL_H
#define CORE_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "../hal_if_defines.h"

/*=======================================================================
 * Protocol Constants
 *=======================================================================*/
#define BL_PKT_HEADER_REQUEST   0xAA
#define BL_PKT_HEADER_RESPONSE 0x55

#define BL_PKT_HEADER_SIZE    1
#define BL_PKT_OPCODE_SIZE   1
#define BL_PKT_LENGTH_SIZE   2
#define BL_PKT_CRC_SIZE      2
#define BL_PKT_MIN_SIZE      (BL_PKT_HEADER_SIZE + BL_PKT_OPCODE_SIZE + BL_PKT_LENGTH_SIZE + BL_PKT_CRC_SIZE)

#define BL_PKT_MAX_PAYLOAD   4096 + 8
#define BL_PKT_MAX_SIZE      (BL_PKT_MIN_SIZE + BL_PKT_MAX_PAYLOAD)

/*=======================================================================
 * Opcodes
 *=======================================================================*/
typedef enum {
    BL_OP_INQUERY     = 0x01,
    BL_OP_RESET       = 0x21,
    BL_OP_BOOT        = 0x22,
    BL_OP_ERASE       = 0x81,
    BL_OP_PROGRAM     = 0x82,
    BL_OP_VERIFY      = 0x83,
    BL_OP_WRITE_HEADER = 0x84,
} bl_opcode_t;

/*=======================================================================
 * Device Info (returned by INQUERY/0x00, 16 bytes packed)
 *
 * Layout (all multi-byte fields are little-endian):
 *   offset  0..2  : BL version (major, minor, build)
 *   offset  3      : has_app flag (0 = no app on device, 1 = app present)
 *   offset  4..6  : App version (major, minor, build; valid only if has_app=1)
 *   offset  7..8  : MTU (max payload size in bytes)
 *   offset  9..12 : Capability bitmask (BIT0=SIGNED, BIT1=ANTIROLLBACK, BIT2=CRC, ...)
 *   offset 13..15 : reserved (set to 0)
 *=======================================================================*/
#pragma pack(push, 1)
typedef struct {
    uint8_t  bl_major;
    uint8_t  bl_minor;
    uint8_t  bl_build;
    uint8_t  has_app;
    uint8_t  app_major;
    uint8_t  app_minor;
    uint8_t  app_build;
    uint16_t mtu;
    uint32_t caps;
    uint32_t header_addr;   /* BL_MAGIC_HEADER_ADDR (depends on device flash layout) */
    uint32_t app_base;      /* BL_APP_BASE (depends on device flash layout) */
    uint8_t  reserved[3];
} device_info_t;
#pragma pack(pop)

/*=======================================================================
 * Error Codes
 *=======================================================================*/
typedef enum {
    BL_ERR_OK        = 0,
    BL_ERR_OPCODE    = 1,
    BL_ERR_OVERFLOW  = 2,
    BL_ERR_TIMEOUT   = 3,
    BL_ERR_FORMAT    = 4,
    BL_ERR_VERIFY    = 5,
    BL_ERR_PARAM     = 6,
    BL_ERR_SIGNATURE = 7,
    BL_ERR_UNKNOWN   = 0xFF,
} bl_errcode_t;

/*=======================================================================
 * Protocol State Machine
 *=======================================================================*/
typedef enum {
    BL_STATE_IDLE    = 0,
    BL_STATE_HEADER  = 1,
    BL_STATE_OPCODE  = 2,
    BL_STATE_LENGTH  = 3,
    BL_STATE_PAYLOAD = 4,
    BL_STATE_CRC     = 5,
} bl_state_t;

/* Handler context */
typedef struct bl_context bl_context_t;
typedef bl_errcode_t (*bl_handler_fn)(bl_context_t *ctx);

typedef struct bl_protocol {
    bl_state_t     state;
    bl_opcode_t    opcode;
    uint16_t       payload_length;
    uint16_t       payload_index;
    uint32_t       last_byte_time;   /* 32-bit ms timestamp */
    uint16_t       rx_timeout_ms;    /* uint16_t: supports up to 65535ms, avoids uint8_t truncation */
    bl_handler_fn  handlers[256];
    uint8_t        buffer[BL_PKT_MAX_SIZE];
    uint16_t       buffer_index;
    const platform_desc_t *plat;
} bl_protocol_t;

struct bl_context {
    bl_protocol_t *proto;
    const uint8_t *payload;
    uint16_t payload_length;
    uint8_t *response;
    uint16_t *response_length;
    bl_errcode_t errcode;
    bool resp_sent;  /* handler already sent response via bl_response() */
};

/*=======================================================================
 * API
 *=======================================================================*/
void bl_protocol_init(bl_protocol_t *proto, const platform_desc_t *plat);
bool bl_protocol_feed(bl_protocol_t *proto, const uint8_t *data, uint32_t len);
void bl_protocol_register(bl_protocol_t *proto, bl_opcode_t opcode, bl_handler_fn fn);
void bl_response(bl_context_t *ctx, bl_opcode_t opcode, bl_errcode_t errcode,
                const uint8_t *data, uint16_t len);
void bl_protocol_dispatch(bl_protocol_t *proto);
void bl_protocol_reset(bl_protocol_t *proto);

#endif /* CORE_PROTOCOL_H */
