/**
 * @file bl_config.h
 * @brief Unified include entry
 *
 * Application code only needs #include "bl_config.h" to pull in all headers.
 * There is no need to separately include board_config.h / bl_main.h, etc.
 */

#ifndef BL_CONFIG_H
#define BL_CONFIG_H

/* Board + application configuration (board_config.h automatically includes bl_features.h at the end) */
#include "board_config.h"

/* HAL abstraction layer interface definitions (platform_desc_t, etc.) */
#include "hal_if_defines.h"

/* Logging library (when BL_LOG_ENABLED=0, all log_xxx macros are no-ops) */
#include "elog.h"

/* Bootloader core API */
#include "core/bl_main.h"
#include "core/image.h"
#include "core/protocol.h"

#endif /* BL_CONFIG_H */
