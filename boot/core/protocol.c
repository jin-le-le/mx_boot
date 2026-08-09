/**
 * @file protocol.c
 * @brief Bootloader protocol implementation with security checks
 */

#include "protocol.h"
#include "image.h"
#include "board_config.h"
#include "utils.h"
#include "crc16.h"
#include "crc32.h"
#include <string.h>

#define LOG_TAG "proto"
#define LOG_LVL ELOG_LVL_INFO
#include "elog.h"

/* Protocol buffer accessors */
static inline uint16_t get_payload_len(bl_protocol_t *proto)
{
    return get_u16(proto->buffer + BL_PKT_HEADER_SIZE + BL_PKT_OPCODE_SIZE);
}

static inline bl_opcode_t get_opcode(bl_protocol_t *proto)
{
    return (bl_opcode_t)proto->buffer[BL_PKT_HEADER_SIZE];
}

static void reset_protocol(bl_protocol_t *proto)
{
    proto->state = BL_STATE_HEADER;
    proto->buffer_index = 0;
    proto->payload_index = 0;
}

void bl_protocol_reset(bl_protocol_t *proto)
{
    reset_protocol(proto);
}

void bl_response(bl_context_t *ctx, bl_opcode_t opcode, bl_errcode_t errcode,
                const uint8_t *data, uint16_t len)
{
    uint8_t *rsp = ctx->response;
    uint8_t *pw = rsp;

    put_u8_inc(&pw, BL_PKT_HEADER_RESPONSE);
    put_u8_inc(&pw, (uint8_t)opcode);
    put_u8_inc(&pw, (uint8_t)errcode);
    put_u16_inc(&pw, len);
    if (data && len > 0) {
        memcpy(pw, data, len);
        pw += len;
    }
    uint16_t crc = crc16(rsp, pw - rsp);
    put_u16_inc(&pw, crc);

    uint16_t resp_len = (uint16_t)(pw - rsp);
    *ctx->response_length = resp_len;
    ctx->resp_sent = true;
}

/* Opcode handlers */
static bl_errcode_t h_inquery(bl_context_t *ctx);
static bl_errcode_t h_erase(bl_context_t *ctx);
static bl_errcode_t h_program(bl_context_t *ctx);
static bl_errcode_t h_verify(bl_context_t *ctx);
static bl_errcode_t h_reset(bl_context_t *ctx);
static bl_errcode_t h_boot(bl_context_t *ctx);
static bl_errcode_t h_write_header(bl_context_t *ctx);

static bl_context_t s_ctx;
static uint8_t s_response_buf[BL_PKT_MAX_SIZE];
static uint16_t s_response_len;  /* Shared response length variable */

static bl_errcode_t h_inquery(bl_context_t *ctx)
{
    if (ctx->payload_length < 1) {
        return BL_ERR_PARAM;
    }

    uint8_t sub = ctx->payload[0];
    if (sub != 0x00) {
        /* Only sub-command 0x00 (DEVICE_INFO) is supported now.
         * Old sub 0x01 (MTU) was merged into DEVICE_INFO. */
        return BL_ERR_PARAM;
    }

    log_i("[handle] --> DEVICE_INFO");

    device_info_t info;
    memset(&info, 0, sizeof(info));

    /* Parse BL version from "X.Y.Z" string in platform descriptor */
    const char *s = ctx->proto->plat->bl_version;
    uint8_t *out = &info.bl_major;
    uint8_t field = 0;
    while (*s && field < 3) {
        if (*s == '.') {
            field++;
            if (field == 1) out = &info.bl_minor;
            else if (field == 2) out = &info.bl_build;
            else break;
        } else if (*s >= '0' && *s <= '9') {
            *out = (uint8_t)(*out * 10 + (*s - '0'));
        } else {
            break;
        }
        s++;
    }

    info.mtu = BL_MAX_PAYLOAD_SIZE;

    /* Capabilities bitmask: BIT0=SIGNED, BIT1=ANTIROLLBACK, BIT2=CRC */
    uint32_t caps = 0x04;  /* CRC always supported */
#if BL_SIGNATURE_ENABLED
    caps |= 0x01;
#endif
#if BL_ANTIROLLBACK_ENABLED
    caps |= 0x02;
#endif
    info.caps = caps;

    /* Device flash layout info (so the upper-computer tool knows where to write) */
    info.header_addr = BL_MAGIC_HEADER_ADDR;
    info.app_base    = BL_APP_BASE;

    /* App version: read from image header at BL_MAGIC_HEADER_ADDR.
     * If header magic doesn't match (erased flash), has_app stays 0. */
    const bl_image_header_t *hdr = bl_image_get_header();
    if (hdr && hdr->magic == BL_IMAGE_MAGIC) {
        info.has_app = 1;
        info.app_major = (uint8_t)hdr->version_major;
        info.app_minor = (uint8_t)hdr->version_minor;
        info.app_build = (uint8_t)hdr->version_build;
    }

    bl_response(ctx, BL_OP_INQUERY, BL_ERR_OK, (const uint8_t *)&info, sizeof(info));
    return BL_ERR_OK;
}

