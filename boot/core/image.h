/**
 * @file image.h
 * @brief Application image management with security validation
 */

#ifndef CORE_IMAGE_H
#define CORE_IMAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../hal_if_defines.h"

/*=======================================================================
 * Image Header (Magic Header)
 *=======================================================================*/
#define BL_IMAGE_MAGIC    0x4D414749  /* "MAGI" */
#define BL_IMAGE_VERSION  2

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint32_t header_version;
    uint32_t flags;
    uint32_t image_type;
    uint32_t image_addr;
    uint32_t image_size;
    uint32_t image_crc32;
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_build;
    uint8_t  signature[64];
    uint32_t min_hw_version;
    uint8_t  reserved[140];  /* Padding (actual compiled size may vary due to packing/alignment) */
    uint32_t header_crc32;    /* Offset 248 in 256-byte header flash area */
} bl_image_header_t;
#pragma pack(pop)

/* Image flags */
#define BL_IMAGE_FLAG_SIGNED     (1 << 0)
#define BL_IMAGE_FLAG_ENCRYPTED  (1 << 1)

/*=======================================================================
 * Image Validation Result
 *=======================================================================*/
typedef enum {
    BL_IMAGE_OK           = 0,
    BL_IMAGE_ERR_MAGIC    = 1,
    BL_IMAGE_ERR_HEADER_CRC = 2,
    BL_IMAGE_ERR_IMAGE_CRC = 3,
    BL_IMAGE_ERR_SIGNATURE = 4,
    BL_IMAGE_ERR_VERSION   = 5,
    BL_IMAGE_ERR_ADDR      = 6,
    BL_IMAGE_ERR_SIZE      = 7,
} bl_image_err_t;

/*=======================================================================
 * API
 *=======================================================================*/

/**
 * @brief Initialize image subsystem
 * @param header_addr Address where image header is stored
 * @param flash_if Pointer to flash interface for write operations
 */
void bl_image_init(uint32_t header_addr, const flash_if_t *flash_if);

/**
 * @brief Validate application image with full security checks
 * @return BL_IMAGE_OK if valid, error code otherwise
 */
bl_image_err_t bl_image_validate_full(void);

/**
 * @brief Simple validate for backward compatibility
 * @return true if image is valid, false otherwise
 */
bool bl_image_validate(void);

/**
 * @brief Get pointer to current image header
 * @return Pointer to header at stored header address
 */
const bl_image_header_t *bl_image_get_header(void);

/**
 * @brief Get application image address from header
 * @return Image address
 */
uint32_t bl_image_get_address(void);

/**
 * @brief Get application image size from header
 * @return Image size
 */
uint32_t bl_image_get_size(void);

/**
 * @brief Write image header to flash
 * @param data Pointer to 256-byte header data from PC tool
 * @param len Length of data (must be 256)
 * @return 0 on success, -1 on failure
 */
int bl_image_write_header(const uint8_t *data, size_t len);

/**
 * @brief Check if address is within safe bounds for Flash operations
 * @param addr Address to check
 * @param size Size of data
 * @return true if safe, false otherwise
 */
bool bl_image_is_addr_safe(uint32_t addr, uint32_t size);

#endif /* CORE_IMAGE_H */
