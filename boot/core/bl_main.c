/**
 * @file bl_main.c
 * @brief Bootloader main implementation
 */

#include "bl_main.h"
#include "image.h"
#include "board_config.h"
#include "ringbuffer.h"
#include <string.h>

/* Log macros - LOG_TAG and LOG_LVL must be defined before including elog.h */
#ifndef LOG_TAG
#define LOG_TAG "bl"
#endif
#ifndef LOG_LVL
#define LOG_LVL ELOG_LVL_INFO
#endif
#include "elog.h"

static bl_config_t s_config;
static const platform_desc_t *s_plat;
static bl_protocol_t s_proto_instance;
static bl_protocol_t *s_proto;

static uint8_t s_rx_buffer[BL_RX_BUFFER_SIZE];
static rb_t s_rx_rb;

/* RX trap detection: pattern-match the GetDeviceInfo header (configurable via
 * BL_RX_TRAP_PATTERN in board_config.h).
 *
 * Default pattern [AA 01 01 00 00] matches:
 *   AA        = protocol frame header
 *   01        = opcode INQUERY
 *   01 00     = payload length = 1 (little-endian)
 *   00        = sub-command DEVICE_INFO
 *
 * Random noise matching all 5 bytes has probability (1/256)^5 ~ 10^-12,
 * effectively eliminating false trap triggers from line noise.
 *
 * The upper computer's polling naturally sends this exact pattern as its
 * first action, so legitimate trap triggering is unchanged. */
static const uint8_t TRAP_PATTERN[] = BL_RX_TRAP_PATTERN;
#define TRAP_PATTERN_LEN  BL_RX_TRAP_PATTERN_LEN
static uint8_t s_trap_match_idx;
static volatile bool s_proto_activity;

static void uart_rx_cb(const uint8_t *data, uint32_t len)
{
    /* Stream pattern matcher: tracks consecutive matching bytes across
     * callbacks. On mismatch, restarts if current byte could be the start
     * of a new pattern (handles back-to-back or overlapping candidates). */
    for (uint32_t i = 0; i < len; i++) {
        if (data[i] == TRAP_PATTERN[s_trap_match_idx]) {
            s_trap_match_idx++;
            if (s_trap_match_idx >= TRAP_PATTERN_LEN) {
                s_proto_activity = true;
                s_trap_match_idx = 0;
                break;
            }
        } else if (data[i] == TRAP_PATTERN[0]) {
            /* Mismatch, but this byte could be start of a new match */
            s_trap_match_idx = 1;
        } else {
            s_trap_match_idx = 0;
        }
    }

    uint32_t written = rb_puts(&s_rx_rb, data, len);
    if (written < len) {
        log_w("ringbuffer full, dropped %lu bytes", (unsigned long)(len - written));
    }
}

static bool check_trap_conditions(void)
{
    uint32_t delay_ms = s_config.boot_delay_ms;
    uint32_t remaining = delay_ms;

#if BL_ENABLE_KEY_TRAP
    if (s_config.enable_key_trap) {
        /* KEY pressed = low level (PULLUP mode), so use !read() */
        if (!s_plat->gpio->read(s_config.key_port, s_config.key_pin)) {
            log_i("[BOOT] key pressed, stay in bootloader");
            return true;
        }
    }
#endif

    uint32_t start = s_plat->timer->get_ms();

    /* Print initial countdown ("Ns") before the loop, since the first 100ms
     * delay inside the loop would otherwise eat the top second. */
    uint32_t last_sec = delay_ms / 1000;
    log_i("[BOOT] boot in %lus... (key+rx to stay)", (unsigned long)last_sec);

    while (remaining > 0) {
        s_plat->timer->delay_ms(100);

#if BL_ENABLE_KEY_TRAP
        if (s_config.enable_key_trap) {
            if (!s_plat->gpio->read(s_config.key_port, s_config.key_pin)) {
                log_i("[BOOT] key pressed, stay in bootloader");
                return true;
            }
        }
#endif

        if (s_config.enable_rx_trap) {
            if (s_proto_activity) {
                log_i("[BOOT] GetDeviceInfo packet detected, stay in bootloader");

                return true;
            }
        }

        uint32_t elapsed = s_plat->timer->get_ms() - start;
        remaining = (elapsed >= delay_ms) ? 0 : (delay_ms - elapsed);

        /* Fire when the remaining-time's integer second changes. Tolerant to
         * delay_ms(100) jitter — never misses a second, never double-prints. */
        uint32_t cur_sec = remaining / 1000;
        if (cur_sec != last_sec) {
            log_i("[BOOT] boot in %lus... (key+rx to stay)", (unsigned long)cur_sec);
            last_sec = cur_sec;
        }
    }

    return false;
}