static bl_errcode_t h_erase(bl_context_t *ctx)
{
    log_i("[hangdle] --> ERASE");
    const platform_desc_t *plat = ctx->proto->plat;
    const flash_if_t *flash = plat->flash;

    if (ctx->payload_length != 8) {
        return BL_ERR_PARAM;
    }

    const uint8_t *p = ctx->payload;
    uint32_t addr = get_u32_inc_const(&p);
    uint32_t size = get_u32_inc_const(&p);

    /* Security check: verify address range is safe */
    if (!bl_image_is_addr_safe(addr, size)) {
        log_e("erase: unsafe address 0x%08lX size %lu", addr, (unsigned long)size);
        return BL_ERR_PARAM;
    }

    flash->unlock();
    int ret = flash->erase(addr, size);
    flash->lock();

    if (ret != 0) {
        log_e("erase failed at 0x%08lX", addr);
        return BL_ERR_PARAM;
    }
    return BL_ERR_OK;
}

static bl_errcode_t h_program(bl_context_t *ctx)
{
    log_i("[hangdle] --> PROGRAM");
    const platform_desc_t *plat = ctx->proto->plat;
    const flash_if_t *flash = plat->flash;

    if (ctx->payload_length <= 8) {
        return BL_ERR_PARAM;
    }

    const uint8_t *p = ctx->payload;
    uint32_t addr = get_u32_inc_const(&p);
    uint32_t size = get_u32_inc_const(&p);
    const uint8_t *data = p;

    if (size != ctx->payload_length - 8) {
        return BL_ERR_PARAM;
    }

    /* Security check: verify address range is safe */
    if (!bl_image_is_addr_safe(addr, size)) {
        log_e("program: unsafe address 0x%08lX size %lu", addr, (unsigned long)size);
        return BL_ERR_PARAM;
    }

    log_i("program: addr=0x%08lX size=%lu, payload_len=%u", addr, (unsigned long)size, ctx->payload_length);


    flash->unlock();
    int ret = flash->program(addr, data, size);
    flash->lock();

    if (ret != 0) {
        log_e("program failed at 0x%08lX", addr);
        return BL_ERR_PARAM;
    }
   
    return BL_ERR_OK;
}

static bl_errcode_t h_write_header(bl_context_t *ctx)
{
    log_i("[handle] --> WRITE_HEADER");

    if (ctx->payload_length != 256) {
        log_e("write_header: invalid payload length %u, expected 256",
              ctx->payload_length);
        return BL_ERR_PARAM;
    }

    if (bl_image_write_header(ctx->payload, ctx->payload_length) != 0) {
        log_e("write_header: failed to write header to flash");
        return BL_ERR_PARAM;
    }

    return BL_ERR_OK;
}

static bl_errcode_t h_verify(bl_context_t *ctx)
{
    if (ctx->payload_length != 12) {
        return BL_ERR_PARAM;
    }

    log_i("[handle] --> VERIFY");
    const uint8_t *p = ctx->payload;
    uint32_t addr = get_u32_inc_const(&p);
    uint32_t size = get_u32_inc_const(&p);
    uint32_t expected_crc = get_u32_inc_const(&p);

    /* Security check: verify address range is safe for reading */
    if (!bl_image_is_addr_safe(addr, size)) {
        log_e("verify: unsafe address 0x%08lX size %lu", addr, (unsigned long)size);
        return BL_ERR_PARAM;
    }
    /* Compute CRC over the whole region */
    uint32_t computed_crc = crc32((const uint8_t *)addr, size);
    log_i("verify: expected=0x%08lX computed=0x%08lX", expected_crc, computed_crc);

    if (computed_crc != expected_crc) {
        log_e("verify failed: expected 0x%08lX got 0x%08lX", expected_crc, computed_crc);
        return BL_ERR_VERIFY;
    }

    return BL_ERR_OK;
}

static bl_errcode_t h_reset(bl_context_t *ctx)
{
    log_i("[handle] --> RESET");
    (void)ctx;
    /* Note: bl_protocol_dispatch will send response after handler returns.
     * We need to delay before reset to allow response to be sent. */
    const platform_desc_t *plat = ctx->proto->plat;
    plat->timer->delay_ms(2);
    plat->system->reset();
    return BL_ERR_OK;
}

static bl_errcode_t h_boot(bl_context_t *ctx)
{
    log_i("[handle] --> BOOT");
    (void)ctx;
    /* Boot command will be handled by bl_main_loop after dispatch */
    return BL_ERR_OK;
}

