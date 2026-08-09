/**
 * @file bl_features.h
 * @brief Bootloader feature configuration (chip-independent)
 *
 * ====== Division of labor with board_config.h ======
 *
 * board_config.h: chip + board specific
 *   - Flash address layout (BL_FLASH_BASE / BL_APP_BASE, etc.)
 *   - UART/GPIO/Timer instance numbers
 *   - GPIO pin assignments (LED/KEY, etc.)
 *   - Chip-specific protection mechanisms (e.g. STM32 RDP Level)
 *   -> MUST be modified when porting to a new chip
 *
 * bl_features.h (this file): feature configuration
 *   - Protocol parameters (version number, payload size)
 *   - Boot behavior (delay time, trap trigger mode)
 *   - Security policy (ECDSA signature, anti-rollback)
 *   - Logging switches
 *   - CRC algorithm selection
 *   -> No need to modify when porting to a new chip; all chips share the same copy
 *
 * Include relationship:
 *   board_config.h automatically #includes "bl_features.h" at the end,
 *   so application code only needs #include "board_config.h" to get all macros.
 */

#ifndef BL_FEATURES_H
#define BL_FEATURES_H

/*=======================================================================
 * Protocol Parameters
 *=======================================================================*/
/* F103C8T6 has only 20KB of RAM, buffers must be small */
#define BL_RX_BUFFER_SIZE     (2 * 1024)       /* F407=5120, F103 reduced to 2KB */
#define BL_MAX_PAYLOAD_SIZE   (1024 + 8)       /* F407=4104, F103 reduced to 1KB */

/*=======================================================================
 * Boot Behavior (Timing & Trap)
 *=======================================================================*/
/* Boot delay (trap window)
 *   500ms matches the host's 100ms polling period, hit rate ~= 100%
 *   Production products that want fast boot can change to 200ms (requires host pre-armed)
 *   For development/debugging, can be changed to 3000ms (plenty of human reaction time)*/
#define BL_BOOT_DELAY_MS      500

/* Protocol RX timeout
 *   Reset the protocol state machine if the inter-packet interval exceeds this value
 *   1s is enough for slow hosts to respond */
#define BL_RX_TIMEOUT_MS      1000

/* RX Trap switch (whether to listen for UART data to trigger a trap)
 *   1 = enabled (detect host protocol packets during the boot window)
 *   0 = disabled (rely only on KEY trap or auto-jump)*/
#define BL_ENABLE_RX_TRAP     1

/* RX Trap trigger pattern
 *   By default matches the fixed 5-byte header of the GetDeviceInfo packet:
 *     [0xAA] [0x01] [0x01 0x00] [0x00]
 *      Header  INQUERY  len=1     sub=DEVICE_INFO
 *
 *   Probability of false trigger from noise (1/256)^5 ~= 10^-12, essentially zero.
 *
 *   Custom examples:
 *     - Simple trigger (any protocol header): {0xAA}, 1
 *     - Strict trigger (current default)    : {0xAA, 0x01, 0x01, 0x00, 0x00}, 5
 *     - Magic string                        : {'M','C','U','B','O','O','T'}, 7
 *     - Disable RX trap                     : set BL_ENABLE_RX_TRAP to 0 */
#define BL_RX_TRAP_PATTERN       { 0xAA, 0x01, 0x01, 0x00, 0x00 }
#define BL_RX_TRAP_PATTERN_LEN   5

/*=======================================================================
 * Security Configuration
 *=======================================================================*/

/* Signature type selection (decided at compile time)
 *
 *   0 = No signature (CRC32 only for integrity, no anti-forgery)
 *       Suitable for: first porting to a new chip, development/debugging
 *
 *   1 = ECDSA P-256 (asymmetric signature) - recommended for production
 *       Code ~3.5 KB (micro-ecc), 64B public key compiled into the bootloader
 *       Suitable for: commercial products, scenarios requiring distribution to customers
 *       The host doesn't need a key; the vendor signs offline with a Python script
 */
#define BL_SIGNATURE_TYPE       1