void bl_main_init(const bl_config_t *config, const platform_desc_t *plat)
{
    memcpy(&s_config, config, sizeof(s_config));
    s_plat = plat;

    /* Platform integrity check (one-shot, avoids per-call NULL checks).
     * Required: flash / uart / timer / system (boot cannot work without any).
     * Optional: console_uart (NULL means no log, boot still runs). */
#if BL_LOG_ENABLED
    if (s_plat == NULL) {
        /* Without elog we cannot log; spin so the debugger can catch it */
        while (1) {}
    }
    if (s_plat->flash == NULL)  log_e("platform: flash is NULL");
    if (s_plat->uart == NULL)   log_e("platform: uart is NULL");
    if (s_plat->timer == NULL)  log_e("platform: timer is NULL");
    if (s_plat->system == NULL) log_e("platform: system is NULL");
#else
    /* Production no-log mode: skip checks to save code size.
     * A misconfigured port will HardFault and is debuggable via SWD. */
#endif
}

bl_result_t bl_main(void)
{
    /* ====== Port init hooks ======
     * Each port's init function is invoked as a startup hook. The init body
     * can be:
     *   - empty (relies on CubeMX / startup code to configure hardware)
     *   - a real init (self-contained port that does not depend on CubeMX)
     *   - internal state registration (e.g. uart registers its RX callback)
     * The component does not enforce a specific form; pick what fits the
     * project.
     *
     * Order: timer -> flash -> uart
     *   - timer first: subsequent delay / log timestamps depend on it
     *   - flash next: needed by image validation
     *   - uart last: both logging and protocol depend on it; placing it last
     *     surfaces earlier failures sooner
     *
     * gpio and system have no unified init:
     *   - gpio->init needs port/pin/mode/pupd args, called at each use site
     *   - system_if has no init field (reset/set_vtor etc. are on-demand)
     */
    s_plat->timer->init(0);                  /* period_us is ignored by most ports */
    s_plat->flash->init();
    s_plat->uart->init(BL_UART_BAUDRATE);
    s_plat->uart->register_rx_cb(uart_rx_cb);

    /* Initialize log console if the platform registered a console_uart */
    if (s_plat->console_uart && s_plat->console_uart->init) {
        s_plat->console_uart->init(BL_UART_BAUDRATE);
    }

    log_i("[BOOT] started");

    /* Initialize ringbuffer */
    s_rx_rb = rb_new(s_rx_buffer, BL_RX_BUFFER_SIZE);

    /* Initialize protocol */
    bl_protocol_init(&s_proto_instance, s_plat);
    s_proto = &s_proto_instance;

    /* Check if we should stay in bootloader */
    if (check_trap_conditions()) {
        return BL_RESULT_TRAPPED;
    }

    /* No trap -> try to boot application */
    if (bl_image_validate()) {
        bl_jump_to_app(s_config.app_vtor);
        return BL_RESULT_OK;
    } else {
        return BL_RESULT_TRAPPED;
    }
}

