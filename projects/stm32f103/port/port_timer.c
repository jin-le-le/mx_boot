/**
 * @file port_timer.c
 * @brief STM32F103 Timer interface
 *
 * Uses SysTick (HAL_GetTick) for millisecond counting plus the TIM2 counter
 * for microsecond precision. Does not depend on the TIM2 interrupt, avoiding
 * dead loops caused by interrupt configuration issues.
 */

#include "../hal_if_defines.h"
#include "port.h"
#include "main.h"
#include "tim.h"
#include "board_config.h"

extern TIM_HandleTypeDef htim2;

static int port_timer_init(uint32_t period_us)
{
    (void)period_us;
    /* TIM2 does not need an interrupt; reading the counter is enough. MX_TIM2_Init has already initialized it. */
    HAL_TIM_Base_Start(&htim2);  /* Start TIM2 (no interrupt) */
    return 0;
}

static void port_timer_deinit(void)
{
    HAL_TIM_Base_Stop(&htim2);
}

/**
 * Millisecond count: directly use HAL's SysTick (proven to work by LED blinking)
 */
static uint32_t port_timer_get_ms(void)
{
    return HAL_GetTick();
}

/**
 * Microsecond count: SysTick milliseconds + TIM2 counter (1MHz = 1us/tick)
 */
static uint32_t port_timer_get_us(void)
{
    uint32_t ms = HAL_GetTick();
    uint32_t cnt = __HAL_TIM_GET_COUNTER(&htim2);
    return ms * 1000u + cnt;
}

static void port_timer_delay_us(uint32_t us)
{
    uint32_t start = port_timer_get_us();
    while ((port_timer_get_us() - start) < us);
}

static void port_timer_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);  /* Use HAL's SysTick delay; absolutely reliable */
}

static void port_timer_register_periodic_cb(void (*callback)(void))
{
    (void)callback;
}

const timer_if_t timer_stm32f1 = {
    .init                = port_timer_init,
    .deinit              = port_timer_deinit,
    .get_us              = port_timer_get_us,
    .get_ms              = port_timer_get_ms,
    .delay_us            = port_timer_delay_us,
    .delay_ms            = port_timer_delay_ms,
    .register_periodic_cb = port_timer_register_periodic_cb,
    .instance            = BL_TIMER_INSTANCE,
};
