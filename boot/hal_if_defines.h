/**
 * @file hal_if_defines.h
 * @brief Hardware Abstraction Layer interface definitions
 */

#ifndef HAL_IF_DEFINES_H
#define HAL_IF_DEFINES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=======================================================================
 * Flash Interface
 *=======================================================================*/
typedef struct flash_if {
    int   (*init)(void);
    void  (*deinit)(void);
    int   (*unlock)(void);
    int   (*lock)(void);
    int   (*erase)(uint32_t addr, uint32_t size);
    int   (*program)(uint32_t addr, const uint8_t *data, uint32_t size);
    int   (*read)(uint32_t addr, uint8_t *data, uint32_t size);
    uint32_t base_addr;
    uint32_t total_size;
    uint32_t sector_size;
    uint32_t program_granularity;
} flash_if_t;

/*=======================================================================
 * UART Interface
 *=======================================================================*/
typedef struct uart_if {
    int   (*init)(uint32_t baudrate);
    void  (*deinit)(void);
    int   (*write)(const uint8_t *data, uint32_t size);
    int   (*read)(uint8_t *data, uint32_t size, uint32_t timeout_ms);
    void  (*register_rx_cb)(void (*callback)(const uint8_t *data, uint32_t size));
    uint32_t instance;
} uart_if_t;

/*=======================================================================
 * GPIO Interface
 *=======================================================================*/
typedef struct gpio_if {
    int   (*init)(uint8_t port, uint16_t pin, uint8_t mode, uint8_t pupd);
    void  (*set)(uint8_t port, uint16_t pin, bool level);
    bool  (*read)(uint8_t port, uint16_t pin);
    void  (*deinit)(uint8_t port, uint16_t pin);
} gpio_if_t;

/* GPIO mode values
 * Use the BL_ prefix to avoid name clashes with STM32 HAL macros such as
 * GPIO_MODE_ANALOG. HAL's GPIO_MODE_ANALOG = MODE_ANALOG = 3 has the same
 * numeric value as ours, but the name collision would trigger a
 * -Wmacro-redefined warning. The prefix keeps them cleanly separated. */
#define BL_GPIO_MODE_IN      0
#define BL_GPIO_MODE_OUT     1
#define BL_GPIO_MODE_AF      2
#define BL_GPIO_MODE_ANALOG  3

/* GPIO pull-up/pull-down values */
#define GPIO_PUPD_NONE     0
#define GPIO_PUPD_PULLUP   1
#define GPIO_PUPD_PULLDOWN 2

/*=======================================================================
 * Timer Interface
 *=======================================================================*/
typedef void (*tim_periodic_callback_t)(void);

typedef struct timer_if {
    int   (*init)(uint32_t period_us);
    void  (*deinit)(void);
    uint32_t (*get_us)(void);
    uint32_t (*get_ms)(void);
    void  (*delay_us)(uint32_t us);
    void  (*delay_ms)(uint32_t ms);
    void  (*register_periodic_cb)(tim_periodic_callback_t callback);
    uint32_t instance;
} timer_if_t;

/*=======================================================================
 * System Interface
 *=======================================================================*/
typedef struct system_if {
    void  (*reset)(void);
    void  (*set_vtor)(uint32_t addr);
    void  (*disable_irq)(uint8_t irq_num);
    void  (*enable_irq)(uint8_t irq_num);
    void  (*get_unique_id)(uint8_t *id);
    void  (*jump)(uint32_t sp, uint32_t pc);
    /* Disable all interrupts and clear pending IRQs before jumping to app.
     * Encapsulates chip-specific operations (e.g. NVIC->ICER[] on Cortex-M,
     * CSR manipulation on RISC-V). Each port must implement this. */
    void  (*disable_all_irq)(void);
} system_if_t;

/*=======================================================================
 * Platform Descriptor
 *=======================================================================*/
typedef struct platform_desc {
    const flash_if_t   *flash;
    const uart_if_t    *uart;            /* UART for upgrade/protocol (required) */
    const uart_if_t    *console_uart;    /* UART for log output (may be NULL if no logging) */
    const gpio_if_t    *gpio;
    const timer_if_t   *timer;
    const system_if_t  *system;
    const char *chip_name;
    uint32_t chip_id;
    const char *bl_version;
} platform_desc_t;

const platform_desc_t *platform_get(void);
int platform_init(const platform_desc_t *desc);

#ifdef __cplusplus
}
#endif

#endif /* HAL_IF_DEFINES_H */
