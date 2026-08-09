/**
 * @file bl_features.h
 * @brief Bootloader feature configuration (chip-independent)
 *
 * ====== Division of labor with board_config.h ======
 *
 * board_config.h: chip + board level
 *   - Flash address layout (BL_FLASH_BASE / BL_APP_BASE, etc.)
 *   - UART/GPIO/Timer instance numbers
 *   - GPIO pin assignment (LED/KEY, etc.)
 *   - Chip-specific protection mechanisms (e.g. STM32 RDP Level)
 *   → **Must change** when porting to a new chip
 *
 * bl_features.h (this file): feature related
 *   - Protocol parameters (version number, payload size)
 *   - Boot behavior (delay, trap trigger mode)
 *   - Security policy (ECDSA signature, anti-rollback)
 *   - Logging switches
 *   - CRC algorithm selection
 *   → **No change required** when porting to a new chip; all chips share the same copy
 *
 * Include relationship:
 *   board_config.h automatically #includes "bl_features.h" at the end
 *   So application code only needs #include "board_config.h" to access all macros
 */

#ifndef BL_FEATURES_H
#define BL_FEATURES_H

/*=======================================================================
 * Protocol Parameters
 *=======================================================================*/
/* HC32F460 RAM is generous at 188KB; large buffers can be used to speed up upgrades */
#define BL_RX_BUFFER_SIZE     (5 * 1024)       /* Consistent with F407 */
#define BL_MAX_PAYLOAD_SIZE   (4096 + 8)       /* Consistent with F407 */

/*=======================================================================
 * Boot Behavior (Timing & Trap)
 *=======================================================================*/
/* Boot delay (trap window)
 *   500ms matches the host 100ms polling period, hit rate ≈ 100%
 *   For mass-produced products that need fast boot, set to 200ms (host must be pre-armed)
 *   For development/debugging, set to 3000ms (comfortable human reaction time) */
#define BL_BOOT_DELAY_MS      500

/* Protocol RX timeout
 *   If the gap between packets exceeds this value, the protocol state machine is reset
 *   1s is sufficient for slow hosts to respond */
#define BL_RX_TIMEOUT_MS      1000

/* RX Trap switch (whether to listen on UART to trigger the trap)
 *   1 = enabled (detect host protocol packets during the boot window)
 *   0 = disabled (rely only on KEY trap or automatic jump) */
#define BL_ENABLE_RX_TRAP     1

/* RX Trap trigger pattern
 *   Default matches the fixed 5-byte header of the GetDeviceInfo packet:
 *     [0xAA] [0x01] [0x01 0x00] [0x00]
 *      Head    INQUERY  len=1     sub=DEVICE_INFO
 *
 *   Probability of false trigger from noise (1/256)^5 ≈ 10^-12, essentially zero.
 *
 *   Customization examples:
 *     - Simple trigger (any protocol header): {0xAA}, 1
 *     - Strict trigger (current default)     : {0xAA, 0x01, 0x01, 0x00, 0x00}, 5
 *     - Magic string                         : {'M','C','U','B','O','O','T'}, 7
 *     - Disable RX trap                      : set BL_ENABLE_RX_TRAP to 0 */
#define BL_RX_TRAP_PATTERN       { 0xAA, 0x01, 0x01, 0x00, 0x00 }
#define BL_RX_TRAP_PATTERN_LEN   5

/*=======================================================================
 * Security Configuration
 *=======================================================================*/

/* Signature type selection (decided at compile time)
 *
 *   0 = No signature (only CRC32 integrity check, no anti-forgery)
 *       Suitable for: first porting to a new chip, development and debugging
 *
 *   1 = ECDSA P-256 (asymmetric signature) ★ Recommended for production
 *       Code ~3.5 KB (micro-ecc), 64B public key compiled into the bootloader
 *       Suitable for: commercial products, scenarios that require distribution to customers
 *       The host tool requires no secret key; the manufacturer signs offline with a Python script
 */