void bl_main_loop(void)
{
    bl_protocol_t *proto = s_proto;

    log_i("[BOOT] ready");

    while (1) {
        while (!rb_empty(&s_rx_rb)) {
            uint8_t byte;
            rb_get(&s_rx_rb, &byte);
            if (bl_protocol_feed(proto, &byte, 1)) {
                bl_protocol_dispatch(proto);

                if (proto->opcode == BL_OP_BOOT) {
                    if (bl_image_validate()) {
                        bl_jump_to_app(s_config.app_vtor);
                    }
                }
            }
        }

        s_plat->timer->delay_ms(1);
    }
}

void bl_jump_to_app(uint32_t app_addr)
{
    /* Disable global IRQ and clear all NVIC enable/pending bits.
     * Wrapped in port layer disable_all_irq for portability across chips. */
    s_plat->system->disable_all_irq();

    uint32_t *vectors = (uint32_t *)app_addr;
    uint32_t sp = vectors[0];
    uint32_t pc = vectors[1];

    /* PC must fall within [App base, Flash end].
     * Uses board_config.h macros so the check adapts to other chips. */
    if (pc < BL_APP_BASE || pc > (BL_FLASH_BASE + BL_FLASH_TOTAL)) {
        log_e("[BOOT] invalid PC!");
        return;
    }

    s_plat->system->set_vtor(app_addr);
    s_plat->system->jump(sp, pc);
}

/*=======================================================================
 * Simplified API (boot_init / boot_run)
 *=======================================================================*/

void boot_init(void)
{
    const platform_desc_t *plat = platform_get();

    /* ====== 1. EasyLogger init (only when BL_LOG_ENABLED=1) ====== */
#if BL_LOG_ENABLED
    elog_init();
    /* Per-level output format:
     *   BL_LOG_LINE=1 -> I/bl       (91) [BOOT] started
     *   BL_LOG_LINE=0 -> I/bl       [BOOT] started */
#if BL_LOG_LINE
    #define BL_LOG_FMT_SET  (ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_LINE)
#else
    #define BL_LOG_FMT_SET  (ELOG_FMT_LVL | ELOG_FMT_TAG)
#endif
    elog_set_fmt(ELOG_LVL_ASSERT,  BL_LOG_FMT_SET);
    elog_set_fmt(ELOG_LVL_ERROR,   BL_LOG_FMT_SET);
    elog_set_fmt(ELOG_LVL_WARN,    BL_LOG_FMT_SET);
    elog_set_fmt(ELOG_LVL_INFO,    BL_LOG_FMT_SET);
    elog_set_fmt(ELOG_LVL_DEBUG,   BL_LOG_FMT_SET);
    elog_set_fmt(ELOG_LVL_VERBOSE, BL_LOG_FMT_SET);
    elog_start();
#endif

    /* ====== 2. Image subsystem init ====== */
    bl_image_init(BL_MAGIC_HEADER_ADDR, plat->flash);

    /* ====== 3. Build default config from board_config.h ====== */
    bl_config_t config = {
        .boot_delay_ms     = BL_BOOT_DELAY_MS,
        .rx_timeout_ms     = BL_RX_TIMEOUT_MS,
        .app_vtor          = BL_APP_BASE,
        .magic_header_addr = BL_MAGIC_HEADER_ADDR,
#if BL_ENABLE_KEY_TRAP
        .enable_key_trap   = true,
        .key_port          = BL_KEY_TRAP_PORT,
        .key_pin           = BL_KEY_TRAP_PIN,
#else
        .enable_key_trap   = false,
        .key_port          = 0,
        .key_pin           = 0,
#endif
        .enable_rx_trap    = BL_ENABLE_RX_TRAP,
    };

    /* ====== 4. Inject into bootloader core ====== */
    bl_main_init(&config, plat);
}

void boot_run(void)
{
    bl_result_t result = bl_main();
    if (result == BL_RESULT_TRAPPED) {
        bl_main_loop();   /* Never returns (sits in protocol loop) */
    }
    /* If bl_main returns OK/ERROR, jump failed (should not happen).
     * Just return and let main.c's while(1) trap. */
}