/* Production mode: whether to reject unsigned firmware
 *   0 = Development mode: unsigned firmware can be flashed (convenient for debugging)
 *   1 = Production mode: a signature is required to flash (anti-forgery)
 *
 *   Make sure to set to 1 before mass production! Otherwise an attacker can craft
 *   firmware with flags=0 to bypass signature verification */
#define BL_REJECT_UNSIGNED      1

/* Anti-rollback (prevents downgrade to older firmware versions) */
#define BL_ANTIROLLBACK_ENABLED  1
#define BL_MIN_APP_VERSION       1   /* Minimum allowed App version number */

/*=======================================================================
 * CRC Implementation Selection
 *=======================================================================*
 * Choose the CRC32 / CRC16 implementation (pick one of three):
 *
 *   MODE = 0   Bitwise calculation
 *             - Flash: 0 tables, ~80 B code
 *             - Speed: slowest (500 KB ~300 ms)
 *
 *   MODE = 1   Full-byte table lookup
 *             - Flash: CRC32 table 1 KB + CRC16 table 512 B = 1.5 KB
 *             - Speed: fastest (500 KB ~12 ms)
 *
 *   MODE = 2   Nibble table lookup (recommended)
 *             - Flash: CRC32 table 64 B + CRC16 table 32 B = 96 B
 *             - Speed: medium (500 KB ~25 ms)
 *
 * All three implementations produce identical output, transparently interchangeable
 * between host and device.
 *
 * Suggested configuration strategy:
 *   - Development/debugging   -> MODE=1 (full table, fastest)
 *   - Production balance (recommended) -> MODE=2 (nibble table, small size + fast enough)
 *   - Minimal size           -> MODE=0 (bitwise, 0 tables)
 */
#define CRC32_MODE   0   /* 0=bitwise 1=full table(1KB) 2=nibble table(64B) */
#define CRC16_MODE   0   /* 0=bitwise 1=full table(512B) 2=nibble table(32B) */

/*=======================================================================
 * Logging (EasyLogger)
 *=======================================================================*
 * BL_LOG_ENABLED = 1 -> Enable logging (development/debug)
 * BL_LOG_ENABLED = 0 -> Disable logging (production), all log_xxx macros become no-ops,
 *                     EasyLogger code is fully stripped by --gc-sections, zero size overhead
 *
 * BL_LOG_LEVEL: log level filter
 *   0=ASSERT, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=VERBOSE
 *
 * BL_LOG_COLOR: VT100 color output (requires an ANSI-capable terminal such as MobaXterm/PuTTY)
 *   1 = colored (+~500 B Flash)
 *   0 = plain text (compatible with all terminals, saves ~500 B)
 *
 * BL_LOG_LINE: output line number
 *   1 = show source line (+~200 B Flash)
 *   0 = hide (saves ~200 B)
 */
#define BL_LOG_ENABLED          0
#define BL_LOG_LEVEL            3
#define BL_LOG_COLOR            0   /* 1=color (+500B); 0=plain text */
#define BL_LOG_LINE             1   /* 1=line number (+200B); 0=hidden */

/*=======================================================================
 * EasyLogger enable mapping (automatic, users do not need to manage)
 *=======================================================================
 * The BL_LOG_* macros in bl_features.h are automatically mapped to EasyLogger's ELOG_* macros.
 * Users only need to change BL_LOG_COLOR / BL_LOG_LINE above; no need to dig into elog_cfg.h.
 *
 * Note: EasyLogger uses #ifdef to detect the following macros (defined=enabled, undefined=disabled):
 *   ELOG_OUTPUT_ENABLE  -> controlled by BL_LOG_ENABLED
 *   ELOG_COLOR_ENABLE   -> controlled by BL_LOG_COLOR
 *   ELOG_FMT_USING_LINE -> controlled by BL_LOG_LINE
 */
#if BL_LOG_ENABLED
    #define ELOG_OUTPUT_ENABLE
    #define ELOG_OUTPUT_LVL      BL_LOG_LEVEL
    #if BL_LOG_COLOR
        #define ELOG_COLOR_ENABLE
    #endif
    #if BL_LOG_LINE
        #define ELOG_FMT_USING_LINE
    #endif
#endif

#endif /* BL_FEATURES_H */
