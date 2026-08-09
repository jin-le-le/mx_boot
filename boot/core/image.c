/**
 * @file image.c
 * @brief Application image management implementation with security validation
 */

#include "image.h"
#include "board_config.h"
#include "utils.h"
#include "crc32.h"
#include "sha256.h"

#if BL_SIGNATURE_TYPE == 1
#include "uECC.h"
#include "ecdsa_pubkey.h"
#endif
#include <string.h>

/* Log macros - LOG_TAG and LOG_LVL must be defined before including elog.h */
#ifndef LOG_TAG
#define LOG_TAG "image"
#endif
#ifndef LOG_LVL
#define LOG_LVL ELOG_LVL_INFO
#endif
#include "elog.h"

static uint32_t s_header_addr;
static const flash_if_t *s_flash;

void bl_image_init(uint32_t header_addr, const flash_if_t *flash_if)
{
    s_header_addr = header_addr;
    s_flash = flash_if;
}

/**
 * Validate header by computing CRC over header fields (excluding the header_crc32 field itself).
 *
 * The header stored in flash at BL_MAGIC_HEADER_ADDR is a fixed 256-byte structure.
 * header_crc32 is at offset 248 (bytes 0-247 are covered by CRC).
 *
 * We read the CRC stored at offset 248 directly from flash memory, NOT via the struct,
 * because sizeof(bl_image_header_t) reports 252 instead of 256 due to struct alignment issues.
 */
static bool header_crc_validate(const bl_image_header_t *hdr)
{
    /* CRC covers bytes 0-247 (248 bytes), header_crc32 is at bytes 248-251 */
    enum { HEADER_CRC_OFFSET = 248, HEADER_CRC_FIELD_OFFSET = 248 };

    const uint8_t *hdr_bytes = (const uint8_t *)hdr;

    uint32_t computed = crc32(hdr_bytes, HEADER_CRC_OFFSET);

    /* Read stored CRC directly from flash memory to avoid struct alignment issues */
    uint32_t stored = hdr_bytes[HEADER_CRC_FIELD_OFFSET] |
                     (hdr_bytes[HEADER_CRC_FIELD_OFFSET + 1] << 8) |
                     (hdr_bytes[HEADER_CRC_FIELD_OFFSET + 2] << 16) |
                     (hdr_bytes[HEADER_CRC_FIELD_OFFSET + 3] << 24);
    return computed == stored;
}

/**
 * Check if address range is safe for Flash operations
 * Safe regions: App area only, never bootloader or header
 */
bool bl_image_is_addr_safe(uint32_t addr, uint32_t size)
{
    uint32_t end_addr = addr + size;

    /* Check basic bounds */
    if (addr < s_flash->base_addr) {
        log_e("addr 0x%08lX below flash base", addr);
        return false;
    }
    if (end_addr > s_flash->base_addr + s_flash->total_size) {
        log_e("addr 0x%08lX + size %lu beyond flash end", addr, (unsigned long)size);
        return false;
    }

    /* Check bootloader region protection */
    if (addr < BL_BOOTLOADER_END) {
        log_e("addr 0x%08lX in bootloader region", addr);
        return false;
    }

    /* Check header region protection - header starts at BL_MAGIC_HEADER_ADDR */
    /* App can write up to BL_MAGIC_HEADER_ADDR - 1, but we allow writing through header area */
    /* since header is only BL_HEADER_SIZE (256 bytes) and is written separately */
    /* The actual header write is protected by bl_image_write_header() */
    uint32_t header_start = BL_MAGIC_HEADER_ADDR;
    uint32_t header_end = header_start + BL_HEADER_SIZE;
    if (addr < header_end && end_addr > header_start) {
        log_e("addr range overlaps header region [0x%08lX, 0x%08lX)", header_start, header_end);
        return false;
    }

    return true;
}

/**
 * Verify image signature (ECDSA P-256 or none, by BL_SIGNATURE_TYPE)
 */
