/**
 * @file bl_main.h
 * @brief Bootloader main state machine
 */

#ifndef CORE_BL_MAIN_H
#define CORE_BL_MAIN_H

#include "../hal_if_defines.h"
#include "protocol.h"
#include <stdbool.h>

/* Bootloader configuration */
typedef struct {
    uint32_t boot_delay_ms;
    uint32_t rx_timeout_ms;
    uint32_t app_vtor;
    uint32_t magic_header_addr;
    bool enable_key_trap;
    bool enable_rx_trap;
    uint8_t key_port;
    uint16_t key_pin;
} bl_config_t;

/* Bootloader result */
typedef enum {
    BL_RESULT_OK       = 0,
    BL_RESULT_ERROR    = 1,
    BL_RESULT_TRAPPED  = 2,
    BL_RESULT_UPDATE   = 3,
} bl_result_t;

/*=======================================================================
 * Simplified API (recommended for users)
 *=======================================================================
 * Integrate the bootloader from main.c with just two calls:
 *
 *     boot_init();   // one-shot init (elog + image + bl_main_init)
 *     boot_run();    // start the bootloader (never returns)
 *
 * Internally pulls platform_get() and the board_config.h defaults, so no
 * manual parameters are required.
 *
 * Example:
 *   int main(void) {
 *       HAL_Init();
 *       SystemClock_Config();
 *       MX_GPIO_Init();
 *       MX_TIM6_Init();
 *       MX_USART1_UART_Init();
 *       MX_USART3_UART_Init();
 *
 *       boot_init();     // one-line init
 *       boot_run();      // one-line start (never returns)
 *   }
 */

/**
 * @brief One-shot bootloader initialization
 *
 * Internally:
 *   1. Initialize EasyLogger (when BL_LOG_ENABLED=1)
 *   2. Initialize image subsystem (bl_image_init)
 *   3. Build config from board_config.h defaults
 *   4. Call bl_main_init to inject platform and config
 */
void boot_init(void);

/**
 * @brief Bootloader main run loop (never returns)
 *
 * Internally:
 *   1. Call bl_main() for the boot decision (trap check, image validation,
 *      jump attempt)
 *   2. On trap (user entered upgrade mode or image invalid) -> call
 *      bl_main_loop() to handle the protocol
 *   3. Never returns: stays in the protocol loop on trap, or has already
 *      jumped to the app otherwise
 */
void boot_run(void);

/*=======================================================================
 * Advanced API (optional, for custom scenarios)
 *=======================================================================
 * For custom config or platform, skip boot_init/boot_run and call the
 * lower-level interfaces directly:
 *
 *   bl_main_init(&custom_config, &custom_plat);
 *   bl_main();
 *   if (...) bl_main_loop();
 */

/**
 * @brief Initialize bootloader with configuration and platform descriptor
 * @param config Bootloader configuration
 * @param plat Platform descriptor (GPIO/UART/Timer/Flash/System interfaces)
 */
void bl_main_init(const bl_config_t *config, const platform_desc_t *plat);

/**
 * @brief Execute boot decision logic
 * @return BL_RESULT_TRAPPED if entering update mode, BL_RESULT_OK if jumping to app
 */
bl_result_t bl_main(void);

/**
 * @brief Enter command loop to handle upgrade protocol
 */
void bl_main_loop(void);

/**
 * @brief Jump to application
 * @param app_addr Application vector table address
 */
void bl_jump_to_app(uint32_t app_addr);

#endif /* CORE_BL_MAIN_H */