/* Byte handler */
static bool on_byte(bl_protocol_t *proto, uint8_t byte)
{
    uint32_t now = proto->plat->timer->get_ms();
    uint32_t elapsed = now - proto->last_byte_time;

    if (elapsed > proto->rx_timeout_ms) {
        reset_protocol(proto);
    }
    proto->last_byte_time = now;

    proto->buffer[proto->buffer_index++] = byte;

    switch (proto->state) {
        case BL_STATE_HEADER:
            if (byte == BL_PKT_HEADER_REQUEST) {
                proto->state = BL_STATE_OPCODE;
            } else {
                reset_protocol(proto);
            }
            break;

        case BL_STATE_OPCODE: {
            bl_opcode_t op = (bl_opcode_t)byte;
            switch (op) {
                case BL_OP_INQUERY: case BL_OP_ERASE: case BL_OP_PROGRAM:
                case BL_OP_VERIFY: case BL_OP_RESET: case BL_OP_BOOT:
                case BL_OP_WRITE_HEADER:
                    proto->opcode = op;
                    proto->state = BL_STATE_LENGTH;
                    break;
                default:
                    log_w("unknown opcode 0x%02X", byte);
                    reset_protocol(proto);
                    break;
            }
            break;
        }

        case BL_STATE_LENGTH:
            if (proto->buffer_index == BL_PKT_HEADER_SIZE + BL_PKT_OPCODE_SIZE + BL_PKT_LENGTH_SIZE) {
                uint16_t len = get_payload_len(proto);
                if (len <= BL_PKT_MAX_PAYLOAD) {
                    proto->payload_length = len;
                    proto->state = (len > 0) ? BL_STATE_PAYLOAD : BL_STATE_CRC;
                } else {
                    log_w("payload too large %u", len);
                    reset_protocol(proto);
                }
            }
            break;

        case BL_STATE_PAYLOAD:
            if (proto->buffer_index == BL_PKT_HEADER_SIZE + BL_PKT_OPCODE_SIZE + BL_PKT_LENGTH_SIZE + proto->payload_length) {
                proto->state = BL_STATE_CRC;
            }
            break;

        case BL_STATE_CRC:
            if (proto->buffer_index == BL_PKT_MIN_SIZE + proto->payload_length) {
                uint16_t crc_offset = BL_PKT_HEADER_SIZE + BL_PKT_OPCODE_SIZE + BL_PKT_LENGTH_SIZE + proto->payload_length;
                uint16_t stored_crc = get_u16(proto->buffer + crc_offset);
                uint16_t computed_crc = crc16(proto->buffer, crc_offset);
                if (stored_crc == computed_crc) {
                    return true;
                } else {
                    log_w("CRC mismatch");
                    reset_protocol(proto);
                }
            }
            break;

        default:
            reset_protocol(proto);
            break;
    }

    if (proto->buffer_index >= BL_PKT_MAX_SIZE) {
        log_w("buffer overflow");
        reset_protocol(proto);
    }

    return false;
}

/* Public API */
void bl_protocol_init(bl_protocol_t *proto, const platform_desc_t *plat)
{
    memset(proto, 0, sizeof(*proto));
    proto->plat = plat;
    proto->rx_timeout_ms = 1000;  /* 1000ms timeout to handle slow host sending */
    proto->last_byte_time = plat->timer->get_ms();

    reset_protocol(proto);

    bl_protocol_register(proto, BL_OP_INQUERY,   h_inquery);
    bl_protocol_register(proto, BL_OP_ERASE,     h_erase);
    bl_protocol_register(proto, BL_OP_PROGRAM,   h_program);
    bl_protocol_register(proto, BL_OP_VERIFY,    h_verify);
    bl_protocol_register(proto, BL_OP_RESET,     h_reset);
    bl_protocol_register(proto, BL_OP_BOOT,      h_boot);
    bl_protocol_register(proto, BL_OP_WRITE_HEADER, h_write_header);
}

bool bl_protocol_feed(bl_protocol_t *proto, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        if (on_byte(proto, data[i])) {
            return true;
        }
    }
    return false;
}

void bl_protocol_register(bl_protocol_t *proto, bl_opcode_t opcode, bl_handler_fn fn)
{
    proto->handlers[opcode] = fn;
}

void bl_protocol_dispatch(bl_protocol_t *proto)
{
    // log_i("proto: dispatch opcode=0x%02X", proto->opcode);

    s_ctx.proto = proto;
    s_ctx.payload = proto->buffer + BL_PKT_HEADER_SIZE + BL_PKT_OPCODE_SIZE + BL_PKT_LENGTH_SIZE;
    s_ctx.payload_length = proto->payload_length;
    s_ctx.response = s_response_buf;
    s_ctx.response_length = &s_response_len;
    s_ctx.errcode = BL_ERR_OK;
    s_ctx.resp_sent = false;

    bl_handler_fn handler = proto->handlers[proto->opcode];
    if (handler) {
        s_ctx.errcode = handler(&s_ctx);
    } else {
        s_ctx.errcode = BL_ERR_UNKNOWN;
    }

    /* Send response only if handler hasn't already sent one */
    if (!s_ctx.resp_sent) {
        bl_response(&s_ctx, proto->opcode, s_ctx.errcode, NULL, 0);
    }

    /* Always send the response if there's data to send */
    if (s_ctx.response_length > 0) {
        proto->plat->uart->write(s_ctx.response, *s_ctx.response_length);
    }

    reset_protocol(proto);
}