static bool verify_signature(const bl_image_header_t *hdr)
{
    if (!(hdr->flags & BL_IMAGE_FLAG_SIGNED)) {
#if BL_REJECT_UNSIGNED
        log_e("unsigned image rejected (BL_REJECT_UNSIGNED=1)");
        return false;
#else
        return true;   /* Dev mode: accept unsigned images */
#endif
    }

#if BL_SIGNATURE_TYPE == 1
    /* ===== ECDSA P-256 (asymmetric signature) ===== */
    uint8_t hash[32];
    sha256_compute((const uint8_t *)hdr->image_addr, hdr->image_size, hash);
    if (uECC_verify(ecdsa_public_key, hash, 32, hdr->signature, uECC_secp256r1()) != 1) {
        log_e("ECDSA signature verification failed");
        return false;
    }
    log_i("signature verified (ECDSA)");
    return true;

#else
    /* TYPE=0: no signature */
    return true;
#endif
}

/**
 * Check anti-rollback version
 */
static bool check_version(const bl_image_header_t *hdr)
{
#if BL_ANTIROLLBACK_ENABLED
    uint32_t version = hdr->version_major * 10000 + hdr->version_minor * 100 + hdr->version_build;

    if (version < BL_MIN_APP_VERSION) {
        log_e("image version %lu.%lu.%lu below minimum %lu",
              (unsigned long)hdr->version_major,
              (unsigned long)hdr->version_minor,
              (unsigned long)hdr->version_build,
              (unsigned long)BL_MIN_APP_VERSION);
        return false;
    }
    log_i("version check passed");
#endif
    return true;
}

/**
 * Full image validation with all security checks
 */
bl_image_err_t bl_image_validate_full(void)
{
    const bl_image_header_t *hdr = (const bl_image_header_t *)s_header_addr;

    /* 1. Magic check */
    if (hdr->magic != BL_IMAGE_MAGIC) {
        log_e("magic mismatch: expected 0x%08lX, got 0x%08lX",
              (uint32_t)BL_IMAGE_MAGIC, hdr->magic);
        return BL_IMAGE_ERR_MAGIC;
    }

    /* 2. Header CRC check */
    if (!header_crc_validate(hdr)) {
        log_e("header CRC error");
        return BL_IMAGE_ERR_HEADER_CRC;
    }

    /* 3. Address bounds check */
    if (!bl_image_is_addr_safe(hdr->image_addr, hdr->image_size)) {
        log_e("image address/size invalid");
        return BL_IMAGE_ERR_ADDR;
    }

    /* 4. Image CRC check */
    uint8_t *image_data = (uint8_t *)hdr->image_addr;
    uint32_t computed_crc = crc32(image_data, hdr->image_size);
    if (computed_crc != hdr->image_crc32) {
        log_e("image CRC error: expected 0x%08lX, got 0x%08lX",
              hdr->image_crc32, computed_crc);
        return BL_IMAGE_ERR_IMAGE_CRC;
    }

    /* 5. Version check (anti-rollback) */
    if (!check_version(hdr)) {
        return BL_IMAGE_ERR_VERSION;
    }

    /* 6. Signature verification (ECDSA, by BL_SIGNATURE_TYPE) */
    if (!verify_signature(hdr)) {
        return BL_IMAGE_ERR_SIGNATURE;
    }

    return BL_IMAGE_OK;
}

/* Simple validation for backward compatibility */
bool bl_image_validate(void)
{
    return bl_image_validate_full() == BL_IMAGE_OK;
}

const bl_image_header_t *bl_image_get_header(void)
{
    return (const bl_image_header_t *)s_header_addr;
}

uint32_t bl_image_get_address(void)
{
    return ((const bl_image_header_t *)s_header_addr)->image_addr;
}

uint32_t bl_image_get_size(void)
{
    return ((const bl_image_header_t *)s_header_addr)->image_size;
}

int bl_image_write_header(const uint8_t *data, size_t len)
{
    if (len != 256) {
        log_e("bl_image_write_header: invalid length %u, expected 256", (unsigned int)len);
        return -1;
    }

    s_flash->unlock();
    uint32_t sector_addr = s_header_addr & ~(s_flash->sector_size - 1);
    s_flash->erase(sector_addr, s_flash->sector_size);
    int ret = s_flash->program(s_header_addr, data, 256);
    s_flash->lock();

    if(ret != 0) {
        log_e("bl_image_write_header: flash program failed");
        return -1;
    }

    return ret;
}
