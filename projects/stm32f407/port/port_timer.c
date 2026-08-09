/**
 * @file port_timer.c
 * @brief Timer interface implementation using HAL (32-bit only, no 64-bit math)
 *
 * Refactor notes: the original code accumulated microseconds with a uint64_t
 * and did a 64-bit divide (/1000), which pulled in the __udivmoddi4 library
 * function (~760 B). Now we use a uint32_t millisecond counter and avoid all
 * 64-bit arithmetic.
 *
 * Microsecond precision is reconstructed as (ms_count * 1000 + TIM counter 0-999).
 * ms_count * 1000 fits within uint32_t for about 49.7 days of microseconds,
 * which is more than enough for a bootloader that runs for seconds to minutes.
 *
 * Time-difference calculations use uint32_t subtraction; wraparound is
 * handled correctly automatically (overflow after 49.7 days has no impact).
 */

#include "../hal_if_defines.h"
#include "port.h"
#include "main.h"
#include "tim.h"

extern TIM_HandleTypeDef htim6;

static volatile uint32_t s_ms_count;   /* Accumulated by 1ms interrupts, in ms */
static tim_periodic_callback_t s_periodic_callback;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        s_ms_count++;                 /* +1ms per IRQ (no longer +1000us)*/
        if (s_periodic_callback) {
            s_periodic_callback();
        }
    }
    (void)htim;
}

static int port_timer_init(uint32_t period_us)
{
    (void)period_us;
    /* Timer is already initialized by MX_TIM6_Init() */
    s_ms_count = 0;
    s_periodic_callback = NULL;
    return 0;
}

static void port_timer_deinit(void)
{
    HAL_TIM_Base_DeInit(&htim6);
}

static uint32_t port_timer_get_ms(void)
{
    /* Return the ms counter directly, no division */
    return s_ms_count;
}

static uint32_t port_timer_get_us(void)
{
    /* Reconstruct microseconds as ms_count * 1000 + current TIM counter (0-999).
     * Atomic read: loop to verify ms_count did not change while reading. */
    uint32_t ms1, ms2, cnt;
    do {
        ms1 = s_ms_count;
        cnt = __HAL_TIM_GET_COUNTER(&htim6);
        ms2 = s_ms_count;
    } while (ms1 != ms2);
    return ms1 * 1000u + cnt;
}

static void port_timer_delay_us(uint32_t us)
{
    uint32_t start = port_timer_get_us();
    while ((port_timer_get_us() - start) < us);   /* uint32_t subtraction handles wraparound automatically */
}

static void port_timer_delay_ms(uint32_t ms)
{
    uint32_t start = port_timer_get_ms();
    while ((port_timer_get_ms() - start) < ms);
}

static void port_timer_register_periodic_cb(void (*callback)(void))
{
    s_periodic_callback = callback;
}

const timer_if_t timer_stm32f4 = {
    .init                = port_timer_init,
    .deinit              = port_timer_deinit,
    .get_us              = port_timer_get_us,
    .get_ms              = port_timer_get_ms,
    .delay_us            = port_timer_delay_us,
    .delay_ms            = port_timer_delay_ms,
    .register_periodic_cb = port_timer_register_periodic_cb,
    .instance            = BL_TIMER_INSTANCE,
};
