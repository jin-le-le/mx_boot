/**
 * @file port_timer.c
 * @brief HC32F460 Timer interface (uses SysTick + DDL_DelayMS)
 *
 * Same strategy as F103: no TIM peripheral, use the SysTick-equivalent of HAL_GetTick.
 * HC32 does not have HAL_GetTick, but DDL_DelayMS uses SysTick internally, which is sufficient.
 */

#include "../hal_if_defines.h"
#include "port.h"

/* Simple ms counter (accumulated using SysTick COUNTFLAG) */
static volatile uint32_t s_ms_counter = 0;
static bool s_inited = false;

static void SysTick_Setup_1ms(void)
{
    /* Configure SysTick for a 1ms period and enable the interrupt */
    SysTick_Config(SystemCoreClock / 1000UL);
}

/* SysTick interrupt handler (called from the startup file; requires an extern C interface) */
void SysTick_Handler(void)
{
    s_ms_counter++;
}

static int port_timer_init(uint32_t period_us)
{
    (void)period_us;
    if (!s_inited) {
        SysTick_Setup_1ms();
        s_inited = true;
    }
    return 0;
}

static void port_timer_deinit(void)
{
    SysTick->CTRL = 0;
}

static uint32_t port_timer_get_ms(void)
{
    return s_ms_counter;
}

static uint32_t port_timer_get_us(void)
{
    /* Simplification: return ms * 1000; the boot flow does not require us precision */
    return s_ms_counter * 1000U;
}

static void port_timer_delay_ms(uint32_t ms)
{
    DDL_DelayMS(ms);
}

static void port_timer_delay_us(uint32_t us)
{
    /* HC32 DDL has no precise us delay function; approximate with ms (boot flow is insensitive to us precision) */
    if (us >= 1000U) {
        DDL_DelayMS(us / 1000U);
    } else {
        DDL_DelayMS(1);
    }
}

static void port_timer_register_periodic_cb(tim_periodic_callback_t callback)
{
    (void)callback;
    /* Bootloader does not need a periodic callback; omitted */
}

const timer_if_t timer_hc32f4 = {
    .init                 = port_timer_init,
    .deinit               = port_timer_deinit,
    .get_us               = port_timer_get_us,
    .get_ms               = port_timer_get_ms,
    .delay_us             = port_timer_delay_us,
    .delay_ms             = port_timer_delay_ms,
    .register_periodic_cb = port_timer_register_periodic_cb,
    .instance             = 0,
};