#define BL_SIGNATURE_TYPE       1

/* Production mode: whether to reject unsigned firmware
 *   0 = Development mode: unsigned firmware can be flashed (convenient for debugging)
 *   1 = Production mode: a valid signature is required to flash (anti-forgery)
 *
 * ⚠️ Make sure to set this to 1 before mass production! Otherwise an attacker can craft
 *    firmware with flags=0 to bypass signature verification */
#define BL_REJECT_UNSIGNED      1

/* Anti-rollback (prevent downgrading to older firmware versions) */
#define BL_ANTIROLLBACK_ENABLED  1
#define BL_MIN_APP_VERSION       1   /* Minimum allowed App version number */

/*=======================================================================
 * CRC Implementation Selection
 *=======================================================================*
 * Select the CRC32 / CRC16 implementation (choose one of three):
 *
 *   MODE = 0   Bitwise computation
 *             - Flash: 0 tables, ~80 B of code
 *             - Speed: slowest (500 KB ~300 ms)
 *
 *   MODE = 1   Full-byte table lookup
 *             - Flash: CRC32 table 1 KB + CRC16 table 512 B = 1.5 KB
 *             - Speed: fastest (500 KB ~12 ms)
 *
 *   MODE = 2   Nibble table lookup (recommended ★)
 *             - Flash: CRC32 table 64 B + CRC16 table 32 B = 96 B
 *             - Speed: medium (500 KB ~25 ms)
 *
 * The three implementations produce **identical output** and are interchangeable
 * between device and host transparently.
 *
 * Recommended configuration strategy:
 *   - Development/debugging       → MODE=1 (full table, fastest)
 *   - Balanced production (rec.)  → MODE=2 (nibble table, small size + fast enough)
 *   - Minimum footprint           → MODE=0 (bitwise, 0 tables)
 */
#define CRC32_MODE   0   /* 0=bitwise 1=full table (1KB) 2=nibble table (64B) */
#define CRC16_MODE   0   /* 0=bitwise 1=full table (512B) 2=nibble table (32B) */

/*=======================================================================
 * Logging (EasyLogger)
 *=======================================================================*
 * BL_LOG_ENABLED = 1 → Logging enabled (development/debug)
 * BL_LOG_ENABLED = 0 → Logging disabled (production); all log_xxx macros become no-ops,
 *                     EasyLogger code is fully stripped by --gc-sections with 0 size overhead
 *
 * BL_LOG_LEVEL: log level filter
 *   0=ASSERT, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=VERBOSE
 *
 * BL_LOG_COLOR: VT100 color output (requires an ANSI-capable terminal such as MobaXterm/PuTTY)
 *   1 = Colored (~500 B more Flash)
 *   0 = Plain text (compatible with all terminals, ~500 B saved)
 *
 * BL_LOG_LINE: output source line number
 *   1 = Show source line (~200 B more Flash)
 *   0 = Hide (~200 B saved)
 */
#define BL_LOG_ENABLED          0
#define BL_LOG_LEVEL            3
#define BL_LOG_COLOR            0   /* 1=colored (+500B); 0=plain text */
#define BL_LOG_LINE             1   /* 1=line number (+200B); 0=hidden */

/*=======================================================================
 * EasyLogger enable mapping (automatic, no user action required)
 *=======================================================================
 * The BL_LOG_* macros in bl_features.h are automatically mapped to EasyLogger ELOG_* macros.
 * Users only need to change BL_LOG_COLOR / BL_LOG_LINE above; no need to edit elog_cfg.h.
 *
 * Note: EasyLogger uses #ifdef to detect the following macros (defined=enabled, undefined=disabled):
 *   ELOG_OUTPUT_ENABLE  → controlled by BL_LOG_ENABLED
 *   ELOG_COLOR_ENABLE   → controlled by BL_LOG_COLOR
 *   ELOG_FMT_USING_LINE → controlled by BL_LOG_LINE
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
